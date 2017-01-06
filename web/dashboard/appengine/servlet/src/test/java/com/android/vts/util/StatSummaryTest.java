package com.android.vts.util;

import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Test;
import com.google.protobuf.ByteString;
import java.util.Random;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import com.android.vts.util.StatSummary;

public class StatSummaryTest {
    private static double threshold = 0.0000000001;
    private StatSummary test;

    @Before
    public void setUp() {
        test = new StatSummary(ByteString.copyFromUtf8("label"),
                               VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DECREASING);
    }

    /**
     * Test computation of average.
     */
    @Test
    public void testAverage() {
        int n = 1000;
        double mean = (n - 1) / 2.0;
        for (int i = 0; i < n; i++) {
            test.updateStats(i);
        }
        assertEquals(n, test.getCount(), threshold);
        assertEquals(mean, test.getMean(), threshold);
    }

    /**
     * Test computation of minimum.
     */
    @Test
    public void testMin() {
        double min = Double.MAX_VALUE;
        int n = 1000;
        Random rand = new Random();
        for (int i = 0; i < n; i++) {
            double value = rand.nextInt(1000);
            if (value < min) min = value;
            test.updateStats(value);
        }
        assertEquals(n, test.getCount(), threshold);
        assertEquals(min, test.getMin(), threshold);
    }

    /**
     * Test computation of maximum.
     */
    @Test
    public void testMax() {
        double max = Double.MIN_VALUE;
        int n = 1000;
        Random rand = new Random();
        for (int i = 0; i < n; i++) {
            double value = rand.nextInt(1000);
            if (value > max) max = value;
            test.updateStats(value);
        }
        assertEquals(max, test.getMax(), threshold);
    }

    /**
     * Test computation of standard deviation.
     */
    @Test
    public void testStd() {
        int n = 1000;
        double[] values = new double[n];
        Random rand = new Random();
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            values[i] = rand.nextInt(1000);
            sum += values[i];
            test.updateStats(values[i]);
        }
        double mean = sum / n;
        double sumSq = 0;
        for (int i = 0; i < n; i++) {
            sumSq += (values[i] - mean) * (values[i] - mean);
        }
        double std = Math.sqrt(sumSq / (n - 1));
        assertEquals(std, test.getStd(), threshold);
    }
}
