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
    private static final int MAX_BUILD_IDS_PER_PAGE = 10;

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

            // set to hold all the build IDs
            Set<Integer> buildIdSet = new HashSet<Integer>();

            // set to hold all the test case names
            List<String> testCaseNameList = new ArrayList<String>();

            // set to hold all the test case execution results
            Map<String, String> testCaseResultMap = new HashMap();

            // set to hold the name of profiling tests to maintain uniqueness
            Set<String> profilingPointNameSet = new HashSet<String>();

            // map to hold the the list of time taken for each test. This map is saved to session
            // so that it could be used later.
            Map<String, List<Double>> mapProfilingNameValues = new HashMap();

            ResultScanner scanner = table.getScanner(new Scan());
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());

                    String buildId =
                        new String(testReportMessage.getBuildInfo().getId().toStringUtf8());
                    // filter empty build IDs
                    if (buildId.length() > 0) {
                        try {
                            buildIdSet.add(new Integer(buildId));
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
            List<Integer> sortedBuildIdList = new ArrayList(buildIdSet);
            Collections.sort(sortedBuildIdList, Collections.reverseOrder());
            int listStart = buildIdPageNo * MAX_BUILD_IDS_PER_PAGE;  // inclusive
            int listEnd = (buildIdPageNo + 1) * MAX_BUILD_IDS_PER_PAGE;  // exclusive
            if (listStart >= sortedBuildIdList.size()) {
                listStart = sortedBuildIdList.size() - 1;
            }
            if (listEnd >= sortedBuildIdList.size()) {
                listEnd = sortedBuildIdList.size() - 1;
            }
            sortedBuildIdList = sortedBuildIdList.subList(listStart, listEnd);
            List<String> selectedBuildIdList = new ArrayList<String>(sortedBuildIdList.size());
            for (Object intElem : sortedBuildIdList) {
                selectedBuildIdList.add(intElem.toString());
            }

            scanner = table.getScanner(new Scan());
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());

                    String buildId =
                        new String(testReportMessage.getBuildInfo().getId().toStringUtf8());
                    if (buildId.length() <= 0) continue;
                    if (!selectedBuildIdList.contains(buildId)) continue;

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

            scanner = table.getScanner(new Scan());
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());

                    String buildId =
                        new String(testReportMessage.getBuildInfo().getId().toStringUtf8());
                    if (buildId.length() <= 0) continue;
                    if (!selectedBuildIdList.contains(buildId)) continue;

                    for (TestCaseReportMessage testCaseReportMessage : testReportMessage.
                        getTestCaseList()) {
                        testCaseResultMap.put(
                            buildId + "." + testCaseReportMessage.getName().toStringUtf8(),
                            "" + testCaseReportMessage.getTestResult().getNumber());
                    }
                }
            }

            String[] profilingPointNameArray = profilingPointNameSet.
                toArray(new String[profilingPointNameSet.size()]);

            String[] buildNameArray = selectedBuildIdList.toArray(new String[selectedBuildIdList.size()]);
            if (profilingPointNameArray.length == 0) {
                request.setAttribute("error", PROFILING_DATA_ALERT);
            }
            session.setAttribute("mapProfilingNameValues", mapProfilingNameValues);
            request.setAttribute("tableName", table.getName());

            // pass values by converting to JSON
            request.setAttribute("buildNameArrayJson",
                                 new Gson().toJson(buildNameArray));
            request.setAttribute("testCaseNameListJson",
                                 new Gson().toJson(testCaseNameList));
            request.setAttribute("testCaseResultMapJson",
                                 new Gson().toJson(testCaseResultMap));
            request.setAttribute("profilingPointNameJson",
                                 new Gson().toJson(profilingPointNameArray));

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
