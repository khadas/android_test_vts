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
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.chrome.ChromeDriver;

/**
 * This class tests the behavior of elements on the home screen of VTS Dashboard.
 */
public class DashboardMainServletTest extends TestCase {

    private static final String LOCALHOST = "localhost:8080";
    private static final String ASSERT_MESSAGE = "Title should start differently.";
    private static final String HOMEPAGE_TITLE = "VTS Dashboard";
    private static final String LOGIN_BUTTON_ID = "submit-login";
    public static WebDriver driver = null;

    /**
     * Runs once before running tests.
     */
    @Override
    public void setUp() {
        driver = new ChromeDriver();
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
        driver.navigate().to(LOCALHOST);
        driver.navigate().refresh();
        driver.findElement(By.id(LOGIN_BUTTON_ID)).click();
        Assert.assertTrue(ASSERT_MESSAGE, driver.getTitle().equals(HOMEPAGE_TITLE));
    }
}
