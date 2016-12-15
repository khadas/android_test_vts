#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import difflib
import filecmp
import getopt
import logging
import os
import shutil
import subprocess
import sys
import unittest

from vts.utils.python.common import cmd_utils

class VtscTester(unittest.TestCase):
    """Integration test runner for vtsc in generating the driver/profiler code.

    Runs vtsc with specified mode on a bunch of files and compares the output
    results with canonical ones. Exit code is 0 iff all tests pass.
    Note: need to run the script from the source root to preserve the correct
          path.

    Usage:
        python test_vtsc.py path_to_vtsc canonical_dir output_dir

    example:
        python test/vts/sysfuzzer/vtscompiler/test/test_vtsc.py vtsc
        test/vts/sysfuzzer/vtscompiler/test/data/ temp_output

    Attributes:
        _vtsc_path: the path to run vtsc.
        _canonical_dir: root directory contains canonical files for comparison.
        _output_dir: root directory that stores all output files.
        _errors: number of errors generates during the test.
    """
    def __init__(self, testName, vtsc_path, canonical_dir, output_dir):
        super(VtscTester, self).__init__(testName)
        self._vtsc_path = vtsc_path
        self._canonical_dir = canonical_dir
        self._output_dir = output_dir
        self._errors = 0

    def setUp(self):
        """Removes output dir to prevent interference from previous runs."""
        self.RemoveOutputDir()

    def tearDown(self):
        """If successful, removes the output dir for clean-up."""
        if self._errors == 0:
            self.RemoveOutputDir()

    def testAll(self):
        """Run all tests. """
        self.testProfiler()
        self.assertEqual(self._errors, 0)

    def testProfiler(self):
        """Run tests for PROFILER mode. """
        self.RunTest("PROFILER", "test/vts/specification/hal_hidl/Nfc/Nfc.vts",
                     "nfc.profiler.cpp")

    def RunTest(self, mode, vts_file_path, source_file_name):
        """Run vtsc with given mode for the give vts file and compare the
           output results.

        Args:
            mode: the vtsc mode for generated code. e.g. DRIVER / PROFILER.
            vts_file_path: path of the input vts file.
            source_file_name: name of the generated source file.
        """
        vtsc_cmd = [self._vtsc_path, "-m" + mode, vts_file_path,
                    self._output_dir,
                    os.path.join(self._output_dir, source_file_name)]
        return_code = cmd_utils.RunCommand(vtsc_cmd)
        if (return_code != 0):
            self.Error("Fail to execute command: %s" % vtsc_cmd)

        header_file_name = vts_file_path + ".h"
        canonical_header_file = os.path.join(self._canonical_dir,
                                             header_file_name)
        output_header_file = os.path.join(self._output_dir, header_file_name)

        canonical_source_file = os.path.join(self._canonical_dir,
                                             source_file_name)
        output_source_file = os.path.join(self._output_dir, source_file_name)

        self.CompareOutputFile(output_header_file, canonical_header_file)
        self.CompareOutputFile(output_source_file, canonical_source_file)

    def CompareOutputFile(self, output_file, canonical_file):
        """Compares a given file and the corresponding one under canonical_dir.

        Args:
            canonical_file: name of the canonical file.
            output_file: name of the output file.
        """
        if not os.path.isfile(canonical_file):
            self.Error("Generated unexpected file: %s" % output_file)
        else:
            if not filecmp.cmp(output_file, canonical_file):
                self.Error("output file: %s does not match the canonical_file: "
                           "%s" % (output_file, canonical_file))
                self.PrintDiffFiles(output_file, canonical_file)

    def PrintDiffFiles(self, output_file, canonical_file):
        with open(output_file, 'r') as file1:
            with open(canonical_file, 'r') as file2:
                diff = difflib.unified_diff(file1.readlines(),
                                            file2.readlines(),
                                            fromfile=output_file,
                                            tofile=canonical_file)
        for line in diff:
            logging.error(line)

    def Error(self, string):
        """Prints an error message and increments error count."""
        logging.error(string)
        self._errors += 1

    def RemoveOutputDir(self):
        """Remove the output_dir if it exists."""
        if os.path.exists(self._output_dir):
            shutil.rmtree(self._output_dir)

if __name__ == "__main__":
    # Default values of the input parameter, could be overridden by command.
    vtsc_path = "vtsc"
    canonical_dir = "test/vts/sysfuzzer/vtscompiler/test/data/"
    output_dir = "test/vts/sysfuzzer/vtscompiler/test/temp_coutput/"
    # Parse the arguments and set the provided value for
    # vtsc_path/canonical_dar/output_dir.
    try:
        opts, _ = getopt.getopt(sys.argv[1:], "p:c:o:")
    except getopt.GetoptError, err:
        print "Usage: python test_vtsc.py [-p vtsc_path] [-c canonical_dir] " \
              "[-o output_dir]"
        sys.exit(1)
    for opt, val in opts:
        if opt == "-p":
            vtsc_path = val
        elif opt == "-c":
            canonical_dir = val
        elif opt == "-o":
            output_dir = val
        else:
            print "unhandled option %s" % (opt,)
            sys.exit(1)

    suite = unittest.TestSuite()
    suite.addTest(VtscTester('testAll', vtsc_path, canonical_dir, output_dir))
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(-1)