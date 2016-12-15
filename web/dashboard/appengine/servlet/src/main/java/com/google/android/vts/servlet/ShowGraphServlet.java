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
import com.google.android.vts.proto.VtsReportMessage.TestReportMessage;

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import org.apache.commons.math3.stat.descriptive.rank.Percentile;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import com.google.gson.Gson;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to load graphs.
 */
@WebServlet(name = "show_graph", urlPatterns = {"/show_graph"})
public class ShowGraphServlet extends HttpServlet {

    private static final Logger logger = LoggerFactory.getLogger(ShowGraphServlet.class);
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";

    /**
     * Returns the table corresponding to the table name.
     * @param tableName Describes the table name which is passed as a parameter from
     *        dashboard_main.jsp, which represents the table to fetch data from.
     * @return table An instance of org.apache.hadoop.hbase.client.Table
     * @throws IOException
     */
    private Table getTable(TableName tableName) throws IOException {
        long result;
        Table table = null;

        try {
            table = BigtableHelper.getConnection().getTable(tableName);
        } catch (IOException e) {
            logger.error("Exception occurred in com.google.android.vts.servlet.ShowGraphServlet."
              + "getTable()", e);
            return null;
        }
        return table;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;

        String profilingPointName = request.getParameter("profilingPoint");
        tableName = TableName.valueOf(request.getParameter("tableName"));
        table = getTable(tableName);

        // This list holds the values for all profiling points.
        List<Double> profilingPointValuesList = new ArrayList<Double>();

        ResultScanner scanner = table.getScanner(new Scan());
        for (Result result = scanner.next(); (result != null); result = scanner.next()) {
            for (KeyValue keyValue : result.list()) {
                TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                    parseFrom(keyValue.getValue());

                // update map of profiling point names
                for (ProfilingReportMessage profilingReportMessage :
                    testReportMessage.getProfilingList()) {
                    if (! profilingPointName.equals(profilingReportMessage.getName().toStringUtf8())) {
                        continue;
                    }

                    double timeTaken = ((double)(profilingReportMessage.getEndTimestamp() -
                        profilingReportMessage.getStartTimestamp())) / 1000;
                    if (timeTaken < 0) {
                        logger.info("Inappropriate value for time taken");
                        continue;
                    }
                    profilingPointValuesList.add(timeTaken);
                }
            }
        }

        double[] values = new double[profilingPointValuesList.size()];
        for (int i = 0; i < profilingPointValuesList.size(); i++) {
            values[i] = profilingPointValuesList.get(i).doubleValue();
        }

        // pass map to show_graph.jsp through request by converting to JSON
        String valuesJson = new Gson().toJson(values);
        request.setAttribute("valuesJson", valuesJson);

        int[] percentiles = {10, 25, 50 , 75, 80, 90, 95, 99};
        double[] percentileValuesArray = new double[percentiles.length];
        for (int i = 0; i < percentiles.length; i++) {
            percentileValuesArray[i] =
                Math.round(new Percentile().evaluate(values, percentiles[i]) * 1000d) / 1000d;
        }

        if (values.length == 0) {
            request.setAttribute("error", PROFILING_DATA_ALERT);
        }
        request.setAttribute("profilingPointName", profilingPointName);
        request.setAttribute("values", values);
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
