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

import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import com.android.vts.proto.VtsReportMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.util.BigtableHelper;
import com.android.vts.util.EmailHelper;
import com.android.vts.util.ProfilingPointSummary;
import com.android.vts.util.StatSummary;
import com.google.protobuf.ByteString;
import java.io.IOException;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Represents the notifications service which is automatically called on a fixed schedule.
 */
public class VtsPerformanceJobServlet extends BaseServlet {

    private static final String STATUS_TABLE = "vts_status_table";
    private static final byte[] RESULTS_FAMILY = Bytes.toBytes("test");
    private static final int N_DIGITS = 2;
    private static final byte[] DATA_QUALIFIER = Bytes.toBytes("data");
    private static final long MILLI_TO_MICRO = 1000;  // conversion factor from milli to micro units

    private static final String MEAN = "Mean";
    private static final String STD = "Std";
    private static final String DIFFERENCE = "Difference";
    private static final String SUBJECT_PREFIX = "Daily Performance Digest: ";
    private static final String LABEL_STYLE = "font-family: arial";
    private static final String TABLE_STYLE = "border-collapse: collapse; border: 1px solid black; font-size: 12px; font-family: arial;";
    private static final String SECTION_LABEL_STYLE = "border: 1px solid black; border-bottom: none; background-color: lightgray;";
    private static final String COL_LABEL_STYLE = "border: 1px solid black; border-bottom-width: 2px; border-top: 1px dotted gray; background-color: lightgray;";
    private static final String HEADER_COL_STYLE = "border-top: 1px dotted gray; border-right: 2px solid black; text-align: right; background-color: lightgray;";
    private static final String INNER_CELL_STYLE = "border-top: 1px dotted gray; border-right: 1px dotted gray; text-align: right;";
    private static final String OUTER_CELL_STYLE = "border-top: 1px dotted gray; border-right: 2px solid black; text-align: right;";

    /**
     * Produces a map with summaries for each profiling point present in the specified table.
     * @param tableName The name of the table whose profiling vectors to retrieve.
     * @param startTime The (inclusive) start time in microseconds to scan from.
     * @param endTime The (exclusive) end time in microseconds at which to stop scanning.
     * @returns A map with profiling point name keys and ProfilingPointSummary objects as values.
     * @throws IOException
     */
    private static Map<String, ProfilingPointSummary> getProfilingSummaryMap(
            String tableName, long startTime, long endTime) throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        Scan scan = new Scan();
        scan.setStartRow(Bytes.toBytes(Long.toString(startTime)));
        scan.setStopRow(Bytes.toBytes(Long.toString(endTime)));
        ResultScanner scanner = table.getScanner(scan);

        Map<String, ProfilingPointSummary> profilingSummaryMap = new HashMap<>();

        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(RESULTS_FAMILY, DATA_QUALIFIER);
            TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage
                                                                  .parseFrom(value);
            String buildId = testReportMessage.getBuildInfo().getId().toStringUtf8();

            // filter empty build IDs and add only numbers
            if (buildId.length() == 0) continue;

            // filter empty device info lists
            if (testReportMessage.getDeviceInfoList().size() == 0) continue;

            String firstDeviceBuildId = testReportMessage.getDeviceInfoList().get(0)
                                        .getBuildId().toStringUtf8();

            try {
                // filter non-integer build IDs
                Integer.parseInt(buildId);
                Integer.parseInt(firstDeviceBuildId);
            } catch (NumberFormatException e) {
                /* skip a non-post-submit build */
                continue;
            }
            for (ProfilingReportMessage profilingReportMessage :
                testReportMessage.getProfilingList()) {
                String name = profilingReportMessage.getName().toStringUtf8();
                switch(profilingReportMessage.getType()) {
                    case UNKNOWN_VTS_PROFILING_TYPE:
                    case VTS_PROFILING_TYPE_TIMESTAMP :
                        break;
                    case VTS_PROFILING_TYPE_LABELED_VECTOR :
                        if (profilingReportMessage.getLabelList().size() == 0 ||
                            profilingReportMessage.getLabelList().size() !=
                            profilingReportMessage.getValueList().size()) {
                            continue;
                        }
                        if (!profilingSummaryMap.containsKey(name)) {
                            profilingSummaryMap.put(name, new ProfilingPointSummary());
                        }
                        profilingSummaryMap.get(name).update(profilingReportMessage);
                        break;
                    default :
                        break;
                }
            }
        }
        return profilingSummaryMap;
    }

    /**
     * Rounds a double to a fixed number of digits.
     * @param value The number to be rounded.
     * @param digits The number of decimal places of accuracy.
     * @returns The string representation of 'value' rounded to 'digits' number of decimal places.
     */
    private static String round(double value, int digits) {
        String format = "#." + new String(new char[digits]).replace("\0", "#");
        DecimalFormat formatter = new DecimalFormat(format);
        formatter.setRoundingMode(RoundingMode.CEILING);
        return formatter.format(value);
    }

    /**
     * Generates an HTML summary of the performance changes for the profiling results in the
     * specified table.
     *
     * Retrieves the past 24 hours of profiling data and compares it to the 24 hours that preceded
     * it. Creates a table representation of the mean and standard deviation for each profiling
     * point. When performance degrades, the cell is shaded red.
     *
     * @param tableName The name of the table whose profiling data to summarize.
     * @returns An HTML string containing labeled table summaries.
     * @throws IOException
     */
    private static String getPeformanceSummary(String tableName) throws IOException {
        long now = System.currentTimeMillis() * MILLI_TO_MICRO;
        String dateString = new SimpleDateFormat("MM-dd-yyyy").format(new Date());
        String dateStringOld = new SimpleDateFormat("MM-dd-yyyy").format(
                new Date(System.currentTimeMillis() - ONE_DAY/MILLI_TO_MICRO));
        Map<String, ProfilingPointSummary> summaryMap = getProfilingSummaryMap(
                tableName, now - ONE_DAY, now);
        if (summaryMap == null || summaryMap.size() == 0) return null;
        Map<String, ProfilingPointSummary> summaryMapOld = getProfilingSummaryMap(
                tableName, now - ONE_DAY * 2, now - ONE_DAY);

        String tableHTML = "<p style='" + LABEL_STYLE + "'><b>";
        tableHTML += tableName.substring(TABLE_PREFIX.length()) + "</b></p>";
        for (String profilingPoint : summaryMap.keySet()) {
            ProfilingPointSummary summary = summaryMap.get(profilingPoint);
            tableHTML += "<table cellpadding='2' style='" + TABLE_STYLE + "'>";

            // Format header rows
            String[] headerRows = new String[]{profilingPoint, summary.yLabel.toStringUtf8()};
            for (String content : headerRows) {
                tableHTML += "<tr><td colspan='7'>" + content + "</td></tr>";
            }

            // Format section labels
            tableHTML += "<tr>";
            String[] sectionLabels = new String[]{"", dateString, dateStringOld, DIFFERENCE};
            for (String content : sectionLabels) {
                tableHTML += "<th style='" + SECTION_LABEL_STYLE + "' ";
                if (content != "") {
                    tableHTML += "colspan='2'";
                }
                tableHTML += ">" + content + "</th>";
            }
            tableHTML += "</tr>";

            // Format column labels
            tableHTML += "<tr>";
            String[] labelCells = new String[]{summary.xLabel.toStringUtf8(), MEAN, STD,
                                               MEAN, STD, MEAN, STD};
            for (String content : labelCells) {
                tableHTML += "<th style='" + COL_LABEL_STYLE + "'>" + content + "</th>";
            }
            tableHTML += "</tr>";

            // Populate data cells
            for (StatSummary stats : summary) {
                ByteString label = stats.getLabel();
                tableHTML += "<tr><td style='" + HEADER_COL_STYLE +"'>" + label.toStringUtf8();
                tableHTML += "</td><td style='" + INNER_CELL_STYLE + "'>";
                tableHTML += round(stats.getMean(), N_DIGITS)  + "</td>";
                tableHTML += "<td style='" + OUTER_CELL_STYLE + "'>";
                tableHTML += round(stats.getStd(), N_DIGITS) + "</td>";
                if (!summaryMapOld.containsKey(profilingPoint) ||
                    !summaryMapOld.get(profilingPoint).hasLabel(label)) {
                    tableHTML += "<td></td><td></td><td></td><td></td></tr>";
                    continue;
                }
                ProfilingPointSummary summaryOld = summaryMapOld.get(profilingPoint);
                StatSummary statsOld = summaryOld.getStatReport(label);
                tableHTML += "<td style='" + INNER_CELL_STYLE + "'>";
                tableHTML += round(statsOld.getMean(), N_DIGITS)  + "</td>";
                tableHTML += "<td style='" + OUTER_CELL_STYLE + "'>";
                tableHTML += round(statsOld.getStd(), N_DIGITS) + "</td>";
                // Intensity of red color is a function of the relative (percent) change
                // in the new value compared to the previous day's. Intensity is a linear function
                // of percentage change, reaching a ceiling at 100% change (e.g. a doubling).
                double alpha = Math.max(0, (stats.getMean() - statsOld.getMean()) /
                                   statsOld.getMean());
                String color = "background-color: rgba(255, 0, 0, " + alpha + ");";
                tableHTML += "<td style='" + INNER_CELL_STYLE + color + "'>";
                tableHTML += round(stats.getMean() - statsOld.getMean(), N_DIGITS) + "</td>";

                alpha = Math.max(0, (stats.getStd() - statsOld.getStd()) / statsOld.getStd());
                color = "background-color: rgba(255, 0, 0, " + alpha + ");";
                tableHTML += "<td style='" + OUTER_CELL_STYLE + color + "'>";
                tableHTML += round(stats.getStd() - statsOld.getStd(), N_DIGITS) + "</td></tr>";
            }
            tableHTML += "</table><br>";
        }
        return tableHTML;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        doGetHandler(request, response);
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        Table statusTable = BigtableHelper.getTable(TableName.valueOf(STATUS_TABLE));
        HTableDescriptor[] tables = BigtableHelper.getTables();
        Set<String> allTables = new HashSet<>();

        for (HTableDescriptor descriptor : tables) {
            String tableName = descriptor.getNameAsString();
            if (tableName.startsWith(TABLE_PREFIX)) {
                allTables.add(tableName);
            }
        }
        for (String tableName : allTables) {
            String body = getPeformanceSummary(tableName);
            if (body == null || body.equals("")) continue;
            List<String> emails = EmailHelper.getSubscriberEmails(statusTable, tableName);
            if (emails.size() == 0) continue;
            String subject = SUBJECT_PREFIX + tableName.substring(TABLE_PREFIX.length());
            EmailHelper.send(emails, subject, body);
        }
    }
}
