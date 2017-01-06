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

package com.android.vts.api;

import com.android.vts.util.BigtableHelper;
import com.google.api.client.json.gson.GsonFactory;
import com.google.api.client.http.javanet.NetHttpTransport;
import com.google.api.client.json.jackson.JacksonFactory;
import com.google.api.client.googleapis.auth.oauth2.GoogleCredential;
import com.google.api.client.googleapis.auth.oauth2.GoogleIdToken;
import com.google.api.client.googleapis.auth.oauth2.GoogleIdTokenVerifier;
import com.google.api.services.oauth2.Oauth2;
import com.google.api.services.oauth2.model.Tokeninfo;
import org.apache.commons.codec.binary.Base64;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.BufferedReader;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Represents the servlet that is invoked on loading the first page of dashboard.
 */
@WebServlet(name = "bigtable", urlPatterns = {"/api/bigtable"})
public class BigtableApiServlet extends HttpServlet {

    private static final String SERVICE_CLIENT_ID = System.getenv("SERVICE_CLIENT_ID");
    private static final Logger logger = Logger.getLogger(BigtableApiServlet.class.getName());

    @Override
    public void doPost(HttpServletRequest request, HttpServletResponse response) throws IOException {
        // Retrieve the params
        String payload = new String();
        JSONObject payloadJson;
        try {
            String line = null;
            BufferedReader reader = request.getReader();
            while ((line = reader.readLine()) != null) {
                payload += line;
            }
            payloadJson = new JSONObject(payload);
        } catch (IOException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        // Verify service account access token.
        boolean authorized = false;
        if (payloadJson.has("accessToken")) {
            String accessToken = payloadJson.getString("accessToken").trim();
            GoogleCredential credential = new GoogleCredential().setAccessToken(accessToken);
            Oauth2 oauth2 = new Oauth2.Builder(new NetHttpTransport(), new JacksonFactory(),
                                               credential).build();
            Tokeninfo tokenInfo = oauth2.tokeninfo().setAccessToken(accessToken).execute();
            if (tokenInfo.getIssuedTo().equals(SERVICE_CLIENT_ID)) {
                authorized = true;
            }
        }

        if (!authorized) {
            response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
            return;
        }

        // Parse the desired action and execute the command
        try {
            if (payloadJson.has("verb")) {
                switch (payloadJson.getString("verb")) {
                    case "createTable":
                        createTable(payloadJson);
                        break;
                    case "insertRow":
                        insertRow(payloadJson);
                        break;
                    default:
                        logger.log(Level.INFO, "Invalid Bigtable API verb: " +
                                   payloadJson.getString("verb"));
                        throw new IOException("Unsupported POST verb.");
                }
            }
        } catch (IOException e) {
            response.setStatus(HttpServletResponse.SC_BAD_REQUEST);
            return;
        }
        response.setStatus(HttpServletResponse.SC_OK);
    }

    /**
     * Creates a table in the Bigtable instance.
     * @param payloadJson The JSON object representing the table to be created. Of the form:
     *                    {
     *                      'tableName' : 'table',
     *                      'familyNames' : ['family1', 'family2', 'family3']
     *                    }
     * @throws IOException
     */
    private void createTable(JSONObject payloadJson) throws IOException {
        if (!payloadJson.has("tableName") || !payloadJson.has("familyNames")) {
            logger.log(Level.INFO, "Missing attributes for bigtable api createTable().");
            throw new IOException("Missing attributes.");
        }
        String table = payloadJson.getString("tableName");
        TableName tableName = TableName.valueOf(table);
        JSONArray familyArray = payloadJson.getJSONArray("familyNames");
        HTableDescriptor tableDescriptor = new HTableDescriptor(tableName);
        for (int i = 0; i < familyArray.length(); i++) {
            tableDescriptor.addFamily(new HColumnDescriptor(familyArray.getString(i).trim()));
        }
        Admin admin = BigtableHelper.getConnection().getAdmin();
        admin.createTable(tableDescriptor);
    }

    /**
     * Inserts a row into the BigTable instance
     * @param payloadJson The JSON object representing the row to be inserted. Of the form:
     *                    {
     *                      'tableName' : 'table',
     *                      'rowKey' : 'row',
     *                      'family' : 'family',
     *                      'qualifier' : 'qualifier',
     *                      'value' : 'value'
     *                    }
     * @throws IOException
     */
    private void insertRow(JSONObject payloadJson) throws IOException {
        if (!payloadJson.has("tableName") || !payloadJson.has("rowKey") ||
            !payloadJson.has("family") || !payloadJson.has("qualifier") ||
            !payloadJson.has("value")) {
            logger.log(Level.INFO, "Missing attributes for bigtable api insertRow().");
            throw new IOException("Missing attributes.");
        }
        String tableName = payloadJson.getString("tableName");
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        String row = payloadJson.getString("rowKey").trim();
        String family = payloadJson.getString("family").trim();
        String qualifier = payloadJson.getString("qualifier").trim();
        byte[] value = Base64.decodeBase64(payloadJson.getString("value"));
        Put put = new Put(Bytes.toBytes(row));
        put.addColumn(Bytes.toBytes(family), Bytes.toBytes(qualifier), value);
        table.put(put);
    }
}
