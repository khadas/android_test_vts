package com.android.tradefed.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.*;

import com.android.tradefed.device.ITestDevice;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.function.Predicate;

/**
 * Unit tests for {@link CmdUtil}.
 */
@RunWith(JUnit4.class)
public class CmdUtilTest {
    static final String TEST_CMD = "test cmd";

    class MockSleeper implements CmdUtil.ISleeper {
        @Override
        public void sleep(int seconds) throws InterruptedException {
            return;
        }
    };

    CmdUtil mCmdUtil = null;

    // Predicates to stop retrying cmd.
    private Predicate<String> mCheckEmpty = (String str) -> {
        return str.isEmpty();
    };

    @Mock private ITestDevice mDevice;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        CmdUtil.ISleeper msleeper = new MockSleeper();
        mCmdUtil = new CmdUtil();
        mCmdUtil.setSleeper(msleeper);
    }

    @Test
    public void testWaitCmdSuccess() throws Exception {
        doReturn("").when(mDevice).executeShellCommand(TEST_CMD);
        assertTrue(mCmdUtil.waitCmdResultWithDelay(mDevice, TEST_CMD, mCheckEmpty));
    }

    @Test
    public void testWaitCmdSuccessWithRetry() throws Exception {
        when(mDevice.executeShellCommand(TEST_CMD)).thenReturn("something").thenReturn("");
        assertTrue(mCmdUtil.waitCmdResultWithDelay(mDevice, TEST_CMD, mCheckEmpty));
    }

    @Test
    public void testWaitCmdSuccessFail() throws Exception {
        doReturn("something").when(mDevice).executeShellCommand(TEST_CMD);
        assertFalse(mCmdUtil.waitCmdResultWithDelay(mDevice, TEST_CMD, mCheckEmpty));
    }
}
