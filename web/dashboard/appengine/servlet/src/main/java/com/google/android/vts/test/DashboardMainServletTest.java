/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.vts.test;

import junit.framework.Assert;
import junit.framework.TestCase;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.chrome.ChromeDriver;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

/**
 * This class tests the behavior of elements on the home screen of VTS Dashboard.
 */
public class DashboardMainServletTest extends TestCase {

    private static final String LOCALHOST_PROPERTY = "LOCALHOST";
    private static final String CONFIG_FILE = "config.properties";
    private static final String ASSERT_MESSAGE = "Title should start differently.";
    private static final String HOMEPAGE_TITLE = "VTS Dashboard";
    private static final String LOGIN_BUTTON_ID = "submit-login";
    private static final String HOMEPAGE_TABLE_ID = "dashboard_main_table";
    private static WebDriver driver = null;
    private static Properties properties = null;
    private static InputStream input = null;

    // constants for page two - VTS Table
    private static final String VTS_TABLE_TITLE = "VTS Table";

    /**
     * Runs once before running tests.
     */
    @Override
    public void setUp() {
        System.setProperty("webdriver.chrome.driver", "chromedriver");
        driver = new ChromeDriver();
        properties = new Properties();

        // load property file
        try {
            input =  getClass().getResourceAsStream(CONFIG_FILE);
            properties.load(input);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * Runs after the all tests are run.
     */
    @Override
    public void tearDown() {
        driver.close();
    }

    /**
     * Tests the title of Home page - VTS Dashboard.
     */
    public void testDashboardMainTitle() {
        driver.navigate().to(properties.getProperty(LOCALHOST_PROPERTY));
        driver.navigate().refresh();
        driver.findElement(By.id(LOGIN_BUTTON_ID)).click();
        Assert.assertTrue(ASSERT_MESSAGE, driver.getTitle().equals(HOMEPAGE_TITLE));
    }

    /**
     * This test clicks each of the links in the table on the home page and confirms that it
     * navigates to the second page.
     */
    public void testDashboardMainTable() {
        driver.navigate().to(properties.getProperty(LOCALHOST_PROPERTY));
        driver.navigate().refresh();
        driver.findElement(By.id(LOGIN_BUTTON_ID)).click();

        WebElement tableElement = driver.findElement(By.id(HOMEPAGE_TABLE_ID));
        List<WebElement> trCollection = tableElement.findElements(By.tagName("tr"));

        List<String> linkCollection = new ArrayList<String>();

        for(WebElement trElement : trCollection) {
            try {
                linkCollection.add(trElement.findElement(By.tagName("td")).
                    findElement(By.tagName("a")).getAttribute("href"));
            } catch (NoSuchElementException e) {
                /* Ignore header elements. */
            }
        }

        for(String link : linkCollection) {
            driver.navigate().to(link);
            Assert.assertTrue(ASSERT_MESSAGE, driver.getTitle().equals(VTS_TABLE_TITLE));
            driver.navigate().back();
        }
    }

}