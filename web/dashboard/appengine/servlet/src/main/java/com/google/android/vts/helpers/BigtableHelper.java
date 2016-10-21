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
package com.google.android.vts.helpers;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.filter.KeyOnlyFilter;
import org.apache.hadoop.hbase.filter.PageFilter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.io.IOException;


/**
 * BigtableHelper, a helper class for interacting with a Bigtable instance.
 **/
public class BigtableHelper {

    private static String PROJECT_ID = System.getenv("BIGTABLE_PROJECT");
    private static String INSTANCE_ID = System.getenv("BIGTABLE_INSTANCE");

    private static final Logger logger = LoggerFactory.getLogger(BigtableHelper.class);

    // The initial connection to Cloud Bigtable is an expensive operation -- We cache this
    // connection to speed things up.
    private static Connection connection = null;     // The authenticated connection

    /**
     * Connect will establish the connection to Cloud Bigtable.
     * @throws IOException
     **/
    public static void connect() throws IOException {
        Configuration conf = HBaseConfiguration.create();

        conf.setClass("hbase.client.connection.impl",
            com.google.cloud.bigtable.hbase1_2.BigtableConnection.class,
            org.apache.hadoop.hbase.client.Connection.class);   // Required for Cloud Bigtable

        if (PROJECT_ID == null || INSTANCE_ID == null) {
            logger.info("environment variables BIGTABLE_PROJECT, and BIGTABLE_INSTANCE need "
                        + "to be defined.");
            return;
        }
        conf.set("google.bigtable.project.id", PROJECT_ID);
        conf.set("google.bigtable.instance.id", INSTANCE_ID);

        connection = ConnectionFactory.createConnection(conf);
    }

    /**
     * Returns the list of tables in the Bigtable database.
     *
     * @return tableDescriptor : An array of HTableDescriptor
     * @throws IOException
     */
    public static HTableDescriptor[] getTables() throws IOException {
        HTableDescriptor[] tableDescriptors = null;
        try {
            // Instantiating HBaseAdmin object
            Admin admin = getConnection().getAdmin();
            // Getting all the list of tables using HBaseAdmin object
            tableDescriptors = admin.listTables();

        } catch (MasterNotRunningException e) {
            logger.info("Exception occurred in com.google.android.vts.servlet.BigtableHelper"
                        + ".getTables() ", e);
        } catch (IOException e) {
            logger.info("Exception occurred in com.google.android.vts.servlet.BigtableHelper."
                        + "getTables() ", e);
        }

        return tableDescriptors;
    }

    /**
     * Returns the table corresponding to the table name.
     * @param tableName Describes the table name.
     * @return table An instance of org.apache.hadoop.hbase.client.Table
     * @throws IOException
     */
    public static Table getTable(TableName tableName) throws IOException {
        Table table = null;

        try {
            table = getConnection().getTable(tableName);
        } catch (IOException e) {
            logger.error("Exception occurred in com.google.android.vts.servlet.BigtableHelper."
                         + "getTable()" + e.toString());
            return null;
        }
        return table;
    }

    /**
     * Returns true if there are data points newer than lowerBound in the table.
     * This method assumes the row key is a time stamp.
     * @param table An instance of org.apache.hadoop.hbase.client.Table with a long time stamp as
     *              its row key.
     * @param lowerBound The (inclusive) lower time bound, long, microseconds.
     * @return boolean True if there are newer data points.
     * @throws IOException
     */
    public static boolean hasNewer(Table table, long lowerBound) throws IOException {
        FilterList filters = new FilterList();
        filters.addFilter(new PageFilter(1));
        filters.addFilter(new KeyOnlyFilter());
        Scan scan = new Scan();
        scan.setStartRow(Long.toString(lowerBound).getBytes());
        scan.setFilter(filters);
        ResultScanner scanner = table.getScanner(scan);
        int count = 0;
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            count++;
        }
        scanner.close();
        return count > 0;
    }

    /**
     * Returns true if there are data points older than upperBound in the table.
     * This method assumes the row key is a time stamp.
     * @param table An instance of org.apache.hadoop.hbase.client.Table with a long time stamp as
     *              its row key.
     * @param upperBound The (exclusive) upper time bound, long, microseconds.
     * @return boolean True if there are older data points.
     * @throws IOException
     */
    public static boolean hasOlder(Table table, long upperBound) throws IOException {
        FilterList filters = new FilterList();
        filters.addFilter(new PageFilter(1));
        filters.addFilter(new KeyOnlyFilter());
        Scan scan = new Scan();
        scan.setStopRow(Long.toString(upperBound).getBytes());
        scan.setFilter(filters);
        ResultScanner scanner = table.getScanner(scan);
        int count = 0;
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            count++;
        }
        scanner.close();
        return count > 0;
    }

    /**
     * Returns an instance of connection to the big table database.
     *
     * @return connection : An instance of org.apache.hadoop.hbase.client.Connection
     * @throws IOException
     */
    public static Connection getConnection() throws IOException {
        if (connection == null) {
            connect();
        }
        if (connection == null) {
            logger.info("BigtableHelper-No Connection");
        }
        return connection;
    }
}
