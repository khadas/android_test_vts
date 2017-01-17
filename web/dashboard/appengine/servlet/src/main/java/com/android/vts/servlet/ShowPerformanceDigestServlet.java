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

import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.MathUtil;
import com.android.vts.util.PerformanceUtil;
import com.android.vts.util.PerformanceUtil.TimeInterval;
import com.android.vts.util.ProfilingPointSummary;
import com.android.vts.util.StatSummary;
import com.google.protobuf.ByteString;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Represents the notifications service which is automatically called on a fixed schedule.
 */
public class ShowPerformanceDigestServlet extends BaseServlet {

    private static final int N_DIGITS = 2;
    private static final long MILLI_TO_MICRO = 1000;  // conversion factor from milli to micro units

    private static final String MEAN = "Mean";
    private static final String MIN = "Min";
    private static final String MIN_DELTA = "&Delta;Min (%)";
    private static final String MAX_DELTA = "&Delta;Max (%)";
    private static final String HIGHER_IS_BETTER = "Note: Higher values are better. Maximum is the best-case performance.";
    private static final String LOWER_IS_BETTER = "Note: Lower values are better. Minimum is the best-case performance.";
    private static final String STD = "Std";
    private static final String LAST_WEEK = "Last Week";


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
     * @param classNames A string containing HTML classes to apply to the table cell.
     * @returns An HTML string for a colored table cell containing the percent change.
     */
    private static String getPercentChangeHTML(double baseline, double test, String classNames,
                                               VtsProfilingRegressionMode mode) {
        String pctChangeString = "0 %";
        double alpha = 0;
        double delta = test - baseline;
        if (baseline != 0) {
            double pctChange = delta / baseline;
            alpha = pctChange * 2;
            pctChangeString = MathUtil.round(pctChange * 100, N_DIGITS) + " %";
        } else if (delta != 0){
            // If the percent change is undefined, the cell will be solid red or white
            alpha = (int) Math.signum(delta);  // get the sign of the delta (+1, 0, -1)
            pctChangeString = "";
        }
        if (mode == VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING) {
            alpha = -alpha;
        }
        String color = "background-color: rgba(255, 0, 0, " + alpha + ");";
        String html = "<td class='" + classNames + "' style='" + color + "'>";
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
        row += getPercentChangeHTML(baselineValue, testValue, "cell inner-cell",
                                    test.getRegressionMode());
        row += "<td class='cell inner-cell'>";
        row += MathUtil.round(baseline.getMin(), N_DIGITS);
        row += "</td><td class='cell inner-cell'>";
        row += MathUtil.round(baseline.getMean(), N_DIGITS);
        row += "</td><td class='cell outer-cell'>";
        row += MathUtil.round(baseline.getStd(), N_DIGITS) + "</td>";
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
     * @param profilingPoint The name of the profiling point to summarize.
     * @param baseline The ProfilingPointSummary object to compare against.
     * @param summaryMaps List of maps for each profiling run (in reverse chronological order)
     *                    containing profiling point names as keys and ProfilingPointSummary objects
     *                    as values.
     * @param labels List of string labels for use as the column headers.
     * @returns An HTML string for a table comparing the profiling point results across time intervals.
     */
    public static String getPeformanceSummary(String profilingPoint, ProfilingPointSummary baseline,
            List<Map<String, ProfilingPointSummary>> summaryMaps, List<String> labels) {
        if (summaryMaps.size() == 0) return null;
        String tableHTML = "<table>";

        // Format section labels
        tableHTML += "<tr>";
        tableHTML += "<th class='section-label grey lighten-2'>";
        tableHTML += baseline.yLabel.toStringUtf8() + "</th>";
        for (int i = 0; i < labels.size(); i++) {
            String content = labels.get(i);
            tableHTML += "<th class='section-label grey lighten-2 center' ";
            if (i == 0) tableHTML += "colspan='3'";
            else tableHTML += "colspan='4'";
            tableHTML += ">" + content + "</th>";
        }
        tableHTML += "</tr>";

        String deltaString;
        switch (baseline.getRegressionMode()) {
            case VTS_REGRESSION_MODE_DECREASING:
                deltaString = MAX_DELTA;
                //subtext = HIGHER_IS_BETTER;
                break;
            default:
                deltaString = MIN_DELTA;
                //subtext = LOWER_IS_BETTER;
                break;
        }

        // Format column labels
        tableHTML += "<tr>";
        for (int i = 0; i < labels.size() + 1; i++) {
            if (i > 1) {
                tableHTML += "<th class='section-label grey lighten-2'>" + deltaString + "</th>";
            }
            if (i == 0) {
                tableHTML += "<th class='section-label grey lighten-2'>";
                tableHTML += baseline.xLabel.toStringUtf8() + "</th>";
            } else if (i > 0) {
                tableHTML += "<th class='section-label grey lighten-2'>" + MIN + "</th>";
                tableHTML += "<th class='section-label grey lighten-2'>" + MEAN + "</th>";
                tableHTML += "<th class='section-label grey lighten-2'>" + STD + "</th>";
            }
        }
        tableHTML += "</tr>";

        // Populate data cells
        for (StatSummary stats : baseline) {
            ByteString label = stats.getLabel();
            tableHTML += "<tr><td class='axis-label grey lighten-2'>" + label.toStringUtf8();
            tableHTML += "</td><td class='cell inner-cell'>";
            tableHTML += MathUtil.round(stats.getMin(), N_DIGITS)  + "</td>";
            tableHTML += "<td class='cell inner-cell'>";
            tableHTML += MathUtil.round(stats.getMean(), N_DIGITS)  + "</td>";
            tableHTML += "<td class='cell outer-cell'>";
            tableHTML += MathUtil.round(stats.getStd(), N_DIGITS) + "</td>";
            for (int i = 0; i < summaryMaps.size(); i++) {
                Map<String, ProfilingPointSummary> summaryMapOld = summaryMaps.get(i);
                if (summaryMapOld.containsKey(profilingPoint)) {
                    tableHTML += getPerformanceComparisonHTML(summaryMapOld.get(profilingPoint)
                                     .getStatSummary(label), stats);
                } else tableHTML += "<td></td><td></td><td></td><td></td>";
            }
            tableHTML += "</tr>";
        }
        tableHTML += "</table>";
        return tableHTML;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        String testName = request.getParameter("testName");
        String tableName = TABLE_PREFIX + testName;

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

        List<Map<String, ProfilingPointSummary>> summaryMaps = new ArrayList<>();
        List<String> labels = new ArrayList<>();

        for (TimeInterval interval : timeIntervals) {
            Map<String, ProfilingPointSummary> summaryMap =
                PerformanceUtil.getProfilingSummaryMap(tableName, interval.start, interval.end);
            if (summaryMap == null || summaryMap.size() == 0) continue;
            summaryMaps.add(summaryMap);
            labels.add(interval.label);
        }

        List<String> tables = new ArrayList<>();
        List<String> tableTitles = new ArrayList<>();
        List<String> tableSubtitles = new ArrayList<>();
        if (summaryMaps.size() != 0) {
            Map<String, ProfilingPointSummary> todayPerformance = summaryMaps.remove(0);
            Set<String> profilingNameSet = todayPerformance.keySet();
            String[] profilingNames = profilingNameSet.toArray(new String[profilingNameSet.size()]);
            Arrays.sort(profilingNames);

            for (String profilingPointName : profilingNames) {
                ProfilingPointSummary baselinePerformance = todayPerformance.get(profilingPointName);
                String table = getPeformanceSummary(profilingPointName, baselinePerformance, summaryMaps, labels);
                if (table != null) {
                    tables.add(table);
                    tableTitles.add(profilingPointName);
                    switch (baselinePerformance.getRegressionMode()) {
                        case VTS_REGRESSION_MODE_DECREASING:
                            tableSubtitles.add(HIGHER_IS_BETTER);
                            break;
                        default:
                            tableSubtitles.add(LOWER_IS_BETTER);
                            break;
                    }
                }
            }
        }
        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("tables", tables);
        request.setAttribute("tableTitles", tableTitles);
        request.setAttribute("tableSubtitles", tableSubtitles);

        dispatcher = request.getRequestDispatcher("/show_performance_digest.jsp");
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : " + e.toString());
        }
    }
}
