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
import org.apache.commons.math3.stat.descriptive.rank.Percentile;
import com.google.gson.Gson;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
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

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        UserService userService = UserServiceFactory.getUserService();
        User currentUser = userService.getCurrentUser();
        RequestDispatcher dispatcher = null;
        String profilingPointName = request.getParameter("profilingPoint");
        HttpSession session = request.getSession();

        // map to hold the the list of time taken for each test.
        Map<String, List<Double>> mapProfilingNameValues = ((Map<String, List<Double>>)
            session.getAttribute("mapProfilingNameValues"));

        double[] values = new double[mapProfilingNameValues.get(profilingPointName).size()];
        for (int i = 0; i < values.length; i++) {
            values[i] = mapProfilingNameValues.get(profilingPointName).get(i);
        }

        // pass map to show_graph.jsp through request by converting to JSON
        String valuesJson = new Gson().toJson(values);
        request.setAttribute("valuesJson", valuesJson);

        int[] percentiles = {10, 25, 50 , 75, 80, 90, 95, 99};
        double[] percentileResultArray = new double[percentiles.length];
        for (int i = 0; i < percentiles.length; i++) {
            percentileResultArray[i] =
                Math.round(new Percentile().evaluate(values, percentiles[i]) * 1000d) / 1000d;
        }

        if (values.length == 0) {
            request.setAttribute("error", PROFILING_DATA_ALERT);
        }
        request.setAttribute("profilingPointName", profilingPointName);
        request.setAttribute("values", values);
        request.setAttribute("percentileResultArray", percentileResultArray);
        response.setContentType("text/plain");
        dispatcher = request.getRequestDispatcher("/show_graph.jsp");
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.error("Servlet Excpetion caught : ", e);
        }
    }
}
