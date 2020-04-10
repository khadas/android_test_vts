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

package com.android.compatibility.tradefed.util;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.TargetFileUtils;
import java.util.ArrayList;
import java.util.Arrays;
import org.easymock.EasyMock;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class TargetFileUtilsTest {
    private ITestDevice mMockDevice;

    /** Test {@link TargetFileUtils#findFile()} with NumberFormatException be asserted. */
    @Test
    public void testFindFile() throws DeviceNotAvailableException {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        CommandResult commandResult = new CommandResult(CommandStatus.SUCCESS);
        commandResult.setStdout("path1\npath2\npath3\n");
        commandResult.setExitCode(0);
        EasyMock.expect(mMockDevice.executeShellV2Command("find findPath -name \"namePattern\" "
                                + "option1 option2"))
                .andReturn(commandResult);
        EasyMock.replay(mMockDevice);
        ArrayList<String> findPaths = new ArrayList<>();
        findPaths.add("path1");
        findPaths.add("path2");
        findPaths.add("path3");
        String[] options = {"option1", "option2"};
        assertEquals(findPaths,
                TargetFileUtils.findFile(
                        "findPath", "namePattern", Arrays.asList(options), mMockDevice));
    }

    /** Test {@link TargetFileUtils#findFile()} with shell command failed. */
    @Test
    public void testFindFile_w_cmd_result_fail() throws DeviceNotAvailableException {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        CommandResult commandResult = new CommandResult(CommandStatus.FAILED);
        commandResult.setStdout("path1\npath2\npath3\n");
        commandResult.setExitCode(0);
        EasyMock.expect(mMockDevice.executeShellV2Command("find findPath -name \"namePattern\""))
                .andReturn(commandResult);
        EasyMock.replay(mMockDevice);
        ArrayList<String> findPaths = new ArrayList<>();
        assertEquals(
                findPaths, TargetFileUtils.findFile("findPath", "namePattern", null, mMockDevice));
    }

    /** Test {@link TargetFileUtils#findFile()} which have stdout with empty line. */
    @Test
    public void testFindFile_w_empty_line_stdout() throws DeviceNotAvailableException {
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        CommandResult commandResult = new CommandResult(CommandStatus.SUCCESS);
        commandResult.setStdout("");
        commandResult.setExitCode(0);
        EasyMock.expect(mMockDevice.executeShellV2Command("find findPath -name \"namePattern\""))
                .andReturn(commandResult);
        EasyMock.replay(mMockDevice);
        ArrayList<String> findPaths = new ArrayList<>();
        assertEquals(
                findPaths, TargetFileUtils.findFile("findPath", "namePattern", null, mMockDevice));
    }
}
