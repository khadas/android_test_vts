#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# for platform/bionic/tests
vts_test_lib_packages := \
	libBionicStandardTests \
	libfortify1-tests-gcc \
	libfortify2-tests-gcc \
	libfortify1-tests-clang \
	libfortify2-tests-clang \
	libBionicTests \
	libBionicGtestMain \
	libBionicLoaderTests \
	libBionicCtsGtestMain \

# for platform/bionic/tests/libs
vts_test_lib_packages += \
	libgnu-hash-table-library \
	libsysv-hash-table-library \
	libdlext_test_norelro \
	libtest_simple \
	libtest_nodelete_1 \
	libtest_nodelete_2 \
	libtest_nodelete_dt_flags_1 \
	libtest_with_dependency_loop \
	libtest_with_dependency_loop_a \
	libtest_with_dependency_loop_b_tmp \
	libtest_with_dependency_loop_b \
	libtest_with_dependency_loop_c \
	libtest_relo_check_dt_needed_order \
	libtest_relo_check_dt_needed_order_1 \
	libtest_relo_check_dt_needed_order_2 \
	libtest_ifunc \
	libtest_atexit \
	libdl_preempt_test_1 \
	libdl_preempt_test_2 \
	libdl_test_df_1_global \
	libtest_dlsym_df_1_global \
	libtest_dlsym_weak_func \
	libtest_dlsym_from_this \
	libtest_dlsym_from_this_child \
	libtest_dlsym_from_this_grandchild \
	libtest_empty \
	libtest_dlopen_weak_undefined_func \
	libtest_dlopen_from_ctor \
	libtest_dlopen_from_ctor_main \
