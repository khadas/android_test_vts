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

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import java.util.EnumSet;
import java.util.HashMap;
import java.util.Map;


/**
 * FilterUril, a helper class for parsing and matching search queries to data.
 **/
public class FilterUtil {
    private static final String TERM_DELIMITER = ":";

    /**
     * Key class to represent a filter token.
     */
    public static enum Key {
        DEVICE_BUILD_ID("devicebuildid"),
        BRANCH("branch"),
        TARGET("target"),
        DEVICE("device"),
        VTS_BUILD_ID("vtsbuildid"),
        HOSTNAME("hostname"),
        PASSING("passing"),
        NONPASSING("nonpassing");

        private static final Map<String, Key> keyMap;
        static {
            keyMap = new HashMap<>();
            for (Key k : EnumSet.allOf(Key.class)) {
                keyMap.put(k.toString(), k);
            }
        }

        /**
         * Test if a string is a valid key.
         * @param keyString The key string.
         * @return True if they key string matches a key.
         */
        public static boolean isKey(String keyString) {
            return keyMap.containsKey(keyString);
        }

        /**
         * Parses a key string into a key.
         * @param keyString The key string.
         * @return The key matching the key string.
         */
        public static Key parse(String keyString) {
            return keyMap.get(keyString);
        }

        /**
         * Gets the key at the specified index.
         * @param index The ordered index.
         * @return The key at the specified index.
         */
        public static Key get(int index) {
            return Key.values()[index];
        }

        private final String keyString;

        /**
         * Constructs a key with the specified key string.
         * @param keyString The identifying key string.
         */
        private Key(String keyString) {
            this.keyString = keyString;
        }

        @Override
        public String toString() {
            return this.keyString;
        }
    }

    /**
     * Parse the search string to populate the searchPairs map.
     *
     * Expected format:
     *       field:value vtsbuildid:"local build"
     *
     * Search terms are delimited by spaces and may be enclosed by quotes. Search terms must be
     * preceded by <field>: where the field is in
     * SEARCH_KEYS.
     *
     * @param searchString The String search query.
     * @returns A map containing search keys to values parsed from the search string.
     */
    public static Map<Key, String> parseSearchString(String searchString) {
        Map<Key, String> searchPairs = new HashMap<>();
        if (searchString != null) {
            Matcher m = Pattern.compile("([^\"]\\S*|\".+?\")\\s*").matcher(searchString);
            while (m.find()) {
                String term = m.group(1).replace("\"", "");
                if (term.contains(TERM_DELIMITER)) {
                    String[] terms = term.split(TERM_DELIMITER, 2);
                    if (terms.length == 2 && Key.isKey(terms[0].toLowerCase())) {
                        searchPairs.put(Key.parse(terms[0].toLowerCase()), terms[1]);
                    }
                }
            }
        }
        return searchPairs;
    }

    /**
     * Determine if test report should be included in the result based on search terms.
     * @param report The TestReportMessage object to compare to the search terms.
     * @param searchPairs The map of search keys to values parsed from the search string.
     * @returns boolean True if the report matches the search terms, false otherwise.
     */
    public static boolean includeInSearch(TestReportMessage report, Map<Key, String> searchPairs) {
        if (searchPairs.size() == 0) return true;

        // Verify that VTS build ID matches search term.
        String vtsBuildId = report.getBuildInfo().getId().toStringUtf8().toLowerCase();
        if (searchPairs.containsKey(Key.VTS_BUILD_ID)) {
            if (!vtsBuildId.equals(searchPairs.get(Key.VTS_BUILD_ID))) {
                return false;
            }
        }

        // Verify that the host name matches the search term.
        String hostname = report.getHostInfo().getHostname().toStringUtf8().toLowerCase();
        if (searchPairs.containsKey(Key.HOSTNAME)) {
            if (!hostname.equals(searchPairs.get(Key.HOSTNAME))) {
                return false;
            }
        }

        // Verify that the pass/non-pass count matches the search term
        if (searchPairs.containsKey(Key.PASSING) || searchPairs.containsKey(Key.NONPASSING)) {
            int passCount = 0;
            int nonpassCount = 0;
            for (TestCaseReportMessage testCaseReport : report.getTestCaseList()) {
                if (testCaseReport.getTestResult() ==
                    TestCaseResult.TEST_CASE_RESULT_PASS) {
                    passCount++;
                } else if (testCaseReport.getTestResult() !=
                           TestCaseResult.TEST_CASE_RESULT_SKIP) {
                    nonpassCount++;
                }
            }
            try {
                if (searchPairs.containsKey(Key.PASSING) &&
                    passCount != Integer.parseInt(searchPairs.get(Key.PASSING))) {
                    return false;
                }
                if (searchPairs.containsKey(Key.NONPASSING) &&
                    nonpassCount != Integer.parseInt(searchPairs.get(Key.NONPASSING))) {
                    return false;
                }
            } catch (NumberFormatException e) {
                return false;
            }
        }

        // Verify that device-specific search terms are satisfied between the target devices.
        boolean hasDeviceMatch = false;
        for (AndroidDeviceInfoMessage device : report.getDeviceInfoList()) {
            String[] props = {device.getBuildId().toStringUtf8().toLowerCase(),
                              device.getBuildAlias().toStringUtf8().toLowerCase(),
                              device.getBuildFlavor().toStringUtf8().toLowerCase(),
                              device.getProductVariant().toStringUtf8().toLowerCase()};
            boolean deviceMatches = true;
            for (int i = 0; i < props.length; i++) {
                if (searchPairs.containsKey(Key.get(i)) &&
                    !props[i].equals(searchPairs.get(Key.get(i)))) {
                    deviceMatches = false;
                }
            }
            if (deviceMatches) hasDeviceMatch = true;
        }
        return hasDeviceMatch;
    }

}
