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

import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * PerformanceSummary, an object summarizing performance across profiling points for a test run.
 **/
public class PerformanceSummary {

    protected static Logger logger = Logger.getLogger(PerformanceSummary.class.getName());
    private Map<String, ProfilingPointSummary> summaryMap;
    private Set<String> devices;
    private String deviceFilter;
    private Set<String> optionSplitKeys;

    /**
     * Creates a performance summary object.
     */
    public PerformanceSummary() {
        this.summaryMap = new HashMap<>();
        this.devices = new HashSet<>();
        this.deviceFilter = null;
    }

    /**
     * Creates a performance summary object with the specified device name filter.
     * If the specified name is null, then use no filter.
     * @param deviceFilter The name of the device to include in the performance summary.
     * @param optionSplitKeys A set of option keys to split on (i.e. profiling data with different
     *                        values corresponding to the option key will be analyzed as different
     *                        profiling points).
     */
    public PerformanceSummary(String deviceFilter, Set<String> optionSplitKeys) {
        this();
        if (deviceFilter != null) this.deviceFilter = deviceFilter.trim().toLowerCase();
        this.optionSplitKeys = optionSplitKeys;
    }

    /**
     * Returns true if the test data should be included in the summary.
     * @param testReportMessage The TestReportMessage object to test for inclusion.
     * @return True if the build IDs are integers and the device matches the filter, else False.
     */
    private boolean includeInSummary(TestReportMessage testReportMessage) {
        String buildId = testReportMessage.getBuildInfo().getId().toStringUtf8();

        // filter empty build IDs and add only numbers
        if (buildId.length() == 0) return false;

        // filter empty device info lists
        if (testReportMessage.getDeviceInfoList().size() == 0) return false;

        AndroidDeviceInfoMessage firstDeviceInfo = testReportMessage.getDeviceInfoList().get(0);
        String firstDeviceBuildId = firstDeviceInfo.getBuildId().toStringUtf8();
        String firstDeviceType = firstDeviceInfo.getProductVariant().toStringUtf8().toLowerCase();
        devices.add(firstDeviceType);

        if (deviceFilter != null && !firstDeviceType.equals(deviceFilter)) return false;

        try {
            // filter non-integer build IDs
            Integer.parseInt(buildId);
            Integer.parseInt(firstDeviceBuildId);
        } catch (NumberFormatException e) {
            /* skip a non-post-submit build */
            return false;
        }
        return true;
    }

    /**
     * Add the profiling data from a TestReportMessage to the performance summary.
     * Profiling data is only appended if it matches the specified filter (if any).
     * @param testReportMessage The TestReportMessage object whose data to add.
     */
    public void addData(TestReportMessage testReportMessage) {
        if (!includeInSummary(testReportMessage)) return;

        for (ProfilingReportMessage profilingReportMessage :
             testReportMessage.getProfilingList()) {
            if (profilingReportMessage.getRegressionMode() ==
                VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DISABLED) {
                continue;
            }

            String name = profilingReportMessage.getName().toStringUtf8();
            String optionSuffix = PerformanceUtil.getOptionKeys(
                    profilingReportMessage.getOptionsList(), optionSplitKeys);

            switch (profilingReportMessage.getType()) {
                case VTS_PROFILING_TYPE_TIMESTAMP:
                    logger.log(Level.WARNING, "Timestamp profiling data skipped : " + name);
                    break;
                case VTS_PROFILING_TYPE_LABELED_VECTOR:
                    if (profilingReportMessage.getLabelList().size() == 0 ||
                        profilingReportMessage.getLabelList().size() !=
                        profilingReportMessage.getValueList().size()) {
                        break;
                    }
                    if (!optionSuffix.equals("")) {
                        name += " (" + optionSuffix + ")";
                    }
                    if (!summaryMap.containsKey(name)) {
                        summaryMap.put(name, new ProfilingPointSummary());
                    }
                    summaryMap.get(name).update(profilingReportMessage);
                    break;
                case VTS_PROFILING_TYPE_UNLABELED_VECTOR:
                    if (profilingReportMessage.getValueList().size() == 0) {
                        break;
                    }
                    // Use the option suffix as the table name.
                    // Group all profiling points together into one table
                    if (!summaryMap.containsKey(optionSuffix)) {
                        summaryMap.put(optionSuffix, new ProfilingPointSummary());
                    }
                    summaryMap.get(optionSuffix).updateLabel(
                            profilingReportMessage, profilingReportMessage.getName());
                    break;
                case UNKNOWN_VTS_PROFILING_TYPE:
                default:
                    break;
            }
        }
    }

    /**
     * Adds a ProfilingPointSummary object into the summary map only if the key doesn't exist.
     * @param key The name of the profiling point.
     * @param summary The ProfilingPointSummary object to add into the summary map.
     * @return True if the data was inserted into the performance summary, false otherwise.
     */
    public boolean insertProfilingPointSummary(String key, ProfilingPointSummary summary) {
        if (!summaryMap.containsKey(key)) {
            summaryMap.put(key, summary);
            return true;
        }
        return false;
    }

    /**
     * Gets the set of devices observed in the performance summary.
     * @return The set of devices seen in all TestReportMessage objects processed.
     */
    public Set<String> getDevices() {
        return devices;
    }

    /**
     * Gets the number of profiling points.
     * @return The number of profiling points in the performance summary.
     */
    public int size() {
        return summaryMap.size();
    }

    /**
     * Gets the names of the profiling points.
     * @return A string array of profiling point names.
     */
    public String[] getProfilingPointNames() {
        String[] profilingNames = summaryMap.keySet().toArray(new String[summaryMap.size()]);
        Arrays.sort(profilingNames);
        return profilingNames;
    }

    /**
     * Determines if a profiling point is described by the performance summary.
     * @param profilingPointName The name of the profiling point.
     * @return True if the profiling point is contained in the performance summary, else false.
     */
    public boolean hasProfilingPoint(String profilingPointName) {
        return summaryMap.containsKey(profilingPointName);
    }

    /**
     * Gets the profiling point summary by name.
     * @param profilingPointName The name of the profiling point to fetch.
     * @return The ProfilingPointSummary object describing the specified profiling point.
     */
    public ProfilingPointSummary getProfilingPointSummary(String profilingPointName) {
        return summaryMap.get(profilingPointName);
    }
}
