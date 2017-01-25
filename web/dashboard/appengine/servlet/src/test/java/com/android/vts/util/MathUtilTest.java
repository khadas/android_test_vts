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
