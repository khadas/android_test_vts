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
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import java.io.IOException;
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

        public TimeInterval(long start, long end) {
            this(start, end, "<span class='date-label'>" + Long.toString(end) + "</span>");
        }
    }

    /**
     * Updates a PerformanceSummary object with data in the specified window.
     * @param tableName The name of the table whose profiling vectors to retrieve.
     * @param startTime The (inclusive) start time in milliseconds to scan from.
     * @param endTime The (exclusive) end time in milliseconds at which to stop scanning.
     * @param perfSummary The PerformanceSummary object to update with data.
     * @throws IOException
     */
    public static void updatePerformanceSummary(String tableName, long startTime, long endTime,
                                                PerformanceSummary perfSummary) throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        Scan scan = new Scan();
        scan.setStartRow(Bytes.toBytes(Long.toString(startTime * MILLI_TO_MICRO)));
        scan.setStopRow(Bytes.toBytes(Long.toString(endTime * MILLI_TO_MICRO)));
        ResultScanner scanner = table.getScanner(scan);

        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            byte[] value = result.getValue(RESULTS_FAMILY, DATA_QUALIFIER);
            TestReportMessage testReportMessage = VtsReportMessage.TestReportMessage
                                                                  .parseFrom(value);
            perfSummary.addData(testReportMessage);
        }
    }
}
