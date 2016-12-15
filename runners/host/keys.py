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

"""This module has the global key values that are used across framework
modules.
"""


class ConfigKeys(object):
    """Enum values for test config related lookups.
    """
    # Keys used to look up values from test config files.
    # These keys define the wording of test configs and their internal
    # references.
    KEY_LOG_PATH = "log_path"
    KEY_TESTBED = "test_bed"
    KEY_TESTBED_NAME = "name"
    KEY_TEST_PATHS = "test_paths"
    KEY_TEST_SUITE = "test_suite"

    # Keys in test suite
    KEY_INCLUDE_FILTER = "include_filter"
    KEY_EXCLUDE_FILTER = "exclude_filter"

    # Keys for binary tests
    IKEY_BINARY_TEST_SOURCES = "binary_test_sources"
    IKEY_BINARY_TEST_WORKING_DIRECTORIES = "binary_test_working_directories"
    IKEY_BINARY_TEST_LD_LIBRARY_PATHS = "binary_test_ld_library_paths"

    # Internal keys, used internally, not exposed to user's config files.
    IKEY_USER_PARAM = "user_params"
    IKEY_TESTBED_NAME = "testbed_name"
    IKEY_LOG_PATH = "log_path"

    IKEY_BUILD = "build"
    IKEY_DATA_FILE_PATH = "data_file_path"

    # sub fields of test_bed
    IKEY_PRODUCT_TYPE = "product_type"
    IKEY_PRODUCT_VARIANT = "product_variant"
    IKEY_BUILD_FLAVOR = "build_flavor"
    IKEY_BUILD_ID = "build_id"
    IKEY_BRANCH = "branch"
    IKEY_BUILD_ALIAS = "build_alias"
    IKEY_API_LEVEL = "api_level"
    IKEY_SERIAL = "serial"

    # A list of keys whose values in configs should not be passed to test
    # classes without unpacking first.
    RESERVED_KEYS = (KEY_TESTBED, KEY_LOG_PATH, KEY_TEST_PATHS)
