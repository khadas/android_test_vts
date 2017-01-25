#!/usr/bin/env python
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

import os
import tempfile
import shutil
import subprocess
import logging

PATH_SYSTRACE_SCRIPT = os.path.join('tools/external/chromium-trace',
                                    'systrace.py')
EXPECTED_START_STDOUT = 'Starting tracing'


class SystraceController(object):
    '''A util to start/stop systrace through shell command.

    Attributes:
        _android_vts_path: string, path to android-vts
        _path_output: string, systrace temporally output path
        _path_systrace_script: string, path to systrace controller python script
        _subprocess: subprocess.Popen, a subprocess objects of systrace shell command
        is_valid: boolean, whether the current environment setting for
                  systrace is valid
        process_name: string, process name to trace
    '''

    def __init__(self, android_vts_path, process_name):
        self._android_vts_path = android_vts_path
        self._path_output = None
        self._path_systrace_script = os.path.join(android_vts_path,
                                                  PATH_SYSTRACE_SCRIPT)
        self.is_valid = os.path.exists(self._path_systrace_script)
        if not self.is_valid:
            logging.error('invalid systrace script path: %s',
                          self._path_systrace_script)
        self.process_name = process_name

    @property
    def is_valid(self):
        ''''returns whether the current environment setting is valid'''
        return self._is_valid

    @is_valid.setter
    def is_valid(self, is_valid):
        ''''Set valid status'''
        self._is_valid = is_valid

    def Start(self):
        '''Start systrace process.

        Use shell command to start a python systrace script

        Returns:
            True if successfully started systrace; False otherwise.
        '''
        self._subprocess = None
        self._path_output = None

        if not self.is_valid:
            logging.error(
                'Cannot start systrace: configuration is not correct for %s.',
                self.process_name)
            return False

        # TODO: check target device for compatibility (e.g. has systrace hooks)

        self._path_output = os.path.join(tempfile.mkdtemp(),
                                         self.process_name + '.html')

        cmd = ('python -u {script} hal sched '
               '-a {process_name} -o {output}').format(
                   script=self._path_systrace_script,
                   process_name=self.process_name,
                   output=self._path_output)
        process = subprocess.Popen(
            str(cmd),
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        line = ''
        success = False
        while process.poll() is None:
            line += process.stdout.read(1)

            if not line:
                break
            elif EXPECTED_START_STDOUT in line:
                success = True
                break

        if not success:
            logging.error('Failed to start systrace on process %s',
                          process_name)
            stdout, stderr = process.communicate()
            logging.error('stdout: %s', line + stdout)
            logging.error('stderr: %s', stderr)
            logging.error('ret_code: %s', process.returncode)
            return False

        self._subprocess = process
        logging.info('Systrace started for %s', self.process_name)
        return True

    def Stop(self):
        '''Stop systrace process.

        Returns:
            True if successfully stopped systrace; False otherwise.
        '''
        if not self.is_valid or not self._subprocess:
            logging.warn(
                'Cannot stop systrace: systrace was not started for %s.',
                self.process_name)
            return False

        # Press enter to stop systrace script
        self._subprocess.stdin.write('\n')
        self._subprocess.stdin.flush()

        # Wait for output to be written down
        self._subprocess.communicate()
        logging.info('Systrace stopped for %s', self.process_name)
        return True

    def ReadLastOutput(self):
        '''Read systrace output html.

        Returns:
            string, data of systrace html output. None if failed to read.
        '''
        if not self.is_valid or not self._subprocess:
            logging.warn(
                'Cannot read output: systrace was not started for %s.',
                self.process_name)
            return None

        try:
            with open(self._outputs[process_name], 'r') as f:
                data = f.read()
                logging.info('Systrace output length for %s: %s', process_name,
                             len(data))
                return data
        except Exception as e:
            logging.error('Cannot read output: file open failed, %s', e)
            return None

    def SaveLastOutput(self, report_path=None):
        if not report_path:
            logging.error('report path supplied is None')
            return False

        if not self._path_output:
            logging.error(
                'systrace did not started correctly. Output path is empty.')
            return False

        parent_dir = os.path.dirname(report_path)
        if not os.path.exists(parent_dir):
            try:
                os.makedirs(parent_dir)
            except Exception as e:
                logging.error('error happened while creating directory: %s', e)
                return False

        try:
            shutil.copy(self._path_output, report_path)
        except Exception as e:  # TODO(yuexima): more specific error catch
            logging.error('failed to copy output to report path: %s', e)
            return False

        return True

    def ClearLastOutput(self):
        '''Clear systrace output html.

        Since output are created in temp directories, this step is optional.

        Returns:
            True if successfully deleted temp output file; False otherwise.
        '''

        if self._path_output:
            try:
                shutil.rmtree(os.path.basename(self._path_output))
            except Exception as e:
                logging.error('failed to remove systrace output file. %s', e)
                return False

        return True
