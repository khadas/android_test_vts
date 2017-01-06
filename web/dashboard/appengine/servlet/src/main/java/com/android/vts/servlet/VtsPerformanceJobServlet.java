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
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.BigtableHelper;
import com.android.vts.util.EmailHelper;
import com.android.vts.util.ProfilingPointSummary;
import com.android.vts.util.StatSummary;
import com.google.protobuf.ByteString;
import java.io.IOException;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
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
    private static final String MIN = "Min";
    private static final String MIN_DELTA = "&Delta;Min (%)";
    private static final String MAX_DELTA = "&Delta;Max (%)";
    private static final String HIGHER_IS_BETTER = "Note: Higher values are better. Maximum is the best-case performance.";
    private static final String LOWER_IS_BETTER = "Note: Lower values are better. Minimum is the best-case performance.";
    private static final String STD = "Std";
    private static final String SUBJECT_PREFIX = "Daily Performance Digest: ";
    private static final String LAST_WEEK = "Last Week";
    private static final String LABEL_STYLE = "font-family: arial";
    private static final String SUBTEXT_STYLE = "font-family: arial; font-size: 12px";
    private static final String TABLE_STYLE = "width: 100%; border-collapse: collapse; border: 1px solid black; font-size: 12px; font-family: arial;";
    private static final String SECTION_LABEL_STYLE = "border: 1px solid black; border-bottom: none; background-color: lightgray;";
    private static final String COL_LABEL_STYLE = "border: 1px solid black; border-bottom-width: 2px; border-top: 1px dotted gray; background-color: lightgray;";
    private static final String HEADER_COL_STYLE = "border-top: 1px dotted gray; border-right: 2px solid black; text-align: right; background-color: lightgray;";
    private static final String INNER_CELL_STYLE = "border-top: 1px dotted gray; border-right: 1px dotted gray; text-align: right;";
    private static final String OUTER_CELL_STYLE = "border-top: 1px dotted gray; border-right: 2px solid black; text-align: right;";

    private static class TimeInterval {
        public final long start;
        public final long end;
        public final String label;

        public TimeInterval(long start, long end, String label) {
            this.start = start;
            this.end = end;
            this.label = label;
        }
    }

    /**
     * Produces a map with summaries for each profiling point present in the specified table.
     * @param tableName The name of the table whose profiling vectors to retrieve.
     * @param startTime The (inclusive) start time in milliseconds to scan from.
     * @param endTime The (exclusive) end time in milliseconds at which to stop scanning.
     * @returns A map with profiling point name keys and ProfilingPointSummary objects as values.
     * @throws IOException
     */
    private static Map<String, ProfilingPointSummary> getProfilingSummaryMap(
            String tableName, long startTime, long endTime) throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        Scan scan = new Scan();
        scan.setStartRow(Bytes.toBytes(Long.toString(startTime * MILLI_TO_MICRO)));
        scan.setStopRow(Bytes.toBytes(Long.toString(endTime * MILLI_TO_MICRO)));
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
                switch(profilingReportMessage.getRegressionMode()) {
                    case VTS_REGRESSION_MODE_DISABLED:
                        continue;
                    default:
                        break;
                }
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
     * Creates the HTML for a table cell representing the percent change between two numbers.
     *
     * Computes the percent change (after - before)/before * 100 and inserts it into a table cell
     * with the specified style. The color of the cell is white if 'after' is less than before.
     * Otherwise, the cell is colored red with opacity according to the percent change (100%+
     * delta means 100% opacity). If the before value is 0 and the after value is positive, then
     * the color of the cell is 100% red to indicate an increase of undefined magnitude.
     *
     * @param baseline The baseline value observed.
     * @param test The value to compare against the baseline.
     * @param style A string containing CSS styles to apply to the table cell.
     * @returns An HTML string for a colored table cell containing the percent change.
     */
    private static String getPercentChangeHTML(double baseline, double test, String style,
                                               VtsProfilingRegressionMode mode) {
        String pctChangeString = "0 %";
        double alpha = 0;
        double delta = test - baseline;
        if (baseline != 0) {
            double pctChange = delta / baseline;
            alpha = pctChange * 2;
            pctChangeString = round(pctChange * 100, N_DIGITS) + " %";
        } else if (delta != 0){
            // If the percent change is undefined, the cell will be solid red or white
            alpha = (int) Math.signum(delta);  // get the sign of the delta (+1, 0, -1)
            pctChangeString = "";
        }
        if (mode == VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING) {
            alpha = -alpha;
        }
        String color = "background-color: rgba(255, 0, 0, " + alpha + ");";
        String html = "<td style='" + style + color + "'>";
        html += pctChangeString + "</td>";
        return html;
    }

    /**
     * Compares a test StatSummary to a baseline StatSummary and returns a formatted set of cells
     * @param baseline The StatSummary object containing initial values to compare against
     * @param test The StatSummary object containing test values to be compared against the baseline
     * @return HTML string representing the performance of the test versus the baseline
     */
    public static String getPerformanceComparisonHTML(StatSummary baseline, StatSummary test) {
        if (test == null || baseline == null) {
            return "<td></td><td></td><td></td><td></td>";
        }
        String row = "";
        double baselineValue;
        double testValue;
        switch (test.getRegressionMode()) {
            case VTS_REGRESSION_MODE_DECREASING:
                baselineValue = baseline.getMax();
                testValue = test.getMax();
                break;
            default:
                baselineValue = baseline.getMin();
                testValue = test.getMin();
                break;
        }
        // Intensity of red color is a function of the relative (percent) change
        // in the new value compared to the previous day's. Intensity is a linear function
        // of percentage change, reaching a ceiling at 100% change (e.g. a doubling).
        row += getPercentChangeHTML(baselineValue, testValue, INNER_CELL_STYLE,
                                    test.getRegressionMode());
        row += "<td style='" + INNER_CELL_STYLE + "'>" + round(baseline.getMin(), N_DIGITS);
        row += "</td><td style='" + INNER_CELL_STYLE + "'>" + round(baseline.getMean(), N_DIGITS);
        row += "</td><td style='" + OUTER_CELL_STYLE + "'>";
        row += round(baseline.getStd(), N_DIGITS) + "</td>";
        return row;
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
     * @param summaryMaps List of maps for each profiling run (in reverse chronological order)
     *                    containing profiling point names as keys and ProfilingPointSummary objects
     *                    as values.
     * @param labels List of string labels for use as the column headers.
     * @returns An HTML string containing labeled table summaries.
     */
    public static String getPeformanceSummary(String tableName,
            List<Map<String, ProfilingPointSummary>> summaryMaps, List<String> labels) {
        if (summaryMaps.size() == 0) return "";
        Map<String, ProfilingPointSummary> now = summaryMaps.get(0);
        String tableHTML = "<p style='" + LABEL_STYLE + "'><b>";
        tableHTML += tableName.substring(TABLE_PREFIX.length()) + "</b></p>";
        for (String profilingPoint : now.keySet()) {
            ProfilingPointSummary summary = now.get(profilingPoint);
            tableHTML += "<table cellpadding='2' style='" + TABLE_STYLE + "'>";

            // Format header rows
            String[] headerRows = new String[]{profilingPoint, summary.yLabel.toStringUtf8()};
            int colspan = labels.size() * 4;
            for (String content : headerRows) {
                tableHTML += "<tr><td colspan='" + colspan + "'>" + content + "</td></tr>";
            }

            // Format section labels
            tableHTML += "<tr>";
            for (int i = 0; i < labels.size(); i++) {
                String content = labels.get(i);
                tableHTML += "<th style='" + SECTION_LABEL_STYLE + "' ";
                if (i == 0) tableHTML += "colspan='1'";
                else if (i == 1) tableHTML += "colspan='3'";
                else tableHTML += "colspan='4'";
                tableHTML += ">" + content + "</th>";
            }
            tableHTML += "</tr>";

            String deltaString;
            String subtext;
            switch (now.get(profilingPoint).getRegressionMode()) {
                case VTS_REGRESSION_MODE_DECREASING:
                    deltaString = MAX_DELTA;
                    subtext = HIGHER_IS_BETTER;
                    break;
                default:
                    deltaString = MIN_DELTA;
                    subtext = LOWER_IS_BETTER;
                    break;
            }

            // Format column labels
            tableHTML += "<tr>";
            for (int i = 0; i < labels.size(); i++) {
                if (i > 1) {
                    tableHTML += "<th style='" + COL_LABEL_STYLE + "'>" + deltaString + "</th>";
                }
                if (i == 0) {
                    tableHTML += "<th style='" + COL_LABEL_STYLE + "'>";
                    tableHTML += summary.xLabel.toStringUtf8() + "</th>";
                } else if (i > 0) {
                    tableHTML += "<th style='" + COL_LABEL_STYLE + "'>" + MIN + "</th>";
                    tableHTML += "<th style='" + COL_LABEL_STYLE + "'>" + MEAN + "</th>";
                    tableHTML += "<th style='" + COL_LABEL_STYLE + "'>" + STD + "</th>";
                }
            }
            tableHTML += "</tr>";

            // Populate data cells
            for (StatSummary stats : summary) {
                ByteString label = stats.getLabel();
                tableHTML += "<tr><td style='" + HEADER_COL_STYLE +"'>" + label.toStringUtf8();
                tableHTML += "</td><td style='" + INNER_CELL_STYLE + "'>";
                tableHTML += round(stats.getMin(), N_DIGITS)  + "</td>";
                tableHTML += "<td style='" + INNER_CELL_STYLE + "'>";
                tableHTML += round(stats.getMean(), N_DIGITS)  + "</td>";
                tableHTML += "<td style='" + OUTER_CELL_STYLE + "'>";
                tableHTML += round(stats.getStd(), N_DIGITS) + "</td>";
                for (int i = 1; i < summaryMaps.size(); i++) {
                    Map<String, ProfilingPointSummary> summaryMapOld = summaryMaps.get(i);
                    if (summaryMapOld.containsKey(profilingPoint)) {
                        tableHTML += getPerformanceComparisonHTML(summaryMapOld.get(profilingPoint)
                                         .getStatSummary(label), stats);
                    } else tableHTML += "<td></td><td></td><td></td><td></td>";
                }
                tableHTML += "</tr>";
            }
            tableHTML += "</table>";
            tableHTML += "<i style='" + SUBTEXT_STYLE + "'>" + subtext + "</i><br><br>";
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

        // Add today to the list of time intervals to analyze
        List<TimeInterval> timeIntervals = new ArrayList<>();
        long now = System.currentTimeMillis();
        String dateString = new SimpleDateFormat("MM-dd-yyyy").format(new Date(now));
        TimeInterval today = new TimeInterval(now - ONE_DAY / MILLI_TO_MICRO, now, dateString);
        timeIntervals.add(today);

        // Add yesterday as a baseline time interval for analysis
        long oneDayAgo = now - ONE_DAY / MILLI_TO_MICRO;
        String dateStringYesterday = new SimpleDateFormat("MM-dd-yyyy").format(new Date(oneDayAgo));
        TimeInterval yesterday = new TimeInterval(oneDayAgo - ONE_DAY / MILLI_TO_MICRO, oneDayAgo,
                                                  dateStringYesterday);
        timeIntervals.add(yesterday);

        // Add last week as a baseline time interval for analysis
        long oneWeek = 7 * ONE_DAY / MILLI_TO_MICRO;
        long oneWeekAgo = now - oneWeek;
        TimeInterval lastWeek = new TimeInterval(oneWeekAgo - oneWeek, oneWeekAgo, LAST_WEEK);
        timeIntervals.add(lastWeek);

        for (String tableName : allTables) {
            List<Map<String, ProfilingPointSummary>> summaryMaps = new ArrayList<>();
            List<String> labels = new ArrayList<>();
            labels.add("");
            for (TimeInterval interval : timeIntervals) {
                Map<String, ProfilingPointSummary> summaryMap = getProfilingSummaryMap(
                        tableName, interval.start, interval.end);
                if (summaryMap == null || summaryMap.size() == 0) continue;
                summaryMaps.add(summaryMap);
                labels.add(interval.label);
            }
            String body = getPeformanceSummary(tableName, summaryMaps, labels);
            if (body == null || body.equals("")) continue;
            List<String> emails = EmailHelper.getSubscriberEmails(statusTable, tableName);
            if (emails.size() == 0) continue;
            String subject = SUBJECT_PREFIX + tableName.substring(TABLE_PREFIX.length());
            EmailHelper.send(emails, subject, body);
        }
    }
}
