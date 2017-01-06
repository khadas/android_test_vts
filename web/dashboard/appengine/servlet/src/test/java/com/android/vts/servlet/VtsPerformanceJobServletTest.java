package com.android.vts.servlet;

import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Test;
import com.google.protobuf.ByteString;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage.Builder;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.ProfilingPointSummary;

public class VtsPerformanceJobServletTest {
    private static String root = "src/test/res/servlet";
    private static String[] labels = new String[]{"label1", "label2", "label3"};
    private static long[] highValues = new long[]{10, 20, 30};
    private static long[] lowValues = new long[]{1, 2, 3};

    List<Map<String, ProfilingPointSummary>> dailySummaries;
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
        File f = new File(root, baselineFilename);
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

    @Before
    public void setUp() {
        dailySummaries = new ArrayList<>();
        legendLabels = new ArrayList<>();
        legendLabels.add("");

        // Add today's data
        Map<String, ProfilingPointSummary> today = new HashMap<>();
        ProfilingPointSummary summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(labels, highValues, mode);
        summary.update(report);
        summary.update(report);
        today.put("p1", summary);

        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING;
        report = createProfilingReport(labels, lowValues, mode);
        summary.update(report);
        summary.update(report);
        today.put("p2", summary);
        dailySummaries.add(today);
        legendLabels.add("today");

        // Add yesterday data with regressions
        Map<String, ProfilingPointSummary> yesterday = new HashMap<>();
        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        report = createProfilingReport(labels, lowValues, mode);
        summary.update(report);
        summary.update(report);
        yesterday.put("p1", summary);

        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING;
        report = createProfilingReport(labels, highValues, mode);
        summary.update(report);
        summary.update(report);
        yesterday.put("p2", summary);
        dailySummaries.add(yesterday);
        legendLabels.add("yesterday");

        // Add last week data without regressions
        Map<String, ProfilingPointSummary> lastWeek = new HashMap<>();
        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        report = createProfilingReport(labels, highValues, mode);
        summary.update(report);
        summary.update(report);
        lastWeek.put("p1", summary);

        summary = new ProfilingPointSummary();
        mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING;
        report = createProfilingReport(labels, lowValues, mode);
        summary.update(report);
        summary.update(report);
        lastWeek.put("p2", summary);
        dailySummaries.add(lastWeek);
        legendLabels.add("last week");
    }

    /**
     * End-to-end test of performance report in the normal case.
     * The normal case is when a profiling point is added or removed from the test.
     */
    @Test
    public void testPerformanceSummaryNormal() throws FileNotFoundException, IOException {
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        compareToBaseline(output, "performanceSummary1.html");
    }

    /**
     * End-to-end test of performance report when a profiling point was removed in the latest run.
     */
    @Test
    public void testPerformanceSummaryDroppedProfilingPoint() throws FileNotFoundException, IOException {
        Map<String, ProfilingPointSummary> yesterday = dailySummaries.get(dailySummaries.size() - 1);
        ProfilingPointSummary summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(labels, highValues, mode);
        summary.update(report);
        summary.update(report);
        yesterday.put("p3", summary);
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        compareToBaseline(output, "performanceSummary2.html");
    }

    /**
     * End-to-end test of performance report when a profiling point was added in the latest run.
     */
    @Test
    public void testPerformanceSummaryAddedProfilingPoint() throws FileNotFoundException, IOException {
        Map<String, ProfilingPointSummary> today = dailySummaries.get(0);
        ProfilingPointSummary summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(labels, highValues, mode);
        summary.update(report);
        summary.update(report);
        today.put("p3", summary);
        String output = VtsPerformanceJobServlet.getPeformanceSummary("result_test", dailySummaries, legendLabels);
        compareToBaseline(output, "performanceSummary3.html");
    }
}
