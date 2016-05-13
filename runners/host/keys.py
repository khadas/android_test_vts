#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import enum

"""This module has the global key values that are used across framework
modules.
"""
class Config(enum.Enum):
    """Enum values for test config related lookups.
    """
    # Keys used to look up values from test config files.
    # These keys define the wording of test configs and their internal
    # references.
    KEP_LOG_PATH = "log_path"
    KEY_TESTBED = "test_bed"
    KEY_TESTBED_NAME = "name"
    KEY_TEST_PATHS = "test_paths"

    # Internal keys, used internally, not exposed to user's config files.
    IKEY_USER_PARAM = "user_params"
    IKEY_TESTBED_NAME = "testbed_name"
    IKEY_LOG_PATH = "log_path"

    # A list of keys whose values in configs should not be passed to test
    # classes without unpacking first.
    RESERVED_KEYS = (KEY_TESTBED, KEP_LOG_PATH, KEY_TEST_PATHS)
