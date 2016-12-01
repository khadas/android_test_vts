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

package com.android.vts.test;

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
 * This class tests the behavior of elements on the ShowTableServlet.java or show_table.jsp.
 */
public class ShowTableServletTest extends TestCase {

    private static final String LOCALHOST_PROPERTY = "LOCALHOST";
    private static final String CONFIG_FILE = "config.properties";
    private static final String HOMEPAGE_TABLE_ID = "dashboard_main_table";
    private static final String LOGIN_BUTTON_ID = "submit-login";
    private static final String PREVIOUS_BUTTON_ID = "previous_button";
    private static WebDriver driver = null;
    private static Properties properties = null;
    private static InputStream input = null;

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
     * This test checks if the previous button is disabled on the page 0.
     */
    public void testPreviousButtonOnFirstPage() {
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
            WebElement previousButton = driver.findElement(By.id(PREVIOUS_BUTTON_ID));
            Assert.assertEquals("true", previousButton.getAttribute("disabled"));
            driver.navigate().back();
        }
    }
}
