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

package com.google.android.vts.servlet;

import com.google.android.vts.proto.VtsReportMessage;
import com.google.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.google.android.vts.proto.VtsReportMessage.CoverageReportMessage;
import com.google.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestReportMessage;

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
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
    private static final int MAX_BUILD_IDS_PER_PAGE = 12;
    private static final int DEVICE_INFO_ROW_COUNT = 4;
    private static final int SUMMARY_ROW_COUNT = 4;
    private static final long ONE_DAY = 86400000000000L;  // units microseconds

    // test result constants
    private static final int TEST_RESULT_CASES = 6;
    private static final int UNKNOWN_RESULT = 0;
    private static final String[] TEST_RESULT_NAMES =
        {"Unknown", "Pass", "Fail", "Skip", "Exception", "Timeout"};

    // pie chart table column headers
    private static final String PIE_CHART_TEST_RESULT_NAME = "Test Result Name";
    private static final String PIE_CHART_TEST_RESULT_VALUE = "Test Result Value";

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        Long buildIdStartTime = null;  // time in microseconds
        Long buildIdEndTime = null;  // time in microseconds
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;
        boolean showMostRecent = true;  // determines whether selected subset of data is most recent

        // message to display if profiling point data is not available
        String profilingDataAlert = "";

        if (request.getParameter("tableName") == null) {
            request.setAttribute("tableName", TABLE_NAME_ERROR);
            dispatcher = request.getRequestDispatcher("/show_table.jsp");
            return;
        }
        tableName = TableName.valueOf(request.getParameter("tableName"));

        if (request.getParameter("buildIdStartTime") != null) {
            String time = request.getParameter("buildIdStartTime");
            try {
                buildIdStartTime = Long.parseLong(time);
            } catch (NumberFormatException e) {
                buildIdStartTime = null;
            }
        }
        if (request.getParameter("buildIdEndTime") != null) {
            String time = request.getParameter("buildIdEndTime");
            try {
                buildIdEndTime = Long.parseLong(time);
            } catch (NumberFormatException e) {
                buildIdEndTime = null;
            }
        }
        if (buildIdStartTime != null) {
            if (buildIdEndTime == null) {
                buildIdEndTime = buildIdStartTime + ONE_DAY;
                showMostRecent = false;
            }
        } else {
            if (buildIdEndTime != null) {
                buildIdStartTime = buildIdEndTime - ONE_DAY;
            } else {  //both null -- i.e. init
                long now = System.currentTimeMillis() * 1000000L;
                buildIdStartTime = now - ONE_DAY;
                buildIdEndTime = now;
            }
        }

        if (currentUser != null) {
            response.setContentType("text/plain");
            table = BigtableHelper.getTable(tableName);

            // this is the tip of the tree and is used for populating pie chart.
            String topBuild = null;

            // TestReportMessage corresponding to the top build -- will be used for pie chart.
            TestReportMessage topBuildTestReportMessage = null;

            // Each case corresponds to an array of size 2.
            // First column represents the result name and second represents the number of results.
            String[][] pieChartArray = new String[TEST_RESULT_CASES + 1][2];

            // list to hold a unique combination - build IDs.startTimeStamp
            List<String> sortedBuildIdTimeStampList = new ArrayList<>();

            // set to hold all the test case names
            List<String> testCaseNameList = new ArrayList<>();

            // set to hold all the test case execution results
            Map<String, Integer> testCaseResultMap = new HashMap<>();

            // set to hold the name of profiling tests to maintain uniqueness
            Set<String> profilingPointNameSet = new HashSet<>();

            // Map to hold TestReportMessage based on build ID and start time stamp.
            // This will be used to obtain the corresponding device info later.
            Map<String, TestReportMessage> buildIdTimeStampMap = new HashMap<>();

            while (true) {
                // Scan until there is a full page of data or until there is no
                // more older data.
                Scan scan = new Scan();
                scan.setStartRow(Long.toString(buildIdStartTime).getBytes());
                scan.setStopRow(Long.toString(buildIdEndTime).getBytes());
                ResultScanner scanner = table.getScanner(scan);
                for (Result result = scanner.next(); result != null; result = scanner.next()) {
                    for (Cell cell : result.rawCells()) {
                        TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                            parseFrom(cell.getValueArray());
                        String buildId = testReportMessage.getBuildInfo().getId().toStringUtf8();

                        // filter empty build IDs and add only numbers
                        if (buildId.length() == 0) continue;

                        String key;
                        try {
                            Integer.parseInt(buildId);
                            key = testReportMessage.getBuildInfo().getId().toStringUtf8()
                              + "." + String.valueOf(testReportMessage.getStartTimestamp());
                            sortedBuildIdTimeStampList.add(key);
                            // update map based on time stamp.
                            buildIdTimeStampMap.put(key, testReportMessage);
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
                            if (!testCaseNameList.contains(testCaseName)) {
                                testCaseNameList.add(testCaseName);
                            }
                            testCaseResultMap.put(
                                key + "." + testCaseReportMessage.getName().toStringUtf8(),
                                testCaseReportMessage.getTestResult().getNumber());
                        }
                    }
                }
                scanner.close();
                if (sortedBuildIdTimeStampList.size() < MAX_BUILD_IDS_PER_PAGE
                    && BigtableHelper.hasOlder(table, buildIdStartTime)) {
                    // Look further back in time a day
                    buildIdEndTime = buildIdStartTime;
                    buildIdStartTime -= ONE_DAY;
                } else {
                    // Full page or no more data.
                    break;
                }
            }

            Collections.sort(sortedBuildIdTimeStampList, Collections.reverseOrder());
            if (sortedBuildIdTimeStampList.size() > MAX_BUILD_IDS_PER_PAGE) {
                int startIndex;
                int endIndex;
                if (showMostRecent) {
                    startIndex = 0;
                    endIndex = MAX_BUILD_IDS_PER_PAGE;
                } else {
                    endIndex = sortedBuildIdTimeStampList.size() - 1;
                    startIndex = endIndex - MAX_BUILD_IDS_PER_PAGE + 1;
                }
                sortedBuildIdTimeStampList = sortedBuildIdTimeStampList
                                                .subList(startIndex, endIndex);
            }
            if (sortedBuildIdTimeStampList.size() > 0) {
                buildIdStartTime = buildIdTimeStampMap
                    .get(sortedBuildIdTimeStampList
                        .get(sortedBuildIdTimeStampList.size()-1))
                    .getStartTimestamp() * 1000;
                topBuild = sortedBuildIdTimeStampList.get(0);
                topBuildTestReportMessage = buildIdTimeStampMap.get(topBuild);
                buildIdEndTime = topBuildTestReportMessage
                    .getStartTimestamp() * 1000;
            }

            if (topBuildTestReportMessage != null) {
                // create pieChartArray from top build data.
                // first row is for headers.
                pieChartArray[0][0] = PIE_CHART_TEST_RESULT_NAME;
                pieChartArray[0][1] = PIE_CHART_TEST_RESULT_VALUE;
                for (int i = 1; i < pieChartArray.length; i++) {
                    pieChartArray[i][0] = TEST_RESULT_NAMES[i - 1];
                }

                // temporary count array for each test result
                int[] testResultCount = new int[TEST_RESULT_CASES];
                for (TestCaseReportMessage testCaseReportMessage : topBuildTestReportMessage.
                    getTestCaseList()) {
                    testResultCount[testCaseReportMessage.getTestResult().getNumber()]++;
                }

                // update the pie chart array
                // create pieChartArray from top build data.
                for (int i = 1; i < pieChartArray.length; i++) {
                    pieChartArray[i][1] = String.valueOf(testResultCount[i - 1]);
                }
            }


            // the device grid on the table has four rows - Build Alias, Product Variant,
            // Build Flavor and device build ID, and columns equal to the size of selectedBuildIdList.
            String[][] deviceGrid = new String[DEVICE_INFO_ROW_COUNT][
                sortedBuildIdTimeStampList.size() + 1];

            // the summary grid has four rows - Total Row, Pass Row, Ratio Row, and Coverage %.
            String[][] summaryGrid = new String[SUMMARY_ROW_COUNT][
                sortedBuildIdTimeStampList.size() + 1];

            // first column for device grid
            String[] rowNamesDeviceGrid = {"Branch", "Build Target", "Device", "Device Build ID"};
            for (int i = 0; i < rowNamesDeviceGrid.length; i++) {
                deviceGrid[i][0] = rowNamesDeviceGrid[i];
            }

            // first column for summary grid
            String[] rowNamesSummaryGrid = {"Total", "Passed #", "Passed %", "Coverage %"};
            for (int i = 0; i < rowNamesSummaryGrid.length; i++) {
                summaryGrid[i][0] = rowNamesSummaryGrid[i];
            }

            for (int j = 0; j < sortedBuildIdTimeStampList.size(); j++) {
                String key = sortedBuildIdTimeStampList.get(j);
                List<AndroidDeviceInfoMessage> list =
                    buildIdTimeStampMap.get(key).getDeviceInfoList();
                String buildAlias = "", productVariant = "", buildFlavor = "", deviceBuildID = "";
                for (AndroidDeviceInfoMessage device : list) {
                    buildAlias += device.getBuildAlias().toStringUtf8() + ",";
                    productVariant += device.getProductVariant().toStringUtf8() + ",";
                    buildFlavor += device.getBuildFlavor().toStringUtf8() + ",";
                    deviceBuildID += device.getBuildId().toStringUtf8() + ",";
                }
                buildAlias = buildAlias.length() > 0 ?
                    buildAlias.substring(0, buildAlias.length() - 1) : buildAlias;
                productVariant = productVariant.length() > 0 ?
                    productVariant.substring(0, productVariant.length() - 1) : productVariant;
                buildFlavor = buildFlavor.length() > 0 ?
                    buildFlavor.substring(0, buildFlavor.length() - 1) : buildFlavor;
                deviceBuildID = deviceBuildID.length() > 0 ?
                    deviceBuildID.substring(0, deviceBuildID.length() - 1) : deviceBuildID;

                deviceGrid[0][j + 1] = buildAlias.toLowerCase();
                deviceGrid[1][j + 1] = buildFlavor;
                deviceGrid[2][j + 1] = productVariant;
                deviceGrid[3][j + 1] = deviceBuildID;
            }


            // rows contains the rows from test case names, device info, and from the summary.
            String[][] finalGrid =
                new String[testCaseNameList.size() + DEVICE_INFO_ROW_COUNT +
                           SUMMARY_ROW_COUNT][sortedBuildIdTimeStampList.size() + 1];
            for (int i = 0; i < DEVICE_INFO_ROW_COUNT; i++) {
                finalGrid[i] = deviceGrid[i];
            }

            // summary grid containing Integer -- this will be copied to original summary grid
            float[][] summaryGridfloat = new float[3][sortedBuildIdTimeStampList.size() + 1];

            // fill the remaining grid
            for (int i = DEVICE_INFO_ROW_COUNT + SUMMARY_ROW_COUNT; i < finalGrid.length; i++) {
                String testName = testCaseNameList.get(
                    i - DEVICE_INFO_ROW_COUNT - SUMMARY_ROW_COUNT);
                for (int j = 0; j < finalGrid[0].length; j++) {

                    if (j == 0) {
                        finalGrid[i][j] = testName;
                        continue;
                    }
                    String key = sortedBuildIdTimeStampList.get(j - 1) + "." + testName;
                    summaryGridfloat[0][j]++;
                    Integer value = testCaseResultMap.get(key);
                    if (value != null) {
                        if (value == 1) {
                            summaryGridfloat[1][j]++;
                        }
                        finalGrid[i][j] = String.valueOf(value);
                    } else {
                        finalGrid[i][j] = String.valueOf(UNKNOWN_RESULT);
                    }

                    if (i == finalGrid.length - 1) {
                        try {
                            summaryGridfloat[2][j] =
                                Math.round((100 * summaryGridfloat[1][j] / summaryGridfloat[0][j])
                                           * 100f) / 100f;
                        } catch (ArithmeticException e) {
                            /* ignore where total test cases is zero*/
                        }
                    }
                }
            }

            // copy float values from summary grid
            for (int i = 0; i < summaryGridfloat.length; i++) {
                for (int j = 1; j < summaryGridfloat[0].length; j++) {
                    summaryGrid[i][j] = String.valueOf(summaryGridfloat[i][j]);
                    // add % for second last row
                    if (i == summaryGrid.length - 2) {
                        summaryGrid[i][j] += " %";
                    }
                }
            }

            // last row of summary grid
            // calculate coverage % for each column
            for (int j = 0; j < sortedBuildIdTimeStampList.size(); j++) {
                String key = sortedBuildIdTimeStampList.get(j);
                TestReportMessage testReportMessage = buildIdTimeStampMap.get(key);

                for (TestCaseReportMessage testCaseReportMessage
                     : testReportMessage.getTestCaseList()) {
                    double totalLineCount = 0, coveredLineCount = 0;
                    for (CoverageReportMessage coverageReportMessage :
                        testCaseReportMessage.getCoverageList()) {
                        totalLineCount += coverageReportMessage.getTotalLineCount();
                        coveredLineCount += coverageReportMessage.getCoveredLineCount();
                    }
                    // j + 1 is the column index
                    if (totalLineCount != 0) {
                        summaryGrid[SUMMARY_ROW_COUNT - 1][j + 1] =
                            String.valueOf(Math.round((100 *coveredLineCount / totalLineCount)
                                                      * 100d) / 100d)
                                    + "%";
                    } else {
                        summaryGrid[SUMMARY_ROW_COUNT - 1][j + 1] = "NA";
                    }
                }
            }

            // copy the summary grid
            for (int i = DEVICE_INFO_ROW_COUNT;
                 i < DEVICE_INFO_ROW_COUNT + SUMMARY_ROW_COUNT; i++) {
                finalGrid[i] = summaryGrid[i - DEVICE_INFO_ROW_COUNT];
            }

            String[] profilingPointNameArray = profilingPointNameSet.
                toArray(new String[profilingPointNameSet.size()]);

            String[] buildIDtimeStampArray =
                sortedBuildIdTimeStampList.toArray(
                    new String[sortedBuildIdTimeStampList.size()]);
            if (profilingPointNameArray.length == 0) {
                profilingDataAlert = PROFILING_DATA_ALERT;
            }

            boolean hasNewer = BigtableHelper.hasNewer(table, buildIdEndTime);
            boolean hasOlder = BigtableHelper.hasOlder(table, buildIdStartTime);

            request.setAttribute("tableName", table.getName());

            request.setAttribute("error", profilingDataAlert);
            request.setAttribute("errorJson",
                new Gson().toJson(profilingDataAlert));

            // pass values by converting to JSON
            request.setAttribute("finalGridJson",
                                 new Gson().toJson(finalGrid));
            request.setAttribute("buildIDtimeStampArrayJson",
                                 new Gson().toJson(buildIDtimeStampArray));
            request.setAttribute("profilingPointNameJson",
                                 new Gson().toJson(profilingPointNameArray));

            // data for pie chart
            request.setAttribute("pieChartArrayJson",
                new Gson().toJson(pieChartArray));

            request.setAttribute("topBuildJson",
                new Gson().toJson(topBuild));

            request.setAttribute("buildIdStartTime",
                                  new Gson().toJson(buildIdStartTime));

            request.setAttribute("buildIdEndTime",
                                  new Gson().toJson(buildIdEndTime));

            request.setAttribute("summaryRowCountJson",
                new Gson().toJson(SUMMARY_ROW_COUNT));

            request.setAttribute("deviceInfoRowCountJson",
                new Gson().toJson(DEVICE_INFO_ROW_COUNT));

            request.setAttribute("hasNewer", new Gson().toJson(hasNewer));

            request.setAttribute("hasOlder", new Gson().toJson(hasOlder));

            dispatcher = request.getRequestDispatcher("/show_table.jsp");
            try {
                dispatcher.forward(request, response);
            } catch (ServletException e) {
                logger.error("Servlet Excpetion caught : " + e.toString());
            }
        } else {
            response.sendRedirect(userService.createLoginURL(request.getRequestURI()));
        }
    }
}