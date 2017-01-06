package com.android.vts.util;

import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Test;
import com.google.protobuf.ByteString;
import java.util.ArrayList;
import java.util.List;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage.Builder;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.ProfilingPointSummary;
import com.android.vts.util.StatSummary;

public class ProfilingPointSummaryTest {
    private static String[] labels = new String[]{"label1", "label2", "label3"};
    private static long[] values = new long[]{1, 2, 3};
    private static ProfilingPointSummary summary;

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

    @Before
    public void setUp() {
        summary = new ProfilingPointSummary();
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(labels, values, mode);
        summary.update(report);
    }

    /**
     * Test that all labels are found by hasLabel.
     */
    @Test
    public void testHasLabel() {
        for (String label : labels) {
            assertTrue(summary.hasLabel(ByteString.copyFromUtf8(label)));
        }
    }

    /**
     * Test that invalid labels are not found by hasLabel.
     */
    @Test
    public void testInvalidHasLabel() {
        assertFalse(summary.hasLabel(ByteString.copyFromUtf8("bad label")));
    }

    /**
     * Test that all stat summaries can be retrieved by profiling point label.
     */
    @Test
    public void testGetStatSummary() {
        for (String label : labels) {
            StatSummary stats = summary.getStatSummary(ByteString.copyFromUtf8(label));
            assertNotNull(stats);
            assertEquals(label, stats.getLabel().toStringUtf8());
        }
    }

    /**
     * Test that the getStatSummary method returns null when the label is not present.
     */
    @Test
    public void testInvalidGetStatSummary() {
        StatSummary stats = summary.getStatSummary(ByteString.copyFromUtf8("bad label"));
        assertNull(stats);
    }

    /**
     * Test that StatSummary objects are iterated in the order that the labels were provided.
     */
    @Test
    public void testIterator() {
        VtsProfilingRegressionMode mode = VtsProfilingRegressionMode.VTS_REGRESSION_MODE_INCREASING;
        ProfilingReportMessage report = createProfilingReport(labels, values, mode);
        summary.update(report);

        int i = 0;
        for (StatSummary stats : summary) {
            assertEquals(labels[i++], stats.getLabel().toStringUtf8());
        }
    }
}
