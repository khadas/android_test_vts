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

package com.google.android.vts.servlet;
import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.android.vts.proto.VtsReportMessage;
import com.google.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.google.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.google.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.google.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.google.android.vts.proto.VtsWebStatusMessage;
import com.google.android.vts.proto.VtsWebStatusMessage.TestStatus;
import com.google.android.vts.proto.VtsWebStatusMessage.TestStatusMessage;
import com.google.android.vts.proto.VtsWebStatusMessage.TestStatusMessage.Builder;
import com.google.protobuf.ByteString;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeMessage;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Represents the servlet that is invoked on loading the first page of dashboard.
 */
@WebServlet(name = "vts_alert_job", urlPatterns = {"/cron/vts_alert_job"})
public class VtsAlertJobServlet extends HttpServlet {

    private static final byte[] RESULTS_FAMILY = Bytes.toBytes("test");
    private static final byte[] STATUS_FAMILY = Bytes.toBytes("status");
    private static final byte[] DATA_QUALIFIER = Bytes.toBytes("data");
    private static final byte[] TIME_QUALIFIER = Bytes.toBytes("upload_timestamp");
    private static final String STATUS_TABLE = "vts_status_table";
    private static final String VTS_EMAIL_ADDRESS = "vts-alert@google.com";
    private static final String VTS_EMAIL_NAME = "VTS Alert Bot";
    private static final long ONE_DAY = 86400000000L;  // units microseconds

    private static final Logger logger = LoggerFactory.getLogger(DashboardMainServlet.class);

    /**
     * Sends an email to the specified email address to notify of a test status change.
     * @param emails List of subscriber email addresses (byte[]) to which the email should be sent.
     * @param subject The email subject field, string.
     * @param body The html (string) body to send in the email.
     * @returns The Message object to be sent.
     * @throws MessagingException, UnsupportedEncodingException
     */
    public Message composeEmail(List<ByteString> emails, String subject, String body)
            throws MessagingException, UnsupportedEncodingException {
        if (emails.size() == 0) {
            throw new MessagingException("No subscriber email addresses provided");
        }
        Properties props = new Properties();
        Session session = Session.getDefaultInstance(props, null);

        Message msg = new MimeMessage(session);
        for (ByteString email : emails) {
            String email_utf8 = email.toStringUtf8();
            try {
                msg.addRecipient(Message.RecipientType.TO,
                                 new InternetAddress(email_utf8, email_utf8));
            } catch (MessagingException | UnsupportedEncodingException e) {
                // Gracefully continue when a subscriber email is invalid.
                logger.warn("Error sending email to recipient " + email + " : ", e);
            }
        }
        msg.setFrom(new InternetAddress(VTS_EMAIL_ADDRESS, VTS_EMAIL_NAME));
        msg.setSubject(subject);
        msg.setContent(body, "text/html; charset=utf-8");
        return msg;
    }

    /**
     * Checks whether any new failures have occurred beginning since (and including) startTime.
     * @param tableName The name of the table that stores the results for a test, string.
     * @param startTime The (inclusive) lower time bound, long, microseconds.
     * @param prevStatus The TestStatus for the previous iteration.
     * @param messages The email Message queue.
     * @returns latest test status.
     * @throws IOException
     */
    public TestStatus getTestStatus(String tableName, long startTime, TestStatus prevStatus,
            List<Message> messages) throws IOException {
        Scan scan = new Scan();
        scan.setStartRow(Long.toString(startTime).getBytes());
        Table table = BigtableHelper.getTable(TableName.valueOf(tableName));
        ResultScanner scanner = table.getScanner(scan);
        List<String> testRunKeyList = new ArrayList<>();
        Map<String, TestReportMessage> buildIdTimeStampMap = new HashMap<>();

        boolean anyFailed = false;
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

            String key;
            try {
                // filter non-integer build IDs
                Integer.parseInt(buildId);
                Integer.parseInt(firstDeviceBuildId);
                key = testReportMessage.getBuildInfo().getId().toStringUtf8()
                  + "." + String.valueOf(testReportMessage.getStartTimestamp());
                testRunKeyList.add(0, key);
                // update map based on time stamp.
                buildIdTimeStampMap.put(key, testReportMessage);
            } catch (NumberFormatException e) {
                /* skip a non-post-submit build */
                continue;
            }

            for (TestCaseReportMessage testCaseReportMessage :
                 testReportMessage.getTestCaseList()) {
                if (testCaseReportMessage.getTestResult() ==
                    TestCaseResult.TEST_CASE_RESULT_FAIL) {
                     anyFailed = true;
                }
            }
        }
        scanner.close();
        if (testRunKeyList.size() == 0) return prevStatus;

        boolean latestFailing = false;
        TestReportMessage latestTest = buildIdTimeStampMap.get(testRunKeyList.get(0));
        for (TestCaseReportMessage testCaseReportMessage : latestTest.getTestCaseList()) {
            if (testCaseReportMessage.getTestResult() == TestCaseResult.TEST_CASE_RESULT_FAIL) {
                latestFailing = true;
            }
        }

        String test = latestTest.getTest().toStringUtf8();
        List<String> buildIdList = new ArrayList<>();
        for (AndroidDeviceInfoMessage device : latestTest.getDeviceInfoList()) {
            buildIdList.add(device.getBuildId().toStringUtf8());
        }
        String buildId = StringUtils.join(buildIdList, ",");

        if (latestFailing) {
            String subject = "New test failure in " + test + " @ " + buildId;
            String body = "Hello,<br><br>Test cases are failing in " + test
                          + " for device build ID(s): " + buildId
                          + ".<br><br>For details, visit the"
                          + " <a href='https://android-vts-internal.googleplex.com/'>"
                          + "VTS dashboard.</a>";
            try {
                messages.add(composeEmail(latestTest.getSubscriberEmailList(), subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.error("Error composing email : ", e);
            }
            return TestStatus.TEST_FAIL;
        } else if (anyFailed && prevStatus == TestStatus.TEST_OK) {
            // Transient fail case (i.e. pass, pass, fail, pass, pass)
            String subject = "Transient test failure in " + test + " @ " + buildId;
            String body = "Hello,<br><br>Some test cases failed in " + test + " but tests all "
                          + "are passing in the latest device build(s): "
                          + buildId + ".<br><br>For details, visit the"
                          + " <a href='https://android-vts-internal.googleplex.com/'>"
                          + "VTS dashboard.</a>";
            try {
                messages.add(composeEmail(latestTest.getSubscriberEmailList(), subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.error("Error composing email : ", e);
            }
        } else if (prevStatus == TestStatus.TEST_FAIL) {
            // Test failure fixed
            String subject = "All test cases passing in " + test + " @ " + buildId;
            String body = "Hello,<br><br>All test cases passed in " + test
                          + " for device build ID(s): " + buildId
                          + "!<br><br>For details, visit the "
                          + "<a href='https://android-vts-internal.googleplex.com/'>"
                          + "VTS dashboard.</a>";
            try {
                messages.add(composeEmail(latestTest.getSubscriberEmailList(), subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.error("Error composing email : ", e);
            }
        }
        return TestStatus.TEST_OK;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(STATUS_TABLE));
        Scan scan = new Scan();
        ResultScanner scanner = table.getScanner(scan);
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            String testName = Bytes.toString(result.getRow());
            byte[] value = result.getValue(STATUS_FAMILY, DATA_QUALIFIER);

            // Get the time stamp for the last upload.
            long lastUploadTimestamp;
            try {
                lastUploadTimestamp = Long.parseLong(Bytes.toString(
                    result.getValue(STATUS_FAMILY, TIME_QUALIFIER)));
            } catch (NumberFormatException e) {
                // If no upload timestamp, skip this row.
                logger.warn("Error parsing upload timestamp: ", e);
                continue;
            }
            Builder builder;
            TestStatus status;
            List<Message> messageQueue = new ArrayList<>();
            if (value == null) {
                // No table entries yet. Fetch tests as far back as one day.
                status = getTestStatus(testName, lastUploadTimestamp - ONE_DAY,
                                       VtsWebStatusMessage.TestStatus.TEST_OK, messageQueue);

                // Create a new TestStatusMessage.
                builder = VtsWebStatusMessage.TestStatusMessage.newBuilder();
            } else {
                TestStatusMessage testStatusMessage = VtsWebStatusMessage.TestStatusMessage
                                                          .parseFrom(value);
                long statusTimestamp = testStatusMessage.getStatusTimestamp();

                // If newer data exists, get the latest test status.
                if (lastUploadTimestamp > statusTimestamp) {
                    status = getTestStatus(testName, statusTimestamp + 1,
                                           testStatusMessage.getStatus(), messageQueue);
                } else {
                    status = testStatusMessage.getStatus();  // keep the old status
                }

                // Create a new TestStatusMessage based off of the old status.
                builder = VtsWebStatusMessage.TestStatusMessage.newBuilder(testStatusMessage);
            }

            // Update the status and status timestamp.
            builder.setStatusTimestamp(lastUploadTimestamp);
            builder.setStatus(status);

            // Create row insertion operation.
            Put put = new Put(Bytes.toBytes(testName));
            put.addColumn(STATUS_FAMILY, DATA_QUALIFIER,
                          builder.build().toByteArray());

            // To preserve ACID properties, only perform the PUT if the value stored in the DB
            // for the row/col/qualifier is the same as at the time of the READ.
            // Note: if value is null, this method checks for cell non-existence.
            boolean success = table.checkAndPut(
                                  Bytes.toBytes(testName), STATUS_FAMILY,
                                  DATA_QUALIFIER,
                                  value, put);

            // Send emails if the PUT operation succeeds (i.e. another job did not already
            // process the same data)
            if (success) {
                for (Message msg: messageQueue) {
                    try {
                        Transport.send(msg);
                    } catch (MessagingException e) {
                        logger.error("Error sending email : ", e);
                    }
                }
            }
        }
    }
}
