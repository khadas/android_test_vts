#!/usr/bin/env python3.4
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
#

import inspect
import logging
import os
import re
import subprocess
import sys

from vts.runners.host import asserts
from vts.runners.host import base_test_with_webdb
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device


class CameraITSTest(base_test_with_webdb.BaseTestWithWebDbClass):
    """Running CameraITS tests in VTS"""

    # TODO: use config file to pass in:
    #          - serial for dut and screen
    #          - camera id
    #       so that we can run other test scenes with ITS-in-a-box
    def setUpClass(self):
        """Setup ITS running python environment and check for required python modules
        """
        self.dut = self.registerController(android_device)[0]
        self.device_arg = "device=%s" % (self.dut.serial)
        # data_file_path is unicode so convert it to ascii
        self.its_path = str(os.path.join(self.data_file_path, 'CameraITS'))
        self.out_path = logging.log_path
        os.environ["CAMERA_ITS_TOP"] = self.its_path
        # Below module check code assumes tradefed is running python 2.7
        # If tradefed switches to python3, then we will be checking modules in python3 while ITS
        # scripts should be ran in 2.7.
        if sys.version_info[:2] != (2, 7):
            logging.warning(
                "Python version %s found; "
                "CameraITSTest only tested with Python 2.7." % (
                    str(sys.version_info[:3])))
        print "==============================="
        print "Python path is:", sys.executable
        import matplotlib
        print "matplotlib version is " + matplotlib.__version__
        print "matplotlib path is " + inspect.getfile(matplotlib)
        print "==============================="
        modules = ["numpy", "PIL", "Image", "matplotlib", "pylab",
                   "scipy.stats", "scipy.spatial"]
        for m in modules:
            try:
                if m == "Image":
                    # Image modules are now imported from PIL
                    exec ("from PIL import Image")
                elif m == "pylab":
                    exec ("from matplotlib import pylab")
                else:
                    exec ("import " + m)
            except ImportError:
                asserts.fail("Cannot found python module " + m)

        # Add ITS module path to path
        self.pythonpath = "%s/pymodules:%s" % (self.its_path,
                                               os.environ["PYTHONPATH"])
        os.environ["PYTHONPATH"] = self.pythonpath

    def RunTestcase(self, testpath):
        """Runs the given testcase and asserts the result.

        Args:
            testpath: string, format tests/[scenename]/[testname].py
        """
        testname = re.split("/|\.", testpath)[-2]
        cmd = ['python', os.path.join(self.its_path, testpath),
               self.device_arg]
        outdir = self.out_path
        outpath = os.path.join(outdir, testname + "_stdout.txt")
        errpath = os.path.join(outdir, testname + "_stderr.txt")
        with open(outpath, "w") as fout, open(errpath, "w") as ferr:
            retcode = subprocess.call(
                cmd, stderr=ferr, stdout=fout, cwd=outdir)
        if retcode != 0 and retcode != 101:
            # Dump all logs to host log if the test failed
            with open(outpath, "r") as fout, open(errpath, "r") as ferr:
                logging.info(fout.read())
                logging.error(ferr.read())

        asserts.assertTrue(retcode == 0 or retcode == 101,
                           "ITS %s retcode %d" % (testname, retcode))

    def FetchTestPaths(self, scene):
        """Returns a list of test paths for a given test scene.

        Args:
            scnee: one of ITS test scene name.
        """
        its_path = self.its_path
        paths = [os.path.join("tests", scene, s)
                 for s in os.listdir(os.path.join(its_path, "tests", scene))
                 if s[-3:] == ".py" and s[:4] == "test"]
        paths.sort()
        return paths

    def generateScene0Test(self):
        testpaths = self.FetchTestPaths("scene0")
        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=testpaths,
            name_func=lambda path: "%s_%s" % (re.split("/|\.", path)[-3], re.split("/|\.", path)[-2]))


if __name__ == "__main__":
    test_runner.main()
