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

import java.math.RoundingMode;
import java.text.DecimalFormat;

/**
 * MathUtil, a helper class for mathematical operations.
 **/
public class MathUtil {

    /**
     * Rounds a double to a fixed number of digits.
     * @param value The number to be rounded.
     * @param digits The number of decimal places of accuracy.
     * @returns The string representation of 'value' rounded to 'digits' number of decimal places.
     */
    public static String round(double value, int digits) {
        String format = "#." + new String(new char[digits]).replace("\0", "#");
        DecimalFormat formatter = new DecimalFormat(format);
        formatter.setRoundingMode(RoundingMode.HALF_UP);
        return formatter.format(value);
    }
}
