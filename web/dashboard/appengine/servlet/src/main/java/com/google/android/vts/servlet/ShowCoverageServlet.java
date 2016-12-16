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

import com.google.android.vts.helpers.BigtableHelper;
import com.google.android.vts.proto.VtsReportMessage;
import com.google.android.vts.proto.VtsReportMessage.CoverageReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
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
import java.util.List;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to show code coverage.
 */
@WebServlet(name = "show_coverage", urlPatterns = {"/show_coverage"})
public class ShowCoverageServlet extends HttpServlet {

    private static final byte[] FAMILY = Bytes.toBytes("test");
    private static final byte[] QUALIFIER = Bytes.toBytes("data");
    private static final String TABLE_PREFIX = "result_";
    private static final String GERRIT_URI = System.getenv("GERRIT_URI");
    private static final String GERRIT_SCOPE = System.getenv("GERRIT_SCOPE");
    private static final String CLIENT_ID = System.getenv("CLIENT_ID");
    private static final Logger logger = LoggerFactory.getLogger(ShowCoverageServlet.class);

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        String loginURI = userService.createLoginURL(request.getRequestURI());
        String logoutURI = userService.createLogoutURL(loginURI);
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;
        tableName = TableName.valueOf(TABLE_PREFIX + request.getParameter("testName"));

        // key is a unique combination of build Id and timestamp that helps identify the
        // corresponding build id.
        String key = request.getParameter("key");
        Scan scan = new Scan();
        long time = -1;
        try {
            time = Long.parseLong(key);
            scan.setStartRow(key.getBytes());
            scan.setStopRow(Long.toString((time + 1)).getBytes());
        } catch (NumberFormatException e) { }  // Use unbounded scan

        TestReportMessage testReportMessage = null;

        table = BigtableHelper.getTable(tableName);
        ResultScanner scanner = table.getScanner(scan);
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(FAMILY, QUALIFIER);
            TestReportMessage currentTestReportMessage = VtsReportMessage.TestReportMessage.
                parseFrom(value);
            String buildId = currentTestReportMessage.getBuildInfo().getId().toStringUtf8();
            String firstDeviceBuildId = currentTestReportMessage.getDeviceInfoList().get(0)
                                        .getBuildId().toStringUtf8();

            // filter empty build IDs and add only numbers
            if (buildId.length() > 0) {
                try {
                    Integer.parseInt(buildId);
                    Integer.parseInt(firstDeviceBuildId);
                    if (time == currentTestReportMessage.getStartTimestamp()) {
                      testReportMessage = currentTestReportMessage;
                      break;
                    }
                } catch (NumberFormatException e) {
                    /* skip a non-post-submit build */
                }
            }
        }
        scanner.close();

        List<String> sourceFiles = new ArrayList<>();
        List<List<Integer>> coverageVectors = new ArrayList<>();
        List<String> testcaseNames = new ArrayList<>();
        List<String> projects = new ArrayList<>();
        List<String> commits = new ArrayList<>();
        if (testReportMessage != null) {
            for (TestCaseReportMessage testCaseReportMessage : testReportMessage.getTestCaseList()) {
                if (!testCaseReportMessage.hasName()) continue;
                for (CoverageReportMessage coverageReportMessage :
                     testCaseReportMessage.getCoverageList()) {
                    if (coverageReportMessage.getLineCoverageVectorCount() == 0 ||
                        !coverageReportMessage.hasFilePath() ||
                        !coverageReportMessage.hasProjectName() ||
                        !coverageReportMessage.hasRevision()) {
                        continue;
                    }
                    coverageVectors.add(coverageReportMessage.getLineCoverageVectorList());
                    testcaseNames.add(testCaseReportMessage.getName().toStringUtf8());
                    sourceFiles.add(coverageReportMessage.getFilePath().toStringUtf8());
                    projects.add(coverageReportMessage.getProjectName().toStringUtf8());
                    commits.add(coverageReportMessage.getRevision().toStringUtf8());
                }
            }
        }

        request.setAttribute("email", currentUser.getEmail());
        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("gerritURI", new Gson().toJson(GERRIT_URI));
        request.setAttribute("gerritScope", new Gson().toJson(GERRIT_SCOPE));
        request.setAttribute("clientId", new Gson().toJson(CLIENT_ID));
        request.setAttribute("coverageVectors", new Gson().toJson(coverageVectors));
        request.setAttribute("testcaseNames", new Gson().toJson(testcaseNames));
        request.setAttribute("sourceFiles", new Gson().toJson(sourceFiles));
        request.setAttribute("projects", new Gson().toJson(projects));
        request.setAttribute("commits", new Gson().toJson(commits));
        request.setAttribute("startTime", request.getParameter("startTime"));
        request.setAttribute("endTime", request.getParameter("endTime"));
        response.setContentType("text/plain");
        dispatcher = request.getRequestDispatcher("/show_coverage.jsp");

        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.error("Servlet Excpetion caught : ", e);
        }
    }
}
