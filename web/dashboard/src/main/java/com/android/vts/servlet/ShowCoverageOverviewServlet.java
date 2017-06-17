/*
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
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

import com.android.vts.entity.TestEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.proto.VtsReportMessage;
import com.android.vts.util.TestRunMetadata;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.FetchOptions;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.Query;
import com.google.gson.Gson;
import com.google.gson.JsonObject;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Represents the servlet that is invoked on loading the coverage overview page. */
public class ShowCoverageOverviewServlet extends BaseServlet {
    private static final String COVERAGE_OVERVIEW_JSP = "WEB-INF/jsp/show_coverage_overview.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.COVERAGE_OVERVIEW;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        return null;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        Query q = new Query(TestEntity.KIND).setKeysOnly();
        List<Key> allTests = new ArrayList<>();
        for (Entity test : datastore.prepare(q).asIterable()) {
            allTests.add(test.getKey());
        }

        // Add test names to list
        List<String> resultNames = new ArrayList<>();
        for (VtsReportMessage.TestCaseResult r : VtsReportMessage.TestCaseResult.values()) {
            resultNames.add(r.name());
        }

        List<JsonObject> testRunObjects = new ArrayList<>();

        Query.Filter coverageFilter = new Query.FilterPredicate(
                TestRunEntity.HAS_COVERAGE, Query.FilterOperator.EQUAL, true);
        int coveredLines = 0;
        int uncoveredLines = 0;
        int passCount = 0;
        int failCount = 0;
        for (Key key : allTests) {
            Query testRunQuery =
                    new Query(TestRunEntity.KIND)
                            .setAncestor(key)
                            .setFilter(coverageFilter)
                            .addSort(Entity.KEY_RESERVED_PROPERTY, Query.SortDirection.DESCENDING);
            for (Entity testRunEntity :
                    datastore.prepare(testRunQuery).asIterable(FetchOptions.Builder.withLimit(1))) {
                TestRunEntity testRun = TestRunEntity.fromEntity(testRunEntity);
                if (testRun == null)
                    continue;
                TestRunMetadata metadata = new TestRunMetadata(key.getName(), testRun);
                testRunObjects.add(metadata.toJson());
                coveredLines += testRun.coveredLineCount;
                uncoveredLines += testRun.totalLineCount - testRun.coveredLineCount;
                passCount += testRun.passCount;
                failCount += testRun.failCount;
            }
        }

        int[] testStats = new int[VtsReportMessage.TestCaseResult.values().length];
        testStats[VtsReportMessage.TestCaseResult.TEST_CASE_RESULT_PASS.getNumber()] = passCount;
        testStats[VtsReportMessage.TestCaseResult.TEST_CASE_RESULT_FAIL.getNumber()] = failCount;

        response.setStatus(HttpServletResponse.SC_OK);
        request.setAttribute("resultNames", resultNames);
        request.setAttribute("resultNamesJson", new Gson().toJson(resultNames));
        request.setAttribute("testRuns", new Gson().toJson(testRunObjects));
        request.setAttribute("coveredLines", new Gson().toJson(coveredLines));
        request.setAttribute("uncoveredLines", new Gson().toJson(uncoveredLines));
        request.setAttribute("testStats", new Gson().toJson(testStats));
        dispatcher = request.getRequestDispatcher(COVERAGE_OVERVIEW_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
        }
    }
}
