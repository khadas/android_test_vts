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
import com.google.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestReportMessage;

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import org.apache.commons.math3.stat.descriptive.rank.Percentile;
import org.apache.hadoop.hbase.KeyValue;
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
import javax.servlet.http.HttpSession;
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
    private static final int MAX_BUILD_IDS_PER_PAGE = 15;
    private static final int DEVICE_INFO_ROW_COUNT = 4;
    private static final int SUMMARY_ROW_COUNT = 3;

    /**
     * Returns the table corresponding to the table name.
     * @param tableName Describes the table name which is passed as a parameter from
     *        dashboard_main.jsp, which represents the table to fetch data from.
     * @return table An instance of org.apache.hadoop.hbase.client.Table
     * @throws IOException
     */
    public Table getTable(TableName tableName) throws IOException {
        long result;
        Table table = null;

        try {
            table = BigtableHelper.getConnection().getTable(tableName);
        } catch (IOException e) {
            logger.error("Exception occurred in com.google.android.vts.servlet.DashboardServletTable."
              + "getTable()", e);
            return null;
        }
        return table;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        int buildIdPageNo;
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;

        // message to display if profiling point data is not available
        String profilingDataAlert = "";

        // session to save map object so that it can be used later.
        HttpSession session = request.getSession();
        if (request.getParameter("tableName") == null) {
            request.setAttribute("tableName", TABLE_NAME_ERROR);
            dispatcher = request.getRequestDispatcher("/show_table.jsp");
            return;
        }
        tableName = TableName.valueOf(request.getParameter("tableName"));

        buildIdPageNo = 0;
        if (request.getParameter("buildIdPageNo") != null) {
            try {
                buildIdPageNo = Integer.parseInt(request.getParameter("buildIdPageNo"));
            } catch (Exception e) {
            }
        }

        if (currentUser != null) {
            response.setContentType("text/plain");
            table = getTable(tableName);

            // list to hold a unique combination - build IDs.startTimeStamp
            List<String> sortedBuildIdTimeStampList = new ArrayList<String>();

            // set to hold all the test case names
            List<String> testCaseNameList = new ArrayList<String>();

            // set to hold all the test case execution results
            Map<String, Integer> testCaseResultMap = new HashMap();

            // set to hold the name of profiling tests to maintain uniqueness
            Set<String> profilingPointNameSet = new HashSet<String>();

            // map to hold the the list of time taken for each test. This map is saved to session
            // so that it could be used later.
            Map<String, List<Double>> mapProfilingNameValues = new HashMap();

            // Map to hold TestReportMessage based on build ID and start time stamp.
            // This will be used to obtain the corresponding device info later.
            Map<String, TestReportMessage> buildIdTimeStampMap = new HashMap();

            ResultScanner scanner = table.getScanner(new Scan());
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());

                    String buildId = testReportMessage.getBuildInfo().getId().toStringUtf8();
                    // filter empty build IDs and add only numbers
                    if (buildId.length() > 0) {
                        try {
                            Integer.parseInt(buildId);
                            String key = testReportMessage.getBuildInfo().getId().toStringUtf8() + "." +
                                String.valueOf(testReportMessage.getStartTimestamp());
                            sortedBuildIdTimeStampList.add(key);
                            // update map based on time stamp.
                            buildIdTimeStampMap.put(key, testReportMessage);
                        } catch (NumberFormatException e) {
                            /* skip a non-post-submit build */
                        }
                    }


                    // update map of profiling point names
                    for (ProfilingReportMessage profilingReportMessage :
                        testReportMessage.getProfilingList()) {

                        String profilingPointName = profilingReportMessage.getName().toStringUtf8();
                        profilingPointNameSet.add(profilingPointName);
                        double timeTaken = ((double)(profilingReportMessage.getEndTimestamp() -
                            profilingReportMessage.getStartTimestamp())) / 1000;
                        if (timeTaken < 0) {
                            logger.info("Inappropriate value for time taken");
                            continue;
                        }
                        if (!mapProfilingNameValues.containsKey(profilingPointName)) {
                            mapProfilingNameValues.put(profilingPointName, new ArrayList<Double>());
                        }
                        mapProfilingNameValues.get(profilingPointName).add(timeTaken);
                    }
                }
            }

            Collections.sort(sortedBuildIdTimeStampList, Collections.reverseOrder());
            int maxBuildIdPageNo = sortedBuildIdTimeStampList.size() / MAX_BUILD_IDS_PER_PAGE;

            int listStart = buildIdPageNo * MAX_BUILD_IDS_PER_PAGE;  // inclusive
            int listEnd = (buildIdPageNo + 1) * MAX_BUILD_IDS_PER_PAGE;  // exclusive

            if (sortedBuildIdTimeStampList.size() != 0) {
                if (listStart >= sortedBuildIdTimeStampList.size()) {
                    listStart = sortedBuildIdTimeStampList.size() - 1;
                }
                if (listEnd >= sortedBuildIdTimeStampList.size()) {
                    listEnd = sortedBuildIdTimeStampList.size() - 1;
                }
                if (sortedBuildIdTimeStampList.size() % MAX_BUILD_IDS_PER_PAGE == 0) {
                    maxBuildIdPageNo--;
                }
            } else {
                listStart = 0;
                listEnd = 0;
            }

            sortedBuildIdTimeStampList = sortedBuildIdTimeStampList.subList(listStart, listEnd);
            List<String> selectedBuildIdTimeStampList = new ArrayList<String>(sortedBuildIdTimeStampList.size());
            for (Object intElem : sortedBuildIdTimeStampList) {
                selectedBuildIdTimeStampList.add(intElem.toString());
            }


            // the device grid on the table has four rows - Build Alias, Product Variant,
            // Build Flavor and device build ID, and columns equal to the size of selectedBuildIdList.
            String[][] deviceGrid = new String[DEVICE_INFO_ROW_COUNT][selectedBuildIdTimeStampList.size() + 1];

            // the summary grid has three rows - Total Row, Pass Row and Ratio Row.
            String[][] summaryGrid = new String[SUMMARY_ROW_COUNT][selectedBuildIdTimeStampList.size() + 1];

            // first column for device grid
            String[] rowNamesDeviceGrid = {"Branch", "Build Target", "Device", "Device Build ID"};
            for (int i = 0; i < rowNamesDeviceGrid.length; i++) {
                deviceGrid[i][0] = rowNamesDeviceGrid[i];
            }

            // first column for summary grid
            String[] rowNamesSummaryGrid = {"Total", "# Passed", "% Passed"};
            for (int i = 0; i < rowNamesSummaryGrid.length; i++) {
                summaryGrid[i][0] = rowNamesSummaryGrid[i];
            }

            for (int j = 0; j < selectedBuildIdTimeStampList.size(); j++) {
                String key = selectedBuildIdTimeStampList.get(j);
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

            // build the testCaseNameList that contains unique names for test cases.
            scanner = table.getScanner(new Scan());
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());
                    String key = testReportMessage.getBuildInfo().getId().toStringUtf8() + "." +
                        String.valueOf(testReportMessage.getStartTimestamp());
                    if (!selectedBuildIdTimeStampList.contains(key)) continue;

                    // update TestCaseReportMessage list
                    for (TestCaseReportMessage testCaseReportMessage : testReportMessage.
                        getTestCaseList()) {
                        String testCaseName = new String(
                            testCaseReportMessage.getName().toStringUtf8());
                        if (!testCaseNameList.contains(testCaseName)) {
                            testCaseNameList.add(testCaseName);
                        }
                    }
                }
            }

            // build the map for grid table.
            scanner = table.getScanner(new Scan());
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());

                    String key = testReportMessage.getBuildInfo().getId().toStringUtf8() + "." +
                                 String.valueOf(testReportMessage.getStartTimestamp());
                    if (!selectedBuildIdTimeStampList.contains(key)) continue;

                    for (TestCaseReportMessage testCaseReportMessage : testReportMessage.
                        getTestCaseList()) {
                        testCaseResultMap.put(
                            key + "." + testCaseReportMessage.getName().toStringUtf8(),
                            testCaseReportMessage.getTestResult().getNumber());
                    }
                }
            }

            // rows contains the test case names and the 4 rows from device info, and 3 rows for the
            // summary.
            String[][] finalGrid =
                new String[testCaseNameList.size() + DEVICE_INFO_ROW_COUNT +
                           SUMMARY_ROW_COUNT][selectedBuildIdTimeStampList.size() + 1];
            for (int i = 0; i < DEVICE_INFO_ROW_COUNT; i++) {
                finalGrid[i] = deviceGrid[i];
            }

            // summary grid containing Integer -- this will be copied to original summary grid
            float[][] summaryGridfloat = new float[3][selectedBuildIdTimeStampList.size() + 1];

            // fill the remaining grid
            for (int i = DEVICE_INFO_ROW_COUNT + SUMMARY_ROW_COUNT; i < finalGrid.length; i++) {
                String testName = testCaseNameList.get(i - DEVICE_INFO_ROW_COUNT - SUMMARY_ROW_COUNT);
                for (int j = 0; j < finalGrid[0].length; j++) {

                    if (j == 0) {
                        finalGrid[i][j] = testName;
                        continue;
                    }
                    String key = selectedBuildIdTimeStampList.get(j - 1) + "." + testName;
                    summaryGridfloat[0][j]++;
                    Integer value = testCaseResultMap.get(key);
                    if (value != null) {
                        if (value == 1) {
                            summaryGridfloat[1][j]++;
                        }
                        finalGrid[i][j] = String.valueOf(value);
                    }

                    if (i == finalGrid.length - 1) {
                        summaryGridfloat[2][j] =
                            Math.round((100 *summaryGridfloat[1][j]/summaryGridfloat[0][j]) * 100f)/100f;
                    }
                }
            }

            // copy float values from summary grid
            for (int i = 0; i < summaryGridfloat.length; i++) {
                for (int j = 1; j < summaryGridfloat[0].length; j++) {
                    summaryGrid[i][j] = String.valueOf(summaryGridfloat[i][j]);
                    // add %
                    if (i == summaryGrid.length - 1 && summaryGrid[i][j] != null) {
                        summaryGrid[i][j] += " %";
                    }
                }
            }

            // copy the summary grid
            for (int i = DEVICE_INFO_ROW_COUNT; i < DEVICE_INFO_ROW_COUNT + SUMMARY_ROW_COUNT; i++) {
                finalGrid[i] = summaryGrid[i - DEVICE_INFO_ROW_COUNT];
            }

            String[] profilingPointNameArray = profilingPointNameSet.
                toArray(new String[profilingPointNameSet.size()]);

            String[] buildIDtimeStampArray =
                selectedBuildIdTimeStampList.toArray(new String[selectedBuildIdTimeStampList.size()]);
            if (profilingPointNameArray.length == 0) {
                profilingDataAlert = PROFILING_DATA_ALERT;
            }

            // pass values in request
            session.setAttribute("mapProfilingNameValues", mapProfilingNameValues);
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

            // pass table name back
            request.setAttribute("tableName",
                                 new Gson().toJson(request.getParameter("tableName")));

            // pass the buildIdPageNo -- pass the updated buildIdPageNo, since buildIdPageNo
            // can be modified.
            request.setAttribute("buildIdPageNo",
                                  new Gson().toJson(buildIdPageNo));

            request.setAttribute("maxBuildIdPageNo",
                                  new Gson().toJson(maxBuildIdPageNo));

            dispatcher = request.getRequestDispatcher("/show_table.jsp");
            try {
                dispatcher.forward(request, response);
            } catch (ServletException e) {
                logger.error("Servlet Excpetion caught : ", e);
            }
        } else {
            response.sendRedirect(userService.createLoginURL(request.getRequestURI()));
        }
    }
}