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

import types


class ShellEnvironment(object):
    """Class for executing environment definition classes and do cleanup jobs.

    Attributes:
        shell: shell mirror object, shell to execute commands
        _cleanup_jobs: set of CheckSetupCleanup objects, a set used to store
                       clean up jobs if requested.
    """

    def __init__(self, shell):
        self.shell = shell
        self._cleanup_jobs = []

    def Cleanup(self):
        """Final cleanup jobs. Will run all the stored cleanup jobs"""
        return all([method(*args) for method, args in self._cleanup_jobs])

    def AddCleanupJob(self, method, *args):
        """Add a clean up job for final cleanup"""
        if (method, args) not in self._cleanup_jobs:
            self._cleanup_jobs.append((method, args))

    @property
    def shell(self):
        """returns an object that can execute a shell command"""
        return self._shell

    @shell.setter
    def shell(self, shell):
        self._shell = shell

    def ExecuteDefinitions(self, definitions):
        """Execute a given list of environment check definitions"""
        if not isinstance(definitions, types.ListType):
            definitions = [definitions]

        for definition in definitions:
            definition.context = self
            if not definition.Execute():
                return (False, definition.GetNote())

        return (True, "")
