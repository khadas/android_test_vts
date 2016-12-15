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

import org.apache.commons.lang.StringUtils;
import org.apache.hadoop.hbase.Cell;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.util.Bytes;
import java.util.logging.Logger;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.logging.Level;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeMessage;


/**
 * EmailHelper, a helper class for building and sending emails.
 **/
public class EmailHelper {

    protected static final Logger logger = Logger.getLogger(EmailHelper.class.getName());
    protected static final String DEFAULT_EMAIL = System.getenv("DEFAULT_EMAIL");
    protected static final String EMAIL_DOMAIN = System.getenv("EMAIL_DOMAIN");
    protected static final String SENDER_EMAIL = System.getenv("SENDER_EMAIL");
    private static final byte[] TEST_FAMILY = Bytes.toBytes("test_to_email");
    private static final String VTS_EMAIL_NAME = "VTS Alert Bot";

    /**
     * Fetches the list of subscriber email addresses for a test.
     * @param statusTable The Table instance for the VTS status table.
     * @param tableName The name of the table of the test for which to fetch the email addresses.
     * @returns List of email addresses (String).
     * @throws IOException
     */
    public static List<String> getSubscriberEmails(Table statusTable, String tableName)
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
    public static Message composeEmail(List<String> emails, String subject, String body)
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
     * Sends an email.
     * @param msg Message object to send.
     * @returns true if the message sends successfully, false otherwise
     */
    public static boolean send(Message msg) {
        try {
            Transport.send(msg);
        } catch (MessagingException e) {
            logger.log(Level.WARNING, "Error sending email : ", e);
            return false;
        }
        return true;
    }

    /**
     * Sends a list of emails and logs any failures.
     * @param messages List of Message objects to be sent.
     */
    public static void sendAll(List<Message> messages) {
        for (Message msg: messages) {
            send(msg);
        }
    }

    /**
     * Sends an email.
     * @param recipients List of email address strings to which an email will be sent.
     * @param subject The subject of the email.
     * @param body The body of the email.
     * @returns true if the message sends successfully, false otherwise
     */
    public static boolean send(List<String> recipients, String subject, String body) {
        try {
            Message msg = composeEmail(recipients, subject, body);
            return send(msg);
        } catch (MessagingException | UnsupportedEncodingException e) {
            logger.log(Level.WARNING, "Error composing email : ", e);
            return false;
        }
    }
}
