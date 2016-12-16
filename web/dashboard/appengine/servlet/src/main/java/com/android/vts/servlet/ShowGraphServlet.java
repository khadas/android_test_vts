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
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import org.apache.commons.math3.stat.descriptive.rank.Percentile;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import com.google.gson.Gson;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to load graphs.
 */
@WebServlet(name = "show_graph", urlPatterns = {"/show_graph"})
public class ShowGraphServlet extends HttpServlet {

    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";
    private static final byte[] FAMILY = Bytes.toBytes("test");
    private static final byte[] QUALIFIER = Bytes.toBytes("data");
    private static final String TABLE_PREFIX = "result_";
    private static final long ONE_DAY = 86400000000000L;  // units microseconds
    private static final Logger logger = LoggerFactory.getLogger(ShowGraphServlet.class);

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        String loginURI = userService.createLoginURL(request.getRequestURI());
        String logoutURI = userService.createLogoutURL(loginURI);

        String profilingPointName = request.getParameter("profilingPoint");
        long startTime;
        long endTime;
        try {
            startTime = Long.parseLong(request.getParameter("startTime"));
            endTime = Long.parseLong(request.getParameter("endTime"));
        } catch (NumberFormatException e){
            long now = System.currentTimeMillis() * 1000000L;
            startTime = now - ONE_DAY;
            endTime = now;
        }
        tableName = TableName.valueOf(TABLE_PREFIX + request.getParameter("testName"));
        table = BigtableHelper.getTable(tableName);

        // This list holds the values for all profiling points.
        List<Double> profilingPointValuesList = new ArrayList<>();

        // Map for labels and values
        Map<String, Integer> labelIndexMap = new HashMap<>();

        // Set of all labels
        List<String> labels = new ArrayList<>();

        // List of all profiling vectors
        List<ProfilingReportMessage> profilingVectors = new ArrayList<>();

        // List of build IDs for each profiling vector
        List<String> profilingBuildIds = new ArrayList<>();

        // Performance X and Y labels
        String performanceLabelX = "";
        String performanceLabelY = "";

        boolean showProfilingGraph = false;
        boolean showPerformanceGraph = false;

        Scan scan = new Scan();
        scan.setStartRow(Long.toString(startTime).getBytes());
        scan.setStopRow(Long.toString(endTime).getBytes());
        ResultScanner scanner = table.getScanner(scan);
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(FAMILY, QUALIFIER);
            TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                parseFrom(value);

            // update map of profiling point names
            for (ProfilingReportMessage profilingReportMessage :
                testReportMessage.getProfilingList()) {
                if (!profilingPointName.equals(profilingReportMessage.getName().toStringUtf8())) {
                    continue;
                }
                switch(profilingReportMessage.getType()) {
                    case UNKNOWN_VTS_PROFILING_TYPE:
                    case VTS_PROFILING_TYPE_TIMESTAMP :
                        double timeTaken = ((double)(profilingReportMessage.getEndTimestamp() -
                            profilingReportMessage.getStartTimestamp())) / 1000;
                        if (timeTaken < 0) {
                            logger.info("Inappropriate value for time taken");
                            continue;
                        }
                        profilingPointValuesList.add(timeTaken);
                        break;

                    case VTS_PROFILING_TYPE_LABELED_VECTOR :
                        if (profilingReportMessage.getLabelList().size() != 0 &&
                            profilingReportMessage.getLabelList().size() ==
                            profilingReportMessage.getValueList().size()) {
                            profilingVectors.add(profilingReportMessage);
                            profilingBuildIds.add(testReportMessage.getBuildInfo()
                                .getId().toStringUtf8());
                        }
                        break;
                    default :
                        break;
                }
            }
        }
        if (profilingVectors.size() > 0) {
            // Use the most recent profiling vector to generate the labels
            ProfilingReportMessage profilingReportMessage = profilingVectors
                .get(profilingVectors.size() - 1);
            for (int i = 0; i < profilingReportMessage.getLabelList().size(); i++) {
                labels.add(profilingReportMessage.getLabelList().get(i).toStringUtf8());
            }
            performanceLabelX = profilingReportMessage.getXAxisLabel().toStringUtf8();
            performanceLabelY = profilingReportMessage.getYAxisLabel().toStringUtf8();
            showPerformanceGraph = true;
        }
        for (int i = 0; i < labels.size(); i++) {
            labelIndexMap.put(labels.get(i), i);
        }
        long[][] lineGraphValues = new long[labels.size()][profilingVectors.size()];
        for (int reportIndex = 0; reportIndex < profilingVectors.size(); reportIndex++) {
            ProfilingReportMessage report = profilingVectors.get(reportIndex);
            for (int i = 0; i < report.getLabelList().size(); i++) {
                String label = report.getLabelList().get(i).toStringUtf8();

                // Skip value if its label is not present
                if (!labelIndexMap.containsKey(label)) continue;
                lineGraphValues[labelIndexMap.get(label)][reportIndex] =
                    report.getValueList().get(i);
            }
        }

        // fill performance profiling array
        double[] performanceProfilingValues = new double[profilingPointValuesList.size()];
        for (int i = 0; i < profilingPointValuesList.size(); i++) {
            performanceProfilingValues[i] = profilingPointValuesList.get(i).doubleValue();
        }

        // pass map to show_graph.jsp through request by converting to JSON
        String valuesJson = new Gson().toJson(performanceProfilingValues);
        request.setAttribute("performanceValuesJson", valuesJson);

        int[] percentiles = {10, 25, 50 , 75, 80, 90, 95, 99};
        double[] percentileValuesArray = new double[percentiles.length];
        for (int i = 0; i < percentiles.length; i++) {
            percentileValuesArray[i] =
                Math.round(new Percentile().evaluate(performanceProfilingValues, percentiles[i]) * 1000d) / 1000d;
        }

        showProfilingGraph = performanceProfilingValues.length != 0;

        if (!showProfilingGraph && !showPerformanceGraph) {
            request.setAttribute("error", PROFILING_DATA_ALERT);
        }

        request.setAttribute("logoutURL", logoutURI);
        request.setAttribute("email", currentUser.getEmail());
        // performance data for scatter plot
        request.setAttribute("showProfilingGraph", showProfilingGraph);
        request.setAttribute("showPerformanceGraph", showPerformanceGraph);
        request.setAttribute("lineGraphValuesJson", new Gson().toJson(lineGraphValues));
        request.setAttribute("labelsListJson", new Gson().toJson(labels));
        request.setAttribute("profilingBuildIdsJson", new Gson().toJson(profilingBuildIds));
        request.setAttribute("performanceLabelX", new Gson().toJson(performanceLabelX));
        request.setAttribute("performanceLabelY", new Gson().toJson(performanceLabelY));

        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("startTime", new Gson().toJson(startTime));
        request.setAttribute("endTime", new Gson().toJson(endTime));

        request.setAttribute("profilingPointName", profilingPointName);
        request.setAttribute("percentileValuesJson", new Gson().toJson(percentileValuesArray));
        response.setContentType("text/plain");
        dispatcher = request.getRequestDispatcher("/show_graph.jsp");
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.error("Servlet Excpetion caught : ", e);
        }
    }
}
