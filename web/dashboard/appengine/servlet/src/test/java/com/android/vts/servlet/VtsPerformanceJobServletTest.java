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

import static org.junit.Assert.*;

import org.junit.Test;
import com.google.protobuf.ByteString;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage.Builder;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.PerformanceSummary;
import com.android.vts.util.ProfilingPointSummary;

public class VtsPerformanceJobServletTest {
    private static final String LABEL = "testLabel";
    private static final String ROOT = "src/test/res/servlet";
    private static final String[] LABELS = new String[]{"label1", "label2", "label3"};
    private static final long[] HIGH_VALS = new long[]{10, 20, 30};
    private static final long[] LOW_VALS = new long[]{1, 2, 3};

    List<PerformanceSummary> dailySummaries;
    List<String> legendLabels;

    /**
     * Helper method for creating ProfilingReportMessage objects.
     * @param labels The list of data labels.
     * @param values The list of data values. Must be equal in size to the labels list.
     * @param regressionMode The regression mode.
     * @return A ProfilingReportMessage with specified arguments.
     */
    private static ProfilingReportMessage createProfilingReport(
            String[] labels, long[] values, VtsProfilingRegressionMode regressionMode) {
        Builder builder = ProfilingReportMessage.newBuilder();
        List<ByteString> labelList = new ArrayList<>();
        List<Long> valueList = new ArrayList<>();
        for (String label : labels) labelList.add(ByteString.copyFromUtf8(label));
        for (long value : values) valueList.add(value);
        builder.addAllLabel(labelList);
        builder.addAllValue(valueList);
        builder.setRegressionMode(regressionMode);
        return builder.build();
    }

    /**
     * Asserts whether text is the same as the contents in the baseline file specified.
     */
    private static void compareToBaseline(String text, String baselineFilename)
            throws FileNotFoundException, IOException {
        File f = new File(ROOT, baselineFilename);
        String baseline = "";
        try(BufferedReader br = new BufferedReader(new FileReader(f))) {
            StringBuilder sb = new StringBuilder();
            String line = br.readLine();

            while (line != null) {
                sb.append(line);
                line = br.readLine();
            }
            baseline = sb.toString();
        }
        assertEquals(baseline, text);
    }

    public void setUp(boolean grouped) {
        dailySummaries = new ArrayList<>();
        legendLabels = new ArrayList<>();
        legendLabels.add("");

        // Add today's data
        PerformanceSummary today = new PerformanceSummary();
        ProfilingPointSummary summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(LABELS, HIGH_VALS, mode);
        if (grouped) {
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
        } else {
            summary.update(report);
            summary.update(report);
        }
        today.insertProfilingPointSummary("p1", summary);

        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING;
        report = createProfilingReport(LABELS, LOW_VALS, mode);
        if (grouped) {
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
        } else {
            summary.update(report);
            summary.update(report);
        }
        today.insertProfilingPointSummary("p2", summary);
        dailySummaries.add(today);
        legendLabels.add("today");

        // Add yesterday data with regressions
        PerformanceSummary yesterday = new PerformanceSummary();
        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        report = createProfilingReport(LABELS, LOW_VALS, mode);
        if (grouped) {
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
        } else {
            summary.update(report);
            summary.update(report);
        }
        yesterday.insertProfilingPointSummary("p1", summary);

        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING;
        report = createProfilingReport(LABELS, HIGH_VALS, mode);
        if (grouped) {
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
            summary.updateLabel(report, ByteString.copyFromUtf8(LABEL));
        } else {
            summary.update(report);
            summary.update(report);
        }
        yesterday.insertProfilingPointSummary("p2", summary);
        dailySummaries.add(yesterday);
        legendLabels.add("yesterday");

        // Add last week data without regressions
        PerformanceSummary lastWeek = new PerformanceSummary();
        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        report = createProfilingReport(LABELS, HIGH_VALS, mode);
        summary.update(report);
        summary.update(report);
        lastWeek.insertProfilingPointSummary("p1", summary);

        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING;
        report = createProfilingReport(LABELS, LOW_VALS, mode);
        summary.update(report);
        summary.update(report);
        lastWeek.insertProfilingPointSummary("p2", summary);
        dailySummaries.add(lastWeek);
        legendLabels.add("last week");
    }

    /**
     * End-to-end test of performance report in the normal case.
     * The normal case is when a profiling point is added or removed from the test.
     */
    @Test
    public void testPerformanceSummaryNormal() throws FileNotFoundException, IOException {
        setUp(false);
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        compareToBaseline(output, "performanceSummary1.html");
    }

    /**
     * End-to-end test of performance report when a profiling point was removed in the latest run.
     */
    @Test
    public void testPerformanceSummaryDroppedProfilingPoint() throws FileNotFoundException, IOException {
        setUp(false);
        PerformanceSummary yesterday = dailySummaries.get(dailySummaries.size() - 1);
        ProfilingPointSummary summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(LABELS, HIGH_VALS, mode);
        summary.update(report);
        summary.update(report);
        yesterday.insertProfilingPointSummary("p3", summary);
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        compareToBaseline(output, "performanceSummary2.html");
    }

    /**
     * End-to-end test of performance report when a profiling point was added in the latest run.
     */
    @Test
    public void testPerformanceSummaryAddedProfilingPoint() throws FileNotFoundException, IOException {
        setUp(false);
        PerformanceSummary today = dailySummaries.get(0);
        ProfilingPointSummary summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(LABELS, HIGH_VALS, mode);
        summary.update(report);
        summary.update(report);
        today.insertProfilingPointSummary("p3", summary);
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        compareToBaseline(output, "performanceSummary3.html");
    }

    /**
     * End-to-end test of performance report labels are grouped (e.g. as if using unlabled data)
     */
    @Test
    public void testPerformanceSummaryGroupedNormal() throws FileNotFoundException, IOException {
        setUp(true);
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        System.out.println(output);
        compareToBaseline(output, "performanceSummary4.html");
    }
}
