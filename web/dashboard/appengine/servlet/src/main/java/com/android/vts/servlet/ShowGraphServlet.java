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

import com.android.vts.proto.VtsReportMessage;
import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.util.BigtableHelper;
import com.android.vts.util.Graph;
import com.android.vts.util.GraphSerializer;
import com.android.vts.util.Histogram;
import com.android.vts.util.LineGraph;
import com.android.vts.util.PerformanceUtil;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;


/**
 * Servlet for handling requests to load graphs.
 */
public class ShowGraphServlet extends BaseServlet {

    private static final String GRAPH_JSP = "WEB-INF/jsp/show_graph.jsp";
    private static final byte[] FAMILY = Bytes.toBytes("test");
    private static final byte[] QUALIFIER = Bytes.toBytes("data");

    private static final String HIDL_HAL_OPTION = "hidl_hal_mode";
    private static final String[] splitKeysArray = new String[]{HIDL_HAL_OPTION};
    private static final Set<String> splitKeySet =
            new HashSet<String>(Arrays.asList(splitKeysArray));
    private static final String PROFILING_DATA_ALERT = "No profiling data was found.";

    @Override
    public List<String[]> getNavbarLinks(HttpServletRequest request) {
        List<String[]> links = new ArrayList<>();
        Page root = Page.HOME;
        String[] rootEntry = new String[]{root.getUrl(), root.getName()};
        links.add(rootEntry);

        Page table = Page.TABLE;
        String testName = request.getParameter("testName");
        String name = table.getName() + testName;
        String url = table.getUrl() + "?testName=" + testName;
        String[] tableEntry = new String[]{url, name};
        links.add(tableEntry);

        Page graph = Page.GRAPH;
        String profilingPointName = request.getParameter("profilingPoint");
        url = graph.getUrl() + "?testName=" + testName + "&profilingPoint=" + profilingPointName;
        String[] graphEntry = new String[]{url, graph.getName()};
        links.add(graphEntry);
        return links;
    }

    /**
     * Process a profiling report message and determine which line graph to insert the point into.
     * @param profilingReportMessage The profiling report to process.
     * @param idString The ID derived from the test run to identify the profiling report.
     * @param graphMap A map from graph name to Graph object.
     */
    private static void processLineGraphReport(
            ProfilingReportMessage profilingReportMessage, String idString,
            Map<String, Graph> graphMap) {
        if (profilingReportMessage.getLabelList().size() == 0 ||
            profilingReportMessage.getLabelList().size() !=
            profilingReportMessage.getValueList().size()) return;
        String name = PerformanceUtil.getOptionKeys(profilingReportMessage.getOptionsList(),
                                                    splitKeySet);
        if (!graphMap.containsKey(name)) {
            graphMap.put(name, new LineGraph(name));
        }
        graphMap.get(name).addData(idString, profilingReportMessage);
    }

    /**
     * Process a profiling report message and determine which histogram to insert the point into.
     * @param profilingReportMessage The profiling report to process.
     * @param graphMap A map from graph name to Graph object.
     */
    private static void processHistogramReport(
            ProfilingReportMessage profilingReportMessage, String idString,
            Map<String, Graph> graphMap) {
        if (profilingReportMessage.getValueList().size() == 0 &&
            profilingReportMessage.getStartTimestamp() >= profilingReportMessage.getEndTimestamp())
            return;
        String name = PerformanceUtil.getOptionKeys(
                profilingReportMessage.getOptionsList(), splitKeySet);
        if (!graphMap.containsKey(name)) {
            graphMap.put(name, new Histogram(name));
        }
        graphMap.get(name).addData(idString, profilingReportMessage);
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        Table table = null;
        TableName tableName = null;

        String profilingPointName = request.getParameter("profilingPoint");
        String selectedDevice = request.getParameter("device");
        Long startTime = null;
        if (request.getParameter("startTime") != null) {
            String time = request.getParameter("startTime");
            try {
                startTime = Long.parseLong(time);
            } catch (NumberFormatException e) {}
        }
        if (startTime == null) {
            startTime = System.currentTimeMillis() * MILLI_TO_MICRO;
        }
        tableName = TableName.valueOf(TABLE_PREFIX + request.getParameter("testName"));
        table = BigtableHelper.getTable(tableName);

        // Set of device names
        Set<String> deviceSet = new HashSet<String>();

        Map<String, Graph> graphMap = new HashMap<>();

        Scan scan = new Scan();
        scan.setStartRow(Long.toString(startTime - ONE_DAY).getBytes());
        scan.setStopRow(Long.toString(startTime).getBytes());
        ResultScanner scanner = table.getScanner(scan);
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(FAMILY, QUALIFIER);
            TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage.
                parseFrom(value);

            AndroidDeviceInfoMessage firstDeviceInfo = testReportMessage.getDeviceInfoList().get(0);
            String firstDeviceBuildId = firstDeviceInfo.getBuildId().toStringUtf8();
            String firstDeviceType =
                    firstDeviceInfo.getProductVariant().toStringUtf8().toLowerCase();
            String idString = firstDeviceType + " (" + firstDeviceBuildId + ")";
            deviceSet.add(firstDeviceType);
            if (selectedDevice != null && !firstDeviceType.equals(selectedDevice)) continue;

            // update map of profiling point names
            for (ProfilingReportMessage profilingReportMessage :
                testReportMessage.getProfilingList()) {
                if (!profilingPointName.equals(profilingReportMessage.getName().toStringUtf8())) {
                    continue;
                }
                switch(profilingReportMessage.getType()) {
                    case UNKNOWN_VTS_PROFILING_TYPE:
                    case VTS_PROFILING_TYPE_TIMESTAMP:
                        processHistogramReport(profilingReportMessage, idString, graphMap);
                        break;
                    case VTS_PROFILING_TYPE_LABELED_VECTOR:
                        processLineGraphReport(profilingReportMessage, idString, graphMap);
                        break;
                    case VTS_PROFILING_TYPE_UNLABELED_VECTOR:
                        processHistogramReport(profilingReportMessage, idString, graphMap);
                        break;
                    default :
                        break;
                }
            }
        }
        String[] names = graphMap.keySet().toArray(new String[graphMap.size()]);
        Arrays.sort(names);

        List<Graph> graphList = new ArrayList<>();
        boolean hasHistogram = false;
        for (String name : names) {
            Graph g = graphMap.get(name);
            if (g.size() > 0) {
                graphList.add(g);
                if (g instanceof Histogram) hasHistogram = true;
            }
        }

        // sort devices list
        if (!deviceSet.contains(selectedDevice)) selectedDevice = null;
        String[] devices = deviceSet.toArray(new String[deviceSet.size()]);
        Arrays.sort(devices);

        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("filterVal", request.getParameter("filterVal"));
        request.setAttribute("startTime", new Gson().toJson(startTime));
        request.setAttribute("devices", devices);
        request.setAttribute("selectedDevice", selectedDevice);
        request.setAttribute("showFilterDropdown", hasHistogram);
        if (graphList.size() == 0) request.setAttribute("error", PROFILING_DATA_ALERT);

        Gson gson = new GsonBuilder().registerTypeHierarchyAdapter(
                Graph.class, new GraphSerializer()).create();
        request.setAttribute("graphs", gson.toJson(graphList));

        request.setAttribute("profilingPointName", profilingPointName);
        dispatcher = request.getRequestDispatcher(GRAPH_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Excpetion caught : ", e);
        }
    }
}
