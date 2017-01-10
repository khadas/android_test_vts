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
import com.android.vts.proto.VtsReportMessage.ProfilingReportMessage;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.proto.VtsReportMessage.VtsProfilingRegressionMode;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
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
    }

    /**
     * Produces a map with summaries for each profiling point present in the specified table.
     * @param tableName The name of the table whose profiling vectors to retrieve.
     * @param startTime The (inclusive) start time in milliseconds to scan from.
     * @param endTime The (exclusive) end time in milliseconds at which to stop scanning.
     * @returns A map with profiling point name keys and ProfilingPointSummary objects as values.
     * @throws IOException
     */
    public static Map<String, ProfilingPointSummary> getProfilingSummaryMap(
            String tableName, long startTime, long endTime) throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        Scan scan = new Scan();
        scan.setStartRow(Bytes.toBytes(Long.toString(startTime * MILLI_TO_MICRO)));
        scan.setStopRow(Bytes.toBytes(Long.toString(endTime * MILLI_TO_MICRO)));
        ResultScanner scanner = table.getScanner(scan);

        Map<String, ProfilingPointSummary> profilingSummaryMap = new HashMap<>();

        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(RESULTS_FAMILY, DATA_QUALIFIER);
            TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage
                                                                  .parseFrom(value);
            String buildId = testReportMessage.getBuildInfo().getId().toStringUtf8();

            // filter empty build IDs and add only numbers
            if (buildId.length() == 0) continue;

            // filter empty device info lists
            if (testReportMessage.getDeviceInfoList().size() == 0) continue;

            String firstDeviceBuildId = testReportMessage.getDeviceInfoList().get(0)
                                        .getBuildId().toStringUtf8();

            try {
                // filter non-integer build IDs
                Integer.parseInt(buildId);
                Integer.parseInt(firstDeviceBuildId);
            } catch (NumberFormatException e) {
                /* skip a non-post-submit build */
                continue;
            }
            for (ProfilingReportMessage profilingReportMessage :
                 testReportMessage.getProfilingList()) {
                if (profilingReportMessage.getRegressionMode() ==
                    VtsProfilingRegressionMode.VTS_REGRESSION_MODE_DISABLED) {
                    continue;
                }

                String name = profilingReportMessage.getName().toStringUtf8();

                switch (profilingReportMessage.getType()) {
                    case UNKNOWN_VTS_PROFILING_TYPE:
                    case VTS_PROFILING_TYPE_TIMESTAMP :
                        break;
                    case VTS_PROFILING_TYPE_LABELED_VECTOR :
                        if (profilingReportMessage.getLabelList().size() == 0 ||
                            profilingReportMessage.getLabelList().size() !=
                            profilingReportMessage.getValueList().size()) {
                            logger.log(Level.SEVERE, "Invalid profiling report sizes : ", name);
                            continue;
                        }
                        if (!profilingSummaryMap.containsKey(name)) {
                            profilingSummaryMap.put(name, new ProfilingPointSummary());
                        }
                        profilingSummaryMap.get(name).update(profilingReportMessage);
                        break;
                    default :
                        break;
                }
            }
        }
        return profilingSummaryMap;
    }
}
