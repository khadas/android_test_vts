/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tradefed.util;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;

import java.util.concurrent.TimeUnit;
import java.util.function.Predicate;

public class CmdUtil {
    static final int MAX_RETRY_COUNT = 10;
    static final int DELAY_BETWEEN_RETRY_IN_SECS = 3;

    // An interface to wait with delay. Used for testing purpose.
    public interface ISleeper { public void sleep(int seconds) throws InterruptedException; }
    private ISleeper mSleeper = null;

    /**
     * Helper method to retry given cmd until the expected results are satisfied.
     * An example usage it to retry 'lshal' until the expected hal service appears.
     *
     * @param device testing device.
     * @param cmd the command string to be executed on device.
     * @param predicate function that checks the exit condition.
     * @return true if the exit condition is satisfied, false otherwise.
     * @throws DeviceNotAvailableException
     */
    public boolean waitCmdResultWithDelay(ITestDevice device, String cmd,
            Predicate<String> predicate) throws DeviceNotAvailableException {
        for (int count = 0; count < MAX_RETRY_COUNT; count++) {
            String out = device.executeShellCommand(cmd);
            CLog.i("cmd output: %s", out);
            if (predicate.test(out)) {
                CLog.i("Exit condition satisfied.");
                return true;
            } else {
                CLog.i("Exit condition not satisfied. Waiting %s seconds",
                        DELAY_BETWEEN_RETRY_IN_SECS);
                try {
                    if (mSleeper != null) {
                        mSleeper.sleep(DELAY_BETWEEN_RETRY_IN_SECS);
                    } else {
                        TimeUnit.SECONDS.sleep(DELAY_BETWEEN_RETRY_IN_SECS);
                    }
                } catch (InterruptedException ex) {
                    /* pass */
                }
            }
        }
        return false;
    }

    @VisibleForTesting
    void setSleeper(ISleeper sleeper) {
        mSleeper = sleeper;
    }
}
