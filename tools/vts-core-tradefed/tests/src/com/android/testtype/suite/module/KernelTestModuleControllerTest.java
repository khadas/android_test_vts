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
package com.android.tradefed.testtype.suite.module;

import static org.junit.Assert.assertEquals;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.invoker.InvocationContext;
import com.android.tradefed.testtype.suite.ModuleDefinition;
import com.android.tradefed.testtype.suite.module.IModuleController.RunStrategy;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for {@link KernelTestModuleControllerTest}.
 */
@RunWith(JUnit4.class)
public class KernelTestModuleControllerTest {
    private KernelTestModuleController mController;
    private IInvocationContext mContext;

    @Before
    public void setUp() {
        mController = new KernelTestModuleController();
        mContext = new InvocationContext();
        mContext.addInvocationAttribute(ModuleDefinition.MODULE_NAME, "module1");
    }

    @Test
    public void testModuleAbiMatchesArch() throws ConfigurationException {
        mContext.addInvocationAttribute(ModuleDefinition.MODULE_ABI, "arm64-v8a");
        OptionSetter setter = new OptionSetter(mController);
        setter.setOptionValue("arch", "arm64");
        assertEquals(RunStrategy.RUN, mController.shouldRunModule(mContext));
    }

    @Test
    public void testModuleAbiMismatchesArch() throws ConfigurationException {
        mContext.addInvocationAttribute(ModuleDefinition.MODULE_ABI, "arm64-v8a");
        OptionSetter setter = new OptionSetter(mController);
        setter.setOptionValue("arch", "arm");
        assertEquals(RunStrategy.FULL_MODULE_BYPASS, mController.shouldRunModule(mContext));
    }
}
