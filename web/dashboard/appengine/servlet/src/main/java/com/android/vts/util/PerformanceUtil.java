/**
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import com.android.vts.proto.VtsReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import java.io.IOException;
import java.util.logging.Logger;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;

/**
 * PerformanceUtil, a helper class for analyzing profiling and performance data.
 **/
public class PerformanceUtil {
    protected static Logger logger = Logger.getLogger(PerformanceUtil.class.getName());

    private static final byte[] RESULTS_FAMILY = Bytes.toBytes("test");
    private static final byte[] DATA_QUALIFIER = Bytes.toBytes("data");
    private static final int N_DIGITS = 2;
    private static final long MILLI_TO_MICRO = 1000;  // conversion factor from milli to micro units

    public static class TimeInterval {
        public final long start;
        public final long end;
        public final String label;

        public TimeInterval(long start, long end, String label) {
            this.start = start;
            this.end = end;
            this.label = label;
        }

        public TimeInterval(long start, long end) {
            this(start, end, "<span class='date-label'>" + Long.toString(end) + "</span>");
        }
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
     * @param classNames A string containing HTML classes to apply to the table cell.
     * @param style A string containing additional CSS styles.
     * @returns An HTML string for a colored table cell containing the percent change.
     */
    public static String getPercentChangeHTML(double baseline, double test, String classNames,
                                              String style, VtsProfilingRegressionMode mode) {
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
        String color = "background-color: rgba(255, 0, 0, " + alpha + "); ";
        String html = "<td class='" + classNames + "' style='" + color + style + "'>";
        html += pctChangeString + "</td>";
        return html;
    }

    /**
     * Compares a test StatSummary to a baseline StatSummary using best-case performance.
     * @param baseline The StatSummary object containing initial values to compare against
     * @param test The StatSummary object containing test values to be compared against the baseline
     * @param innerClasses Class names to apply to cells on the inside of the grid
     * @param outerClasses Class names to apply to cells on the outside of the grid
     * @param innerStyles CSS styles to apply to cells on the inside of the grid
     * @param outerStyles CSS styles to apply to cells on the outside of the grid
     * @return HTML string representing the performance of the test versus the baseline
     */
    public static String getBestCasePerformanceComparisonHTML(
            StatSummary baseline, StatSummary test, String innerClasses, String outerClasses,
            String innerStyles, String outerStyles) {
        if (test == null || baseline == null) {
            return "<td></td><td></td><td></td><td></td>";
        }
        String row = "";
        // Intensity of red color is a function of the relative (percent) change
        // in the new value compared to the previous day's. Intensity is a linear function
        // of percentage change, reaching a ceiling at 100% change (e.g. a doubling).
        row += getPercentChangeHTML(baseline.getBestCase(), test.getBestCase(), innerClasses, innerStyles,
                                    test.getRegressionMode());
        row += "<td class='" + innerClasses + "' style='" + innerStyles +"'>";
        row += MathUtil.round(baseline.getBestCase(), N_DIGITS);
        row += "<td class='" + innerClasses + "' style='" + innerStyles +"'>";
        row += MathUtil.round(baseline.getMean(), N_DIGITS);
        row += "<td class='" + outerClasses + "' style='" + outerStyles +"'>";
        row += MathUtil.round(baseline.getStd(), N_DIGITS) + "</td>";
        return row;
    }

    /**
     * Compares a test StatSummary to a baseline StatSummary using average-case performance.
     * @param baseline The StatSummary object containing initial values to compare against
     * @param test The StatSummary object containing test values to be compared against the baseline
     * @param innerClasses Class names to apply to cells on the inside of the grid
     * @param outerClasses Class names to apply to cells on the outside of the grid
     * @param innerStyles CSS styles to apply to cells on the inside of the grid
     * @param outerStyles CSS styles to apply to cells on the outside of the grid
     * @return HTML string representing the performance of the test versus the baseline
     */
    public static String getAvgCasePerformanceComparisonHTML(
            StatSummary baseline, StatSummary test, String innerClasses, String outerClasses,
            String innerStyles, String outerStyles) {
        if (test == null || baseline == null) {
            return "<td></td><td></td><td></td><td></td>";
        }
        String row = "";
        // Intensity of red color is a function of the relative (percent) change
        // in the new value compared to the previous day's. Intensity is a linear function
        // of percentage change, reaching a ceiling at 100% change (e.g. a doubling).
        row += getPercentChangeHTML(baseline.getMean(), test.getMean(), innerClasses, innerStyles,
                                    test.getRegressionMode());
        row += "<td class='" + innerClasses + "' style='" + innerStyles +"'>";
        row += MathUtil.round(baseline.getBestCase(), N_DIGITS);
        row += "<td class='" + innerClasses + "' style='" + innerStyles +"'>";
        row += MathUtil.round(baseline.getMean(), N_DIGITS);
        row += "<td class='" + outerClasses + "' style='" + outerStyles +"'>";
        row += MathUtil.round(baseline.getStd(), N_DIGITS) + "</td>";
        return row;
    }

    /**
     * Updates a PerformanceSummary object with data in the specified window.
     * @param tableName The name of the table whose profiling vectors to retrieve.
     * @param startTime The (inclusive) start time in milliseconds to scan from.
     * @param endTime The (exclusive) end time in milliseconds at which to stop scanning.
     * @param perfSummary The PerformanceSummary object to update with data.
     * @throws IOException
     */
    public static void updatePerformanceSummary(String tableName, long startTime, long endTime,
                                                PerformanceSummary perfSummary) throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        Scan scan = new Scan();
        scan.setStartRow(Bytes.toBytes(Long.toString(startTime * MILLI_TO_MICRO)));
        scan.setStopRow(Bytes.toBytes(Long.toString(endTime * MILLI_TO_MICRO)));
        ResultScanner scanner = table.getScanner(scan);

        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(RESULTS_FAMILY, DATA_QUALIFIER);
            TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage
                                                                  .parseFrom(value);
            perfSummary.addData(testReportMessage);
        }
    }
}
