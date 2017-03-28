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

import com.android.vts.proto.VtsReportMessage;
import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.CoverageReportMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.SystraceReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.proto.VtsReportMessage.VtsHostInfo;
import com.android.vts.util.BigtableHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.FilterUtil.Key;
import com.android.vts.util.UrlUtil;
import com.google.gson.Gson;
import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to load individual tables.
 */
public class ShowTableServlet extends BaseServlet {

    private static final String TABLE_JSP = "WEB-INF/jsp/show_table.jsp";
    // Error message displayed on the webpage is tableName passed is null.
    private static final String TABLE_NAME_ERROR = "Error : Table name must be passed!";
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";
    private static final int MAX_BUILD_IDS_PER_PAGE = 10;
    private static final int MAX_SCAN_DAYS = 30;

    // Row labels for the test time-formatted information.
    private static final String[] TIME_INFO_NAMES = {"Test Start", "Test End"};

    // Row labels for the test duration information.
    private static final String[] DURATION_INFO_NAMES = {"<b>Test Duration</b>"};

    // Row labels for the test summary grid.
    private static final String[] SUMMARY_NAMES = {
            "Total", "Passing #", "Non-Passing #", "Passing %", "Covered Lines", "Coverage %",
            "Logs"};

    // Row labels for the device summary information in the table header.
    private static final String[] HEADER_NAMES = {
            "<b>Stats Type \\ Device Build ID</b>", "Branch", "Build Target", "Device",
            "ABI Target", "VTS Build ID", "Hostname"};

    private static final String SEARCH_HELP_HEADER = "Search Help";
    private static final String SEARCH_HELP = "Data can be queried using one or more filters. " +
        "If more than one filter is provided, results will be returned that match <i>all</i>. " +
        "<br><br>Filters are delimited by spaces; to specify a multi-word token, enclose it in " +
        "double quotes. A query must be in the format: \"field:value\".<br><br>" +
        "<b>Supported field qualifiers:</b> " + StringUtils.join(Key.values(), ", ") + ".";
    private static final byte[] FAMILY = Bytes.toBytes("test");
    private static final byte[] QUALIFIER = Bytes.toBytes("data");

    @Override
    public List<String[]> getNavbarLinks(HttpServletRequest request) {
        List<String[]> links = new ArrayList<>();
        Page root = Page.HOME;
        String[] rootEntry = new String[]{root.getUrl(), root.getName()};
        links.add(rootEntry);

        Page table = Page.TABLE;
        String testName = request.getParameter("testName");
        String name = table.getName() + testName;
        String url = table.getUrl() + "?testName=" + testName;
        String[] tableEntry = new String[]{url, name};
        links.add(tableEntry);
        return links;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        boolean unfiltered = request.getParameter("unfiltered") != null;
        boolean showPresubmit = request.getParameter("showPresubmit") != null;
        boolean showPostsubmit = request.getParameter("showPostsubmit") != null;
        String searchString = request.getParameter("search");
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

        Map<Key, String> searchPairs  = FilterUtil.parseSearchString(searchString);

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
        table = BigtableHelper.getTable(tableName);

        // this is the tip of the tree and is used for populating pie chart.
        String topBuildId = null;

        // TestReportMessage corresponding to the top build -- will be used for pie chart.
        TestReportMessage topBuildTestReportMessage = null;

        // Stores results for top build
        int[] topBuildResultCounts = new int[resultNames.size()];

        // list to hold the test report messages that will be shown on the dashboard
        List<TestReportMessage> tests = new ArrayList<TestReportMessage>();

        // set to hold orderings of each test case (testcase name to index)
        Map<String, Integer> testCaseNameMap = new HashMap<>();

        // map to hold log names for tests
        Map<String, List<String[]>> logInfoMap = new HashMap<>();

        // set to hold the name of profiling tests to maintain uniqueness
        Set<String> profilingPointNameSet = new HashSet<>();

        // number of days of data parsed for the current page. Limit to MAX_BUILD_IDS_PER_PAGE days
        // (i.e. one test per day)
        int days = 1;
        while (true) {
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
                if (!FilterUtil.includeInSearch(testReportMessage, searchPairs)) continue;

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

                    String profilingPointName = profilingReportMessage.getName().toStringUtf8();
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
            if (days >= MAX_SCAN_DAYS) {
                break;
            } else if (tests.size() < MAX_BUILD_IDS_PER_PAGE && showMostRecent
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

        // the results an entry for each testcase result for each build.
        String[][] resultsGrid = new String[testCaseNameMap.size()][tests.size() + 1];
        headerRow[0] = StringUtils.join(HEADER_NAMES, "<br>");

        String[][] timeGrid = new String[TIME_INFO_NAMES.length][tests.size() + 1];
        for (int i = 0; i < TIME_INFO_NAMES.length; i++) {
            timeGrid[i][0] = "<b>" + TIME_INFO_NAMES[i] + "</b>";
        }

        String[][] durationGrid = new String[DURATION_INFO_NAMES.length][tests.size() + 1];
        for (int i = 0; i < DURATION_INFO_NAMES.length; i++) {
            durationGrid[i][0] = "<b>" + DURATION_INFO_NAMES[i] + "</b>";
        }

        String[][] summaryGrid = new String[SUMMARY_NAMES.length][tests.size() + 1];
        for (int i = 0; i < SUMMARY_NAMES.length; i++) {
            summaryGrid[i][0] = "<b>" + SUMMARY_NAMES[i] + "</b>";
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
            List<String> abiInfoList = new ArrayList<>();
            for (AndroidDeviceInfoMessage device : devices) {
                buildAliasList.add(device.getBuildAlias().toStringUtf8().toLowerCase());
                productVariantList.add(device.getProductVariant().toStringUtf8());
                buildFlavorList.add(device.getBuildFlavor().toStringUtf8());
                buildIdList.add(device.getBuildId().toStringUtf8());
                String abi = "";
                String abiName = device.getAbiName().toStringUtf8();
                String abiBitness = device.getAbiBitness().toStringUtf8();
                if (abiName.length() > 0) {
                    abi += abiName;
                    if (abiBitness.length() > 0) {
                        abi += " (" + abiBitness + " bit)";
                    }
                }
                abiInfoList.add(abi);
            }
            String buildAlias = StringUtils.join(buildAliasList, ",");
            String productVariant = StringUtils.join(productVariantList, ",");
            String buildFlavor = StringUtils.join(buildFlavorList, ",");
            String buildIds = StringUtils.join(buildIdList, ",");
            String abiInfo = StringUtils.join(abiInfoList, ",");
            String vtsBuildId = report.getBuildInfo().getId().toStringUtf8();

            int passCount = 0;
            int nonpassCount = 0;
            TestCaseResult aggregateStatus = TestCaseResult.UNKNOWN_RESULT;
            long totalLineCount = 0, coveredLineCount = 0;
            for (CoverageReportMessage coverageReport : report.getCoverageList()) {
                totalLineCount += coverageReport.getTotalLineCount();
                coveredLineCount += coverageReport.getCoveredLineCount();
            }
            for (TestCaseReportMessage testCaseReport : report.getTestCaseList()) {
                if (testCaseReport.getTestResult() ==
                    TestCaseResult.TEST_CASE_RESULT_PASS) {
                    passCount++;
                    if (aggregateStatus == TestCaseResult.UNKNOWN_RESULT) {
                        aggregateStatus = TestCaseResult.TEST_CASE_RESULT_PASS;
                    }
                } else if (testCaseReport.getTestResult() !=
                           TestCaseResult.TEST_CASE_RESULT_SKIP) {
                    nonpassCount++;
                    aggregateStatus = TestCaseResult.TEST_CASE_RESULT_FAIL;
                }
                for (CoverageReportMessage coverageReport :
                     testCaseReport.getCoverageList()) {
                    totalLineCount += coverageReport.getTotalLineCount();
                    coveredLineCount += coverageReport.getCoveredLineCount();
                }

                String systraceUrl = null;
                for (SystraceReportMessage systraceReport :
                     testCaseReport.getSystraceList()) {
                    if (systraceReport.getUrlList().size() == 0) {
                        continue;
                    }
                    // Validate the url
                    String urlString = systraceReport.getUrlList().get(0).toStringUtf8();
                    String validatedUrl = UrlUtil.validateUrl(urlString);
                    if (validatedUrl != null) {
                        systraceUrl = validatedUrl;
                    } else {
                        logger.log(Level.WARNING, "Invalid systrace URL : " + urlString);
                    }
                }

                int i = testCaseNameMap.get(testCaseReport.getName().toStringUtf8());
                String classNames = "test-case-status ";
                String glyph = "";
                if (testCaseReport.getTestResult() != null)
                    classNames += testCaseReport.getTestResult().toString();
                else
                    classNames += TestCaseResult.UNKNOWN_RESULT.toString();

                if (systraceUrl != null) {
                    classNames += " width-1";
                    glyph += "<a href=\"" + systraceUrl + "\" " +
                             "class=\"waves-effect waves-light btn red right inline-btn\">" +
                             "<i class=\"material-icons inline-icon\">info_outline</i></a>";
                }
                resultsGrid[i][j + 1] = "<div class=\"" + classNames + "\">&nbsp;</div>" + glyph;
            }
            String passInfo;
            try {
                double passPct = Math.round((100 * passCount /
                                             (passCount + nonpassCount)) * 100f) / 100f;
                passInfo = Double.toString(passPct) + "%";
            } catch (ArithmeticException e) {
                passInfo = " - ";
            }

            String coverageInfo;
            String coveragePctInfo;
            try {
                double coveragePct = Math.round((100 * coveredLineCount /
                                                 totalLineCount) * 100f) / 100f;
                coveragePctInfo = Double.toString(coveragePct) + "%" +
                        "<a href=\"/show_coverage?testName=" + request.getParameter("testName") +
                        "&startTime=" + (report.getStartTimestamp() * MILLI_TO_MICRO) +
                        "\" class=\"waves-effect waves-light btn red right inline-btn\">" +
                        "<i class=\"material-icons inline-icon\">menu</i></a>";
                coverageInfo = coveredLineCount + "/" + totalLineCount;
            } catch (ArithmeticException e) {
                coveragePctInfo = " - ";
                coverageInfo = " - ";
            }

            // Process log information
            String logSummary = " - ";
            int logCount = 0;
            List<String[]> logEntries = new ArrayList<>();
            logInfoMap.put(Integer.toString(j), logEntries);
            for (VtsReportMessage.LogMessage log : report.getLogList()) {
                if (!log.hasName() || !log.hasUrl()) continue;
                String rawUrl = log.getUrl().toStringUtf8();
                String validatedUrl = UrlUtil.validateUrl(rawUrl);
                if (validatedUrl == null) {
                    logger.log(Level.WARNING, "Invalid logging URL : " + rawUrl);
                    continue;
                }
                String[] logInfo = new String[]{
                        log.getName().toStringUtf8(), log.getUrl().toStringUtf8()
                    };
                logEntries.add(logInfo);
                logCount++;
            }
            if (logCount > 0) {
                logSummary = Integer.toString(logCount);
                logSummary += "<i class=\"waves-effect waves-light btn red right inline-btn"
                        + " info-btn material-icons inline-icon\""
                        + " data-col=\"" + Integer.toString(j) + "\""
                        + ">launch</i>";
            }

            String icon = "<div class='status-icon " + aggregateStatus.toString() + "'>&nbsp</div>";
            VtsHostInfo hostInfo = report.getHostInfo();
            String hostname = hostInfo == null ? "" : hostInfo.getHostname().toStringUtf8();
            headerRow[j + 1] = "<span class='valign-wrapper'><b>" + buildIds + "</b>" + icon +
                               "</span>" + buildAlias.toLowerCase() + "<br>" + buildFlavor +
                               "<br>" + productVariant + "<br>" +
                               abiInfo + "<br>" + vtsBuildId + "<br>" + hostname;
            timeGrid[0][j + 1] = Long.toString(report.getStartTimestamp());
            timeGrid[1][j + 1] = Long.toString(report.getEndTimestamp());
            durationGrid[0][j + 1] = Long.toString(report.getEndTimestamp() -
                                                   report.getStartTimestamp());
            summaryGrid[0][j + 1] = Integer.toString(report.getTestCaseList().size());
            summaryGrid[1][j + 1] = Integer.toString(passCount);
            summaryGrid[2][j + 1] = Integer.toString(nonpassCount);
            summaryGrid[3][j + 1] = passInfo;
            summaryGrid[4][j + 1] = coverageInfo;
            summaryGrid[5][j + 1] = coveragePctInfo;
            summaryGrid[6][j + 1] = logSummary;
        }

        String[] profilingPointNameArray = profilingPointNameSet.
            toArray(new String[profilingPointNameSet.size()]);
        Arrays.sort(profilingPointNameArray);

        if (profilingPointNameArray.length == 0) {
            profilingDataAlert = PROFILING_DATA_ALERT;
        }

        boolean hasNewer = BigtableHelper.hasNewer(table, endTime);
        boolean hasOlder = BigtableHelper.hasOlder(table, startTime);

        request.setAttribute("testName", request.getParameter("testName"));

        request.setAttribute("error", profilingDataAlert);
        request.setAttribute("errorJson", new Gson().toJson(profilingDataAlert));
        request.setAttribute("searchString", searchString);
        request.setAttribute("searchHelpHeader", SEARCH_HELP_HEADER);
        request.setAttribute("searchHelpBody", SEARCH_HELP);

        // pass values by converting to JSON
        request.setAttribute("headerRow", new Gson().toJson(headerRow));
        request.setAttribute("timeGrid", new Gson().toJson(timeGrid));
        request.setAttribute("durationGrid", new Gson().toJson(durationGrid));
        request.setAttribute("summaryGrid", new Gson().toJson(summaryGrid));
        request.setAttribute("resultsGrid", new Gson().toJson(resultsGrid));
        request.setAttribute("profilingPointNames", profilingPointNameArray);
        request.setAttribute("profilingPointNameJson", new Gson().toJson(profilingPointNameArray));
        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));
        request.setAttribute("logInfoMap", new Gson().toJson(logInfoMap));

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

        dispatcher = request.getRequestDispatcher(TABLE_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : " + e.toString());
        }
    }
}