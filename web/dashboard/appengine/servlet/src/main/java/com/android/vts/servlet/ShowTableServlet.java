/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.servlet;

import com.android.vts.helpers.BigtableHelper;
import com.android.vts.proto.VtsReportMessage;
import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.CoverageReportMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to load individual tables.
 */
@WebServlet(name = "show_table", urlPatterns = {"/show_table"})
public class ShowTableServlet extends HttpServlet {

    private static final Logger logger = LoggerFactory.getLogger(ShowTableServlet.class);
    // Error message displayed on the webpage is tableName passed is null.
    private static final String TABLE_NAME_ERROR = "Error : Table name must be passed!";
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";
    private static final int MAX_BUILD_IDS_PER_PAGE = 10;
    private static final int TIME_INFO_ROW_COUNT = 2;
    private static final int DURATION_INFO_ROW_COUNT = 1;
    private static final int SUMMARY_ROW_COUNT = 4;
    private static final long ONE_DAY = 86400000000L;  // units microseconds
    private static final String TABLE_PREFIX = "result_";
    private static final byte[] FAMILY = Bytes.toBytes("test");
    private static final byte[] QUALIFIER = Bytes.toBytes("data");

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        String loginURI = userService.createLoginURL(request.getRequestURI());
        String logoutURI = userService.createLogoutURL(loginURI);
        boolean unfiltered = request.getParameter("unfiltered") != null;
        boolean showPresubmit = request.getParameter("showPresubmit") != null;
        boolean showPostsubmit = request.getParameter("showPostsubmit") != null;
        Long startTime = null;  // time in microseconds
        Long endTime = null;  // time in microseconds
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;
        boolean showMostRecent = true;  // determines whether selected subset of data is most recent

        // message to display if profiling point data is not available
        String profilingDataAlert = "";

        if (request.getParameter("testName") == null) {
            request.setAttribute("testName", TABLE_NAME_ERROR);
            dispatcher = request.getRequestDispatcher("/show_table.jsp");
            return;
        }
        tableName = TableName.valueOf(TABLE_PREFIX + request.getParameter("testName"));

        if (request.getParameter("startTime") != null) {
            String time = request.getParameter("startTime");
            try {
                startTime = Long.parseLong(time);
            } catch (NumberFormatException e) {
                startTime = null;
            }
        }
        if (request.getParameter("endTime") != null) {
            String time = request.getParameter("endTime");
            try {
                endTime = Long.parseLong(time);
            } catch (NumberFormatException e) {
                endTime = null;
            }
        }
        if (startTime != null) {
            if (endTime == null) {
                endTime = startTime + ONE_DAY;
                showMostRecent = false;
            }
        } else {
            if (endTime != null) {
                startTime = endTime - ONE_DAY;
            } else {  //both null -- i.e. init
                long now = System.currentTimeMillis() * 1000L;
                startTime = now - ONE_DAY;
                endTime = now;
            }
        }

        // If no params are specified, set to default of postsubmit-only.
        if (!(showPresubmit || showPostsubmit)) {
            showPostsubmit = true;
        }

        // If unfiltered, set showPre- and Post-submit to true for accurate UI.
        if (unfiltered) {
            showPostsubmit = true;
            showPresubmit = true;
        }

        // Add result names to list
        List<String> resultNames = new ArrayList<>();
        for (TestCaseResult r : TestCaseResult.values()) {
            resultNames.add(r.name());
        }

        // Define comparator for sorting test report messages in descending order
        Comparator<TestReportMessage> reportComparator = new Comparator<TestReportMessage>() {
            @Override
            public int compare(TestReportMessage report1, TestReportMessage report2) {
                return new Long(report2.getStartTimestamp()).compareTo(report1.getStartTimestamp());
            }
        };

        response.setContentType("text/plain");
        table = BigtableHelper.getTable(tableName);

        // this is the tip of the tree and is used for populating pie chart.
        String topBuildId = null;

        // TestReportMessage corresponding to the top build -- will be used for pie chart.
        TestReportMessage topBuildTestReportMessage = null;

        // Stores results for top build
        int[] topBuildResultCounts = new int[resultNames.size()];

        // list to hold the test report messages that will be shown on the dashboard
        List<TestReportMessage> tests = new ArrayList<TestReportMessage>();

        // list of column headers (device build IDs)
        List<String> deviceBuildIdList = new ArrayList<>();

        // set to hold orderings of each test case (testcase name to index)
        Map<String, Integer> testCaseNameMap = new HashMap<>();

        // set to hold the name of profiling tests to maintain uniqueness
        Set<String> profilingPointNameSet = new HashSet<>();

        // number of days of data parsed for the current page. Limit to MAX_BUILD_IDS_PER_PAGE days
        // (i.e. one test per day)
        int days = 1;
        while (days <= MAX_BUILD_IDS_PER_PAGE) {
            // Scan until there is a full page of data or until there is no
            // more older data.
            Scan scan = new Scan();
            scan.setStartRow(Bytes.toBytes(Long.toString(startTime)));
            scan.setStopRow(Bytes.toBytes(Long.toString(endTime)));
            ResultScanner scanner = table.getScanner(scan);
            for (Result result = scanner.next(); result != null; result = scanner.next()) {
                byte[] value = result.getValue(FAMILY, QUALIFIER);
                TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                    parseFrom(value);
                String buildId = testReportMessage.getBuildInfo().getId().toStringUtf8();

                // filter empty build IDs and add only numbers
                if (buildId == null || buildId.length() == 0) continue;

                // filter empty device info lists
                if (testReportMessage.getDeviceInfoList().size() == 0) continue;

                String firstDeviceBuildId = testReportMessage.getDeviceInfoList().get(0)
                                          .getBuildId().toStringUtf8();

                try {
                    // filter non-integer build IDs
                    if (!unfiltered) {
                        Integer.parseInt(buildId);
                        if (!showPostsubmit && firstDeviceBuildId.charAt(0) != 'P') {
                            continue;
                        }
                        if (showPresubmit && firstDeviceBuildId.charAt(0) == 'P') {
                            firstDeviceBuildId = firstDeviceBuildId.substring(1);
                        }
                        Integer.parseInt(firstDeviceBuildId);
                    }
                    tests.add(0, testReportMessage);
                } catch (NumberFormatException e) {
                    /* skip a non-post-submit build */
                    continue;
                }

                // update map of profiling point names
                for (ProfilingReportMessage profilingReportMessage :
                    testReportMessage.getProfilingList()) {

                    String profilingPointName = profilingReportMessage
                        .getName().toStringUtf8();
                    profilingPointNameSet.add(profilingPointName);
                }

                // update TestCaseReportMessage list
                for (TestCaseReportMessage testCaseReportMessage :
                     testReportMessage.getTestCaseList()) {
                    String testCaseName = new String(
                        testCaseReportMessage.getName().toStringUtf8());
                    if (!testCaseNameMap.containsKey(testCaseName)) {
                        testCaseNameMap.put(testCaseName, testCaseNameMap.size());
                    }
                }
            }
            scanner.close();
            if (tests.size() < MAX_BUILD_IDS_PER_PAGE && showMostRecent
                && BigtableHelper.hasOlder(table, startTime)) {
                // Look further back in time a day
                endTime = startTime;
                startTime -= ONE_DAY;
            } else if (tests.size() < MAX_BUILD_IDS_PER_PAGE && !showMostRecent
                       && BigtableHelper.hasNewer(table, endTime)) {
                // Look further forward in time a day
                startTime = endTime;
                endTime += ONE_DAY;
            }
            else {
                // Full page or no more data.
                break;
            }
            days += 1;
        }
        Collections.sort(tests, reportComparator);

        if (tests.size() > MAX_BUILD_IDS_PER_PAGE) {
            int startIndex;
            int endIndex;
            if (showMostRecent) {
                startIndex = 0;
                endIndex = MAX_BUILD_IDS_PER_PAGE;
            } else {
                endIndex = tests.size();
                startIndex = endIndex - MAX_BUILD_IDS_PER_PAGE;
            }
            tests = tests.subList(startIndex, endIndex);
        }

        if (tests.size() > 0) {
            startTime = tests.get(tests.size() - 1).getStartTimestamp();
            topBuildTestReportMessage = tests.get(0);
            endTime = topBuildTestReportMessage.getStartTimestamp() + 1;
        }

        if (topBuildTestReportMessage != null) {
            // create pieChartArray from top build data.
            topBuildId = topBuildTestReportMessage.getDeviceInfoList().get(0).getBuildId()
                                                .toStringUtf8();
            // Count array for each test result
            for (TestCaseReportMessage testCaseReportMessage : topBuildTestReportMessage.
                getTestCaseList()) {
                topBuildResultCounts[testCaseReportMessage.getTestResult().getNumber()]++;
            }
        }

        // The header grid on the table has 5 fields - Device build ID, Build Alias, Product Variant,
        // Build Flavor and test build ID.
        String[] headerRow = new String[tests.size() + 1];

        // the time grid on the table has one - Start Time.
        // These represent the start times for the test run.
        String[][] timeGrid = new String[TIME_INFO_ROW_COUNT][tests.size() + 1];

        // the duration grid on the table has one row - test duration.
        // These represent the length of time the test elapsed.
        String[][] durationGrid = new String[DURATION_INFO_ROW_COUNT][tests.size() + 1];

        // the summary grid has four rows - Total Row, Pass Row, Ratio Row, and Coverage %.
        String[][] summaryGrid = new String[SUMMARY_ROW_COUNT][tests.size() + 1];

        // the results an entry for each testcase result for each build.
        String[][] resultsGrid = new String[testCaseNameMap.size()][tests.size() + 1];

        // first column for device grid
        String[] headerFields = {"<b>Stats Type \\ Device Build ID</b>", "Branch", "Build Target",
                                 "Device", "VTS Build ID"};
        headerRow[0] = StringUtils.join(headerFields, "<br>");

        // first column for time grid
        String[] rowNamesTimeGrid = {"Test Start", "Test End"};
        for (int i = 0; i < rowNamesTimeGrid.length; i++) {
            timeGrid[i][0] = "<b>" + rowNamesTimeGrid[i] + "</b>";
        }

        // first column for the duration grid
        durationGrid[0][0] = "<b>Test Duration</b>";

        // first column for summary grid
        String[] rowNamesSummaryGrid = {"Total", "Passed #", "Passed %", "Coverage %"};
        for (int i = 0; i < rowNamesSummaryGrid.length; i++) {
            summaryGrid[i][0] = "<b>" + rowNamesSummaryGrid[i] + "</b>";
        }

        // first column for results grid
        for (String testCaseName : testCaseNameMap.keySet()) {
            resultsGrid[testCaseNameMap.get(testCaseName)][0] = testCaseName;
        }

        for (int j = 0; j < tests.size(); j++) {
            TestReportMessage report = tests.get(j);
            List<AndroidDeviceInfoMessage> devices = report.getDeviceInfoList();
            List<String> buildIdList = new ArrayList<>();
            List<String> buildAliasList = new ArrayList<>();
            List<String> productVariantList = new ArrayList<>();
            List<String> buildFlavorList = new ArrayList<>();
            for (AndroidDeviceInfoMessage device : devices) {
                buildAliasList.add(device.getBuildAlias().toStringUtf8().toLowerCase());
                productVariantList.add(device.getProductVariant().toStringUtf8());
                buildFlavorList.add(device.getBuildFlavor().toStringUtf8());
                buildIdList.add(device.getBuildId().toStringUtf8());
            }
            String buildAlias = StringUtils.join(buildAliasList, ",");
            String productVariant = StringUtils.join(productVariantList, ",");
            String buildFlavor = StringUtils.join(buildFlavorList, ",");
            deviceBuildIdList.add(StringUtils.join(buildIdList, ","));

            String buildId = report.getBuildInfo().getId().toStringUtf8();

            int passCount = 0;
            long totalLineCount = 0, coveredLineCount = 0;
            for (TestCaseReportMessage testCaseReport : report.getTestCaseList()) {
                if (testCaseReport.getTestResult() ==
                    TestCaseResult.TEST_CASE_RESULT_PASS) {
                    passCount++;
                }
                for (CoverageReportMessage coverageReport :
                    testCaseReport.getCoverageList()) {
                    totalLineCount += coverageReport.getTotalLineCount();
                    coveredLineCount += coverageReport.getCoveredLineCount();
                }

                int i = testCaseNameMap.get(testCaseReport.getName().toStringUtf8());
                if (testCaseReport.getTestResult() != null) {
                    resultsGrid[i][j + 1] = "<div class=\"" +
                                            testCaseReport.getTestResult().toString() +
                                            " test-case-status\">&nbsp;</div>";
                } else {
                    resultsGrid[i][j + 1] = "<div class=\"" +
                                            TestCaseResult.UNKNOWN_RESULT.toString() +
                                            " test-case-status\">&nbsp;</div>";
                }
            }
            String passInfo;
            try {
                double passPct = Math.round((100 * passCount /
                                             report.getTestCaseList().size()) * 100f) / 100f;
                passInfo = Double.toString(passPct) + " %";
            } catch (ArithmeticException e) {
                passInfo = " - ";
            }

            String coverageInfo;
            try {
                double coveragePct = Math.round((100 * coveredLineCount /
                                                 totalLineCount) * 100f) / 100f;
                coverageInfo = Double.toString(coveragePct) + " % " +
                        "<a href=\"/show_coverage?key=" + report.getStartTimestamp() +
                        "&testName=" + request.getParameter("testName") +
                        "&startTime=" + startTime +
                        "&endTime=" + endTime +
                        "\" class=\"waves-effect waves-light btn red right coverage-btn\">" +
                        "<i class=\"material-icons coverage-icon\">menu</i></a>";
            } catch (ArithmeticException e) {
                coverageInfo = " - ";
            }

            headerRow[j + 1] = "<b>" + StringUtils.join(buildIdList, ",") + "</b><br>" +
                               buildAlias.toLowerCase() + "<br>" + buildFlavor + "<br>" +
                               productVariant + "<br>" + buildId;
            timeGrid[0][j + 1] = Long.toString(report.getStartTimestamp());
            timeGrid[1][j + 1] = Long.toString(report.getEndTimestamp());
            durationGrid[0][j + 1] = Long.toString(report.getEndTimestamp() -
                                                   report.getStartTimestamp());
            summaryGrid[0][j + 1] = Integer.toString(report.getTestCaseList().size());
            summaryGrid[1][j + 1] = Integer.toString(passCount);
            summaryGrid[2][j + 1] = passInfo;
            summaryGrid[3][j + 1] = coverageInfo;
        }

        String[] profilingPointNameArray = profilingPointNameSet.
            toArray(new String[profilingPointNameSet.size()]);

        if (profilingPointNameArray.length == 0) {
            profilingDataAlert = PROFILING_DATA_ALERT;
        }

        String[] deviceBuildIdArray =
            deviceBuildIdList.toArray(new String[deviceBuildIdList.size()]);

        boolean hasNewer = BigtableHelper.hasNewer(table, endTime);
        boolean hasOlder = BigtableHelper.hasOlder(table, startTime);

        request.setAttribute("logoutURL", logoutURI);
        request.setAttribute("email", currentUser.getEmail());
        request.setAttribute("testName", request.getParameter("testName"));

        request.setAttribute("error", profilingDataAlert);
        request.setAttribute("errorJson", new Gson().toJson(profilingDataAlert));

        // pass values by converting to JSON
        request.setAttribute("headerRow", new Gson().toJson(headerRow));
        request.setAttribute("timeGrid", new Gson().toJson(timeGrid));
        request.setAttribute("durationGrid", new Gson().toJson(durationGrid));
        request.setAttribute("summaryGrid", new Gson().toJson(summaryGrid));
        request.setAttribute("resultsGrid", new Gson().toJson(resultsGrid));
        request.setAttribute("profilingPointNameJson", new Gson().toJson(profilingPointNameArray));
        request.setAttribute("deviceBuildIdArrayJson", new Gson().toJson(deviceBuildIdArray));
        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));

        // data for pie chart
        request.setAttribute("topBuildResultCounts", new Gson().toJson(topBuildResultCounts));
        request.setAttribute("topBuildId", topBuildId);
        request.setAttribute("startTime", new Gson().toJson(startTime));
        request.setAttribute("endTime", new Gson().toJson(endTime));
        request.setAttribute("hasNewer", new Gson().toJson(hasNewer));
        request.setAttribute("hasOlder", new Gson().toJson(hasOlder));
        request.setAttribute("unfiltered", unfiltered);
        request.setAttribute("showPresubmit", showPresubmit);
        request.setAttribute("showPostsubmit", showPostsubmit);

        dispatcher = request.getRequestDispatcher("/show_table.jsp");
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.error("Servlet Exception caught : " + e.toString());
        }
    }
}