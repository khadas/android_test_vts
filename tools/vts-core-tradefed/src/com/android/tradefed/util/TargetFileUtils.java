/*
 * Copyright (C) 2020 The Android Open Source Project
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

/* Utils for target file.*/
package com.android.tradefed.util;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.log.LogUtil.CLog;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class TargetFileUtils {
    /**
     * Helper method which executes a adb shell find command and returns the results as an {@link
     * ArrayList<String>}.
     *
     * @param path The path to search on device.
     * @param namePattern The file name pattern.
     * @param options A {@link List} of {@link String} for other options pass to find.
     * @param device The test device.
     * @return The result in {@link ArrayList<String>}.
     * @throws DeviceNotAvailableException if connection with device is lost and cannot be
     *     recovered.
     */
    public static ArrayList<String> findFile(String path, String namePattern, List<String> options,
            ITestDevice device) throws DeviceNotAvailableException {
        ArrayList<String> findedFiles = new ArrayList<>();
        String command = String.format("find %s -name \"%s\"", path, namePattern);
        if (options != null) {
            command += " " + String.join(" ", options);
        }
        CLog.d("command: %s", command);
        CommandResult result = device.executeShellV2Command(command);
        if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
            CLog.e("Find command: '%s' failed, returned:\nstdout:%s\nstderr:%s", command,
                    result.getStdout(), result.getStderr());
            return findedFiles;
        }
        findedFiles = new ArrayList<>(Arrays.asList(result.getStdout().split("\n")));
        findedFiles.removeIf(s -> s.contentEquals(""));
        return findedFiles;
    }
}
