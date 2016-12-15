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
import com.google.android.vts.proto.VtsReportMessage.CoverageReportMessage;
import com.google.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;

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
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpSession;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to show code coverage.
 */
@WebServlet(name = "show_coverage", urlPatterns = {"/show_coverage"})
public class ShowCoverageServlet extends HttpServlet {

    private static final Logger logger = LoggerFactory.getLogger(ShowCoverageServlet.class);
    private static final int COVERAGE_GRID_COLUMNS = 4;

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
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;
        tableName = TableName.valueOf(request.getParameter("tableName"));

        // key is a unique combination of build Id and timestamp that helps identify the
        // corresponding build id.
        String key = request.getParameter("key");
        TestReportMessage testReportMessage = null;
        HttpSession session = request.getSession();

        if (currentUser != null) {
            table = getTable(tableName);

            ResultScanner scanner = table.getScanner(new Scan());
            outerloop:
            for (Result result = scanner.next(); (result != null); result = scanner.next()) {
                for (KeyValue keyValue : result.list()) {
                    TestReportMessage currentTestReportMessage = VtsReportMessage.TestReportMessage.
                        parseFrom(keyValue.getValue());
                    String buildId = currentTestReportMessage.getBuildInfo().getId().toStringUtf8();

                    // filter empty build IDs and add only numbers
                    if (buildId.length() > 0) {
                        try {
                            Integer.parseInt(buildId);
                            String currentKey = currentTestReportMessage.getBuildInfo().getId().toStringUtf8() +
                                    "." + String.valueOf(currentTestReportMessage.getStartTimestamp());

                            if (key.equals(currentKey)) {
                              testReportMessage = currentTestReportMessage;
                              break outerloop;
                            }
                        } catch (NumberFormatException e) {
                            /* skip a non-post-submit build */
                        }
                    }
                }
            }

            int rowCount = 0;
            for (TestCaseReportMessage testCaseReportMessage : testReportMessage.getTestCaseList()) {
                for (CoverageReportMessage coverageReportMessage : testCaseReportMessage.getCoverageList()) {
                    rowCount++;
                }
            }

            String[][] coverageGrid = new String[rowCount][COVERAGE_GRID_COLUMNS];
            int rowIndex = 0;
            for (TestCaseReportMessage testCaseReportMessage : testReportMessage.getTestCaseList()) {
                for (CoverageReportMessage coverageReportMessage : testCaseReportMessage.getCoverageList()) {
                    coverageGrid[rowIndex][0] = testCaseReportMessage.getName().toStringUtf8();
                    coverageGrid[rowIndex][1] = coverageReportMessage.getDirPath().toStringUtf8();
                    coverageGrid[rowIndex][2] = coverageReportMessage.getFileName().toStringUtf8();
                    coverageGrid[rowIndex][3] = coverageReportMessage.getHtml().toStringUtf8();
                    rowIndex++;
                }
            }

            String[] coverageGridColumns =
                {"Test Case Report Message", "Directory Path", "File Name", "HTML"};

            request.setAttribute("coverageGridJson", new Gson().toJson(coverageGrid));
            request.setAttribute("coverageGridColumnsJson", new Gson().toJson(coverageGridColumns));
            response.setContentType("text/plain");
            dispatcher = request.getRequestDispatcher("/show_coverage.jsp");

            try {
                dispatcher.forward(request, response);
            } catch (ServletException e) {
                logger.error("Servlet Excpetion caught : ", e);
            }
        }
        else {
          response.sendRedirect(userService.createLoginURL(request.getRequestURI()));
        }
    }
}
