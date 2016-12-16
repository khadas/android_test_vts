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

package com.android.vts.servlet;

import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import com.android.vts.helpers.BigtableHelper;
import com.android.vts.proto.VtsReportMessage;
import com.android.vts.proto.VtsReportMessage.AndroidDeviceInfoMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseReportMessage;
import com.android.vts.proto.VtsReportMessage.TestCaseResult;
import com.android.vts.proto.VtsReportMessage.TestReportMessage;
import com.android.vts.proto.VtsWebStatusMessage;
import com.android.vts.proto.VtsWebStatusMessage.TestStatus;
import com.android.vts.proto.VtsWebStatusMessage.TestStatusMessage;
import com.android.vts.proto.VtsWebStatusMessage.TestStatusMessage.Builder;
import com.google.protobuf.ByteString;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.logging.Level;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeMessage;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Represents the notifications service which is automatically called on a fixed schedule.
 */
public class VtsAlertJobServlet extends BaseServlet {

    private static final byte[] RESULTS_FAMILY = Bytes.toBytes("test");
    private static final byte[] TEST_FAMILY = Bytes.toBytes("test_to_email");
    private static final byte[] STATUS_FAMILY = Bytes.toBytes("status");
    private static final byte[] DATA_QUALIFIER = Bytes.toBytes("data");
    private static final byte[] TIME_QUALIFIER = Bytes.toBytes("upload_timestamp");
    private static final String STATUS_TABLE = "vts_status_table";
    private static final String VTS_EMAIL_NAME = "VTS Alert Bot";
    private static final long MILLI_TO_MICRO = 1000;  // conversion factor from milli to micro units
    private static final long THREE_MINUTES = 180000000L;  // units microseconds

    /**
     * Fetches the list of subscriber email addresses for a test.
     * @param statusTable The Table instance for the VTS status table.
     * @param tableName The name of the table of the test for which to fetch the email addresses.
     * @returns List of email addresses (String).
     * @throws IOException
     */
    public List<String> getSubscriberEmails(Table statusTable, String tableName)
            throws IOException {
        Get get = new Get(Bytes.toBytes(tableName));
        get.addFamily(TEST_FAMILY);
        Result result = statusTable.get(get);
        Set<String> emailSet = new HashSet<>();
        if (!StringUtils.isBlank(DEFAULT_EMAIL)) {
            emailSet.add(DEFAULT_EMAIL);
        }
        if (result != null) {
            List<Cell> cells = result.listCells();
            if (cells != null) {
                for (Cell cell : cells) {
                    String email = Bytes.toString(cell.getQualifierArray());
                    String val = Bytes.toString(cell.getValueArray());
                    if (email != null && val.equals("1") && email.endsWith(EMAIL_DOMAIN)) {
                        emailSet.add(email);
                    }
                }
            }
        }
        return new ArrayList<>(emailSet);
    }

    /**
     * Sends an email to the specified email address to notify of a test status change.
     * @param emails List of subscriber email addresses (byte[]) to which the email should be sent.
     * @param subject The email subject field, string.
     * @param body The html (string) body to send in the email.
     * @returns The Message object to be sent.
     * @throws MessagingException, UnsupportedEncodingException
     */
    public Message composeEmail(List<String> emails, String subject, String body)
            throws MessagingException, UnsupportedEncodingException {
        if (emails.size() == 0) {
            throw new MessagingException("No subscriber email addresses provided");
        }
        Properties props = new Properties();
        Session session = Session.getDefaultInstance(props, null);

        Message msg = new MimeMessage(session);
        for (String email : emails) {
            try {
                msg.addRecipient(Message.RecipientType.TO,
                                 new InternetAddress(email, email));
            } catch (MessagingException | UnsupportedEncodingException e) {
                // Gracefully continue when a subscriber email is invalid.
                logger.log(Level.WARNING, "Error sending email to recipient " + email + " : ", e);
            }
        }
        msg.setFrom(new InternetAddress(SENDER_EMAIL, VTS_EMAIL_NAME));
        msg.setSubject(subject);
        msg.setContent(body, "text/html; charset=utf-8");
        return msg;
    }

    /**
     * Checks whether any new failures have occurred beginning since (and including) startTime.
     * @param tableName The name of the table that stores the results for a test, string.
     * @param link The string URL linking to the test's status table.
     * @param lastUploadTimestamp The timestamp (long) representing when test data was last updated.
     * @param prevStatusMessage The raw (byte[]) TestStatusMessage for the previous iteration.
     * @param emails The list of email addresses to send notifications to.
     * @param messages The email Message queue.
     * @returns latest raw TestStatusMessage (byte[]).
     * @throws IOException
     */
    public byte[] getTestStatus(String tableName, String link, long lastUploadTimestamp,
                                byte[] prevStatusMessage, List<String> emails,
                                List<Message> messages) throws IOException {
        Scan scan = new Scan();
        long startTime = lastUploadTimestamp - ONE_DAY;
        Set<ByteString> failedTestcases = new HashSet<>();
        String footer = "<br><br>For details, visit the"
                        + " <a href='" + link + "'>"
                        + "VTS dashboard.</a>";
        if (prevStatusMessage != null) {
            TestStatusMessage testStatusMessage = VtsWebStatusMessage.TestStatusMessage
                                                      .parseFrom(prevStatusMessage);
            long statusTimestamp = testStatusMessage.getStatusTimestamp();
            if (lastUploadTimestamp <= statusTimestamp) {
                long now = System.currentTimeMillis() * MILLI_TO_MICRO;
                long diff = now - lastUploadTimestamp;
                // Send an email daily to notify that the test hasn't been running.
                // After 7 full days have passed, notifications will no longer be sent (i.e. the
                // test is assumed to be deprecated).
                if (diff > ONE_DAY && diff < ONE_DAY * 8 && diff % ONE_DAY < THREE_MINUTES) {
                    String test = tableName.substring(TABLE_PREFIX.length());
                    String uploadTimeString =
                            new SimpleDateFormat("MM/dd/yyyy HH:mm:ss")
                            .format(new Date(lastUploadTimestamp / MILLI_TO_MICRO));
                    String subject = "Warning! Inactive test: " + test;
                    String body = "Hello,<br><br>Test \"" + test + "\" is inactive. "
                            + "No new data has been uploaded since "
                            + uploadTimeString + "."
                            + footer;
                    try {
                        messages.add(composeEmail(emails, subject, body));
                    } catch (MessagingException | UnsupportedEncodingException e) {
                        logger.log(Level.WARNING, "Error composing email : ", e);
                    }
                }
                return testStatusMessage.toByteArray();
            }
            startTime = statusTimestamp + 1;
            failedTestcases.addAll(testStatusMessage.getFailedTestcasesList());
        }
        scan.setStartRow(Long.toString(startTime).getBytes());
        TableName tableNameObject = TableName.valueOf(tableName);
        Table table = BigtableHelper.getTable(tableNameObject);
        if (!BigtableHelper.getConnection().getAdmin().tableExists(tableNameObject)){
            return null;
        }
        ResultScanner scanner = table.getScanner(scan);
        TestReportMessage mostRecentReport = null;
        Set<ByteString> failingTestcases = new HashSet<>();
        Set<String> fixedTestcases = new HashSet<>();
        Set<String> newTestcaseFailures = new HashSet<>();
        Set<String> continuedTestcaseFailures = new HashSet<>();
        Set<String> skippedTestcaseFailures = new HashSet<>();
        Set<String> transientTestcaseFailures = new HashSet<>();
        Map<String, TestCaseResult> mostRecentTestCaseResults = new HashMap<>();
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
                mostRecentReport = testReportMessage;
            } catch (NumberFormatException e) {
                /* skip a non-post-submit build */
                continue;
            }

            for (TestCaseReportMessage testCaseReportMessage :
                 testReportMessage.getTestCaseList()) {
                TestCaseResult res = testCaseReportMessage.getTestResult();
                String name = testCaseReportMessage.getName().toStringUtf8();
                if (res != TestCaseResult.TEST_CASE_RESULT_PASS
                    && res != TestCaseResult.TEST_CASE_RESULT_SKIP) {
                    // Test case failed (not a pass or a skip).
                    transientTestcaseFailures.add(name);
                    mostRecentTestCaseResults.put(name, TestCaseResult.TEST_CASE_RESULT_FAIL);
                } else if (res == TestCaseResult.TEST_CASE_RESULT_PASS) {
                    // Test case passed.
                    mostRecentTestCaseResults.put(name, TestCaseResult.TEST_CASE_RESULT_PASS);
                }
            }
        }
        scanner.close();
        if (mostRecentReport == null) return null;

        for (TestCaseReportMessage testCaseReportMessage : mostRecentReport.getTestCaseList()) {
            String name = testCaseReportMessage.getName().toStringUtf8();
            TestCaseResult mostRecentResult = mostRecentTestCaseResults.get(name);
            boolean previouslyFailed = failedTestcases.contains(testCaseReportMessage.getName());
            if (mostRecentResult == null) {
                // Consistently skipped; preserve the state from the last update.
                if (previouslyFailed) {
                    failingTestcases.add(testCaseReportMessage.getName());
                    skippedTestcaseFailures.add(name);
                }
            } else if (mostRecentResult == TestCaseResult.TEST_CASE_RESULT_PASS) {
                // Test case most recently passed.
                if (previouslyFailed && !transientTestcaseFailures.contains(name)) {
                    // Test case fixed since last update and did not fail transiently.
                    fixedTestcases.add(name);
                }
            } else {
                // Test case failed.
                failingTestcases.add(testCaseReportMessage.getName());
                if (!previouslyFailed) {
                    // Test case previously passed.
                    newTestcaseFailures.add(name);
                } else {
                    // Continued test failure.
                    continuedTestcaseFailures.add(name);
                }
                transientTestcaseFailures.remove(name);
            }
        }

        String test = mostRecentReport.getTest().toStringUtf8();
        List<String> buildIdList = new ArrayList<>();
        for (AndroidDeviceInfoMessage device : mostRecentReport.getDeviceInfoList()) {
            buildIdList.add(device.getBuildId().toStringUtf8());
        }
        String buildId = StringUtils.join(buildIdList, ",");
        String summary = new String();
        TestStatus newStatus = TestStatus.TEST_OK;
        if (newTestcaseFailures.size() + continuedTestcaseFailures.size() > 0) {
            summary += "The following test cases failed in the latest test run:<br>";

            // Add new test case failures to top of summary in bold font.
            List<String> sortedNewTestcaseFailures = new ArrayList<>(newTestcaseFailures);
            Collections.sort(sortedNewTestcaseFailures);
            for (String testcaseName : sortedNewTestcaseFailures) {
                summary += "- " + "<b>" + testcaseName + "</b><br>";
            }

            // Add continued test case failures to summary.
            List<String> sortedContinuedTestcaseFailures =
                    new ArrayList<>(continuedTestcaseFailures);
            Collections.sort(sortedContinuedTestcaseFailures);
            for (String testcaseName : sortedContinuedTestcaseFailures) {
                summary += "- " + testcaseName + "<br>";
            }
            newStatus = TestStatus.TEST_FAIL;
        }
        if (fixedTestcases.size() > 0) {
            // Add fixed test cases to summary.
            summary += "<br><br>The following test cases were fixed in the latest test run:<br>";
            List<String> sortedFixedTestcases = new ArrayList<>(fixedTestcases);
            Collections.sort(sortedFixedTestcases);
            for (String testcaseName : sortedFixedTestcases) {
                summary += "- <i>" + testcaseName + "</i><br>";
            }
        }
        if (transientTestcaseFailures.size() > 0) {
            // Add transient test case failures to summary.
            summary += "<br><br>The following transient test case failures occured:<br>";
            List<String> sortedTransientTestcaseFailures =
                    new ArrayList<>(transientTestcaseFailures);
            Collections.sort(sortedTransientTestcaseFailures);
            for (String testcaseName : sortedTransientTestcaseFailures) {
                summary += "- " + testcaseName + "<br>";
            }
        }
        if (skippedTestcaseFailures.size() > 0) {
            // Add skipped test case failures to summary.
            summary += "<br><br>The following test cases have not been run since failing:<br>";
            List<String> sortedSkippedTestcaseFailures =
                    new ArrayList<>(skippedTestcaseFailures);
            Collections.sort(sortedSkippedTestcaseFailures);
            for (String testcaseName : sortedSkippedTestcaseFailures) {
                summary += "- " + testcaseName + "<br>";
            }
        }

        if (newTestcaseFailures.size() > 0) {
            String subject = "New test failures in " + test + " @ " + buildId;
            String body = "Hello,<br><br>Test cases are failing in " + test
                          + " for device build ID(s): " + buildId + ".<br><br>"
                          + summary + footer;
            try {
                messages.add(composeEmail(emails, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        } else if (continuedTestcaseFailures.size() > 0) {
            String subject = "Continued test failures in " + test + " @ " + buildId;
            String body = "Hello,<br><br>Test cases are failing in " + test
                          + " for device build ID(s): " + buildId + ".<br><br>"
                          + summary + footer;
            try {
                messages.add(composeEmail(emails, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        } else if (transientTestcaseFailures.size() > 0) {
            String subject = "Transient test failure in " + test + " @ " + buildId;
            String body = "Hello,<br><br>Some test cases failed in " + test + " but tests all "
                          + "are passing in the latest device build(s): " + buildId + ".<br><br>"
                          + summary + footer;
            try {
                messages.add(composeEmail(emails, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        } else if (fixedTestcases.size() > 0) {
            String subject = "All test cases passing in " + test + " @ " + buildId;
            String body = "Hello,<br><br>All test cases passed in " + test
                          + " for device build ID(s): " + buildId + "!<br><br>"
                          + summary + footer;
            try {
                messages.add(composeEmail(emails, subject, body));
            } catch (MessagingException | UnsupportedEncodingException e) {
                logger.log(Level.WARNING, "Error composing email : ", e);
            }
        }
        Builder builder = VtsWebStatusMessage.TestStatusMessage.newBuilder();
        builder.setStatusTimestamp(lastUploadTimestamp);
        builder.setStatus(newStatus);
        builder.addAllFailedTestcases(failingTestcases);
        return builder.build().toByteArray();
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        doGetHandler(request, response);
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        Table table = BigtableHelper.getTable(TableName.valueOf(STATUS_TABLE));
        Scan scan = new Scan();
        scan.addFamily(STATUS_FAMILY);
        ResultScanner scanner = table.getScanner(scan);
        for (Result result = scanner.next(); result != null; result = scanner.next()) {
            String testName = Bytes.toString(result.getRow());
            List<String> emails = getSubscriberEmails(table, testName);
            byte[] value = result.getValue(STATUS_FAMILY, DATA_QUALIFIER);

            // Get the time stamp for the last upload.
            long lastUploadTimestamp;
            try {
                lastUploadTimestamp = Long.parseLong(Bytes.toString(
                    result.getValue(STATUS_FAMILY, TIME_QUALIFIER)));
            } catch (NumberFormatException e) {
                // If no upload timestamp, skip this row.
                logger.log(Level.WARNING, "Error parsing upload timestamp: ", e);
                continue;
            }
            List<Message> messageQueue = new ArrayList<>();
            StringBuffer fullUrl = request.getRequestURL();
            String baseUrl = fullUrl.substring(0, fullUrl.indexOf(request.getRequestURI()));
            String link = baseUrl + "/show_table?testName=" +
                          testName.substring(TABLE_PREFIX.length(), testName.length());
            byte[] newStatus = getTestStatus(testName, link, lastUploadTimestamp, value, emails,
                                             messageQueue);

            if (newStatus != null) {
                // Create row insertion operation.
                Put put = new Put(result.getRow());

                // ACID properties enforced by using full data upload timestamp as version number.
                // If multiple alert jobs execute the update at the same time, the one with the
                // freshest data will update the status to the latest version.
                put.addColumn(STATUS_FAMILY, DATA_QUALIFIER, lastUploadTimestamp, newStatus);
                table.put(put);

                for (Message msg: messageQueue) {
                    try {
                        Transport.send(msg);
                    } catch (MessagingException e) {
                        logger.log(Level.WARNING, "Error sending email : ", e);
                    }
                }
            }
        }
    }
}
