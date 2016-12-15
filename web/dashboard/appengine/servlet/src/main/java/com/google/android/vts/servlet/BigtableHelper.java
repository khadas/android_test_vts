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
package com.google.android.vts.servlet;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;

import java.io.IOException;
import javax.servlet.ServletContext;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;


/**
 * BigtableHelper, a ServletContextListener, is setup in web.xml to run before a JSP is run.
 **/
public class BigtableHelper implements ServletContextListener {

    private static String PROJECT_ID = System.getenv("BIGTABLE_PROJECT");
    private static String INSTANCE_ID = System.getenv("BIGTABLE_INSTANCE");

    // The initial connection to Cloud Bigtable is an expensive operation -- We cache this
    // connection to speed things up.
    private static Connection connection = null;     // The authenticated connection

    private static ServletContext servletContext;

    // Holds the list of tables in the instance.
    private static HTableDescriptor[] tableDescriptor = null;

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
            servletContext.log("environment variables BIGTABLE_PROJECT, and BIGTABLE_INSTANCE need "
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
        if (tableDescriptor != null) {
            return tableDescriptor;
        }

        try {
            // Instantiating HBaseAdmin object
            Admin admin = getConnection().getAdmin();
            // Getting all the list of tables using HBaseAdmin object
            tableDescriptor = admin.listTables();

        } catch (MasterNotRunningException e) {
            servletContext.log("Exception occurred in com.google.android.vts.servlet.BigtableHelper"
                + ".getTables() ", e);
        } catch (ZooKeeperConnectionException e) {
            servletContext.log("Exception occurred in com.google.android.vts.servlet.BigtableHelper."
                + "getTables() ", e);
        } catch (IOException e) {
            servletContext.log("Exception occurred in com.google.android.vts.servlet.BigtableHelper."
                + "getTables() ", e);
        }

        return tableDescriptor;
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
            servletContext.log("BigtableHelper-No Connection");
        }
        return connection;
    }

    @Override
    public void contextInitialized(ServletContextEvent event) {
        // This will be invoked as part of a warmup request, or the first user
        // request if no warmup request was invoked.
        servletContext = event.getServletContext();
        try {
            connect();
        } catch (IOException e) {
            servletContext.log("BigtableHelper - connect ", e);
        }
        if (connection == null) {
            servletContext.log("BigtableHelper-No Connection");
        }
    }

    @Override
    public void contextDestroyed(ServletContextEvent event) {
        // App Engine does not currently invoke this method.
        if (connection == null) {
            return;
        }
        try {
            connection.close();
        } catch (IOException io) {
            servletContext.log("contextDestroyed ", io);
        }
        connection = null;
    }
}
