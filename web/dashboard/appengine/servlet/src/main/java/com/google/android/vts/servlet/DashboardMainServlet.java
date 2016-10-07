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

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import org.apache.hadoop.hbase.HTableDescriptor;
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
 * Represents the servlet that is invoked on loading the first page of dashboard.
 */
@WebServlet(name = "dashboard_main", urlPatterns = {"/"})
public class DashboardMainServlet extends HttpServlet {

    private static final String DASHBOARD_MAIN_JSP = "/dashboard_main.jsp";
    private static final Logger logger = LoggerFactory.getLogger(DashboardMainServlet.class);


    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        RequestDispatcher dispatcher = null;

        if (currentUser != null) {
            response.setContentType("text/plain");
            HTableDescriptor[] tables = BigtableHelper.getTables();
            List<String> testList = new ArrayList<>();

            // filter tables not starting with 'result'
            for (HTableDescriptor table : tables) {
                String tableName = table.getNameAsString();
                if (tableName.startsWith("result_")) {
                    testList.add(tableName.substring(7));  // cut the prefix
                }
            }

            String[] testArray = new String[testList.size()];
            testList.toArray(testArray);
            request.setAttribute("testNames", testArray);
            dispatcher = request.getRequestDispatcher(DASHBOARD_MAIN_JSP);
            try {
                dispatcher.forward(request, response);
            } catch (ServletException e) {
                logger.info("Servlet Excpetion caught : ", e);
            }
        } else {
            response.sendRedirect(userService.createLoginURL(request.getRequestURI()));
        }
    }
}
