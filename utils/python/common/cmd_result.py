# Copyright 2018 - The Android Open Source Project
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

import vts.utils.python.common.cmd_utils as cmd_utils


class CmdResult(object):
    """Shell command result object.

    Attributes:
        stdout: string, command stdout output.
                If multiple command results are included in the object,
                only the last one is returned.
        stdouts: list of string, a list of command stdout outputs.
        stderr: string, command stderr output.
                If multiple command results are included in the object,
                only the last one is returned.
        stderrs: list of string, a list of command stderr outputs
        returncode: int, command returncode output.
                    If multiple command results are included in the object,
                    only the last one is returned.
        returncodes: list of int, a list of command returncode outputs.
    """

    def __init__(self, stdout, stderr, returncode):
        self.stdouts = []
        self.stderrs = []
        self.returncodes = []
        self.AddResult(stdout, stderr, returncode)

    @property
    def stdout(self):
        """Returns command stdout output.

        If multiple command results are included in the object, only the last one is returned.
        """
        return self.stdouts[-1]

    @property
    def stderr(self):
        """Returns command stderr output.

        If multiple command results are included in the object, only the last one is returned.
        """
        return self.stderrs[-1]

    @property
    def returncode(self):
        """Returns command returncode output.

        If multiple command results are included in the object, only the last one is returned.
        """
        return self.returncodes[-1]

    def AddResult(self, stdout, stderr, returncode):
        """Adds additional command result data to the object.

        Args:
            stdout: string, command stdout output
            stderr: string, command stderr output
            returncode: int, command returncode output
        """
        self.stdouts.append(stdout)
        self.stderrs.append(stderr)
        self.returncodes.append(returncode)

    def __getitem__(self, key):
        """Legacy code support for getting results as a dictionary.

        Args:
            key: string, commend result type.

        Returns:
            list of string or int, command results corresponding to provided key.

        Raises:
            KeyError if key is not specified in const.
        """
        if key == cmd_utils.STDOUT:
            return self.stdouts
        elif key == cmd_utils.STDERR:
            return self.stderrs
        elif key == cmd_utils.EXIT_CODE:
            return self.returncodes
        else:
            raise KeyError(key)