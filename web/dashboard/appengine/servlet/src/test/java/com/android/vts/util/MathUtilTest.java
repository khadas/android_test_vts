package com.android.vts.util;

import static org.junit.Assert.*;

import org.junit.Test;
import com.android.vts.util.MathUtil;

public class MathUtilTest {
    private static int N_DIGITS = 3;

    /**
     * Test rounding of a whole number.
     */
    @Test
    public void testSimple() {
        int number = 5;
        String rounded = MathUtil.round(number, N_DIGITS);
        assertEquals(Integer.toString(number), rounded);
    }

    /**
     * Test rounding of a number with fewer significant figures than rounding digits.
     */
    @Test
    public void testBasicDecimal() {
        double number = 5.5;
        String rounded = MathUtil.round(number, N_DIGITS);
        assertEquals(Double.toString(number), rounded);
    }

    /**
     * Test rounding of a number with infinite significant figures.
     */
    @Test
    public void testInfiniteDecimal() {
        double number = 1.0 / 3;
        String rounded = MathUtil.round(number, N_DIGITS);
        assertEquals("0.333", rounded);
    }
}
