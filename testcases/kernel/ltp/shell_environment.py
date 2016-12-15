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

import logging

from vts.runners.host import const
from twisted.protocols.dict import Definition


class CheckDefinition(object):
    """Environment check definition class

    Used to specify what function to call, whether to setup environment after
    check failure, and put additional arguments.

    Attributes:
        _function: method, the function to call
        to_setup: bool, whether or not to setup the environment if check returns
                  False (given the method will deal with setup case). The default
                  value is False
        to_cleanup: whether to clean up after setup. The default value is True
        args: any additional arguments to pass to the method
    """
    def __init__(self, function, to_setup=False, to_cleanup=True, *args):
        self._function = function
        self.to_setup = to_setup
        self.to_cleanup = to_cleanup
        self.args = args

    def Execute(self):
        """Execute the specified function"""
        return self._function(self)

    def __str__(self):
        return "Shell Environment Check Definition: function={}, to_setup={} args={}".format(
                self._function, self.to_setup, self.args)

class ShellEnvironment(object):
    """Wrapper class for all the environment setup classes.

    Provides a bridge between the caller and individual setup classes.

    Attributes:
        _shell: shell mirror object, shell to execute commands
        _cleanup_jobs: set of CheckSetupCleanup objects, a set used to store clean
                       up jobs if requested.
    """
    def __init__(self, shell):
        self._shell = shell
        # TODO use a queue?
        self._cleanup_jobs = set()

    def Cleanup(self):
        """Final cleanup jobs. Will run all the stored cleanup jobs"""
        # TODO check error
        for obj in self._cleanup_jobs:
            obj.Cleanup()

    def AddCleanupJob(self, obj):
        """Add a clean up job for final cleanup"""
        self._cleanup_jobs.add(obj)

    def ExecuteCommand(self, command):
        """Execute a shell command or a list of shell commands.

        Args:
            command: str, the command to execute; Or
                     list of str, a list of commands to execute

        return:
            True if all commands return exit codes 0; False otherwise
        """
        command_result = self._shell.Execute(command)
        return all([i is 0 for i in command_result[const.EXIT_CODE]])

    def LoopDeviceSupport(self, definition):
        """Environment definition function.
        Will create a corresponding setup class and execute

        Args:
            definition: CheckDefinition obj, a definition class which stores
                        related arguments for the setup class to execute

        Return:
            tuple(bool, string), a tuple of True and empty string if success,
                                 a tuple of False and error message if fail.
        """
        return LoopDeviceSupportClass(definition, self).Execute()

    def DirExists(self, definition):
        """Environment definition function.
        Will create a corresponding setup class and execute

        Args:
            definition: CheckDefinition obj, a definition class which stores
                        related arguments for the setup class to execute

        Return:
            tuple(bool, string), a tuple of True and empty string if success,
                                 a tuple of False and error message if fail.
        """
        return DirExistsClass(definition, self).Execute()

    def DirsAllExistAndPermission(self, definition):
        """Environment definition function.
        Will create a corresponding setup class and execute

        Args:
            definition: CheckDefinition obj, a definition class which stores
                        related arguments for the setup class to execute

        Return:
            tuple(bool, string), a tuple of True and empty string if success,
                                 a tuple of False and error message if fail.
        """
        return DirsAllExistAndPermissionClass(definition, self).Execute()


class CheckSetupCleanup(object):
    """An abstract class for defining environment setup job.

    Usually such a job contains check -> setup -> cleanup workflow

    Attributes:
        _NOTE: string, a message used in GetNote() to display when environment
               check or setup failed.
        context: ShellEnvironment object, the context that can be used to add
                 cleanup jobs and execute shell commands
        definition: CheckDefinition object, the definition of a test case
                    requirement
    """
    _NOTE = ""

    def __init__(self, definition, context):
        self.definition = definition
        self.context = context

    def Execute(self):
        """Execute the check, setup, and cleanup.
        Will execute setup and cleanup only if the boolean switches for them
        are True. It will NOT execute cleanup if check function passes.

        Return:
            tuple(bool, string), a tuple of True and empty string if success,
                                 a tuple of False and error message if fail.
        """
        valid_input, message = self.ValidateInput()
        if not valid_input:
            return (valid_input, "Environment check error: %s" % message)

        check_result = self.Check()
        if check_result or not self.definition.to_setup:
            return (check_result, self.GetNote())

        if self.definition.to_cleanup:
            self.context.AddCleanupJob(self)

        return (self.Setup(), self.GetNote())

    def GetNote(self):
        """Get a string note as error message. Can be override by sub-class"""
        return "{}\n{}".format(self.NOTE, self.definition)

    def ValidateInput(self):
        """Validate input parameters. Can be override by sub-class

        Return:
            tuple(bool, string), a tuple of True and empty string if pass,
                                 a tuple of False and error message if fail.
        """
        return (True, "")

    def Check(self):
        """Check function for the class.
        Used to check environment. Can be override by sub-class"""
        return False

    def Setup(self):
        """Check function for the class.
        Used to setup environment if check fail. Can be override by sub-class"""
        return False

    def Cleanup(self):
        """Check function for the class.
        Used to cleanup setup if check fail. Can be override by sub-class"""
        return False

    def ExecuteCommand(self, cmd):
        return self.context.ExecuteCommand(cmd)


class LoopDeviceSupportClass(CheckSetupCleanup):
    """Class for checking loopback device support."""
    NOTE = "Kernel does not have loop device support"

    def Check(self):
        return self.ExecuteCommand("losetup -f")

class DirExistsClass(CheckSetupCleanup):
    """Class for check, setup, and cleanup a existence of a directory."""
    NOTE = "Directory does not exist"

    def ValidateInput(self):
        return (len(self.definition.args) is 1,
                "unexpected arguments in definition: %s" % self.definition)

    def Check(self):
        logging.info("Checking existence of directory %s" % definition.args[0])
        return self.ExecuteCommand("ls %s" % self.definition.args[0])

    def Setup(self):
        return self.ExecuteCommand("mkdir -p %s" % self.definition.args[0])

    def Cleanup(self):
        return self.ExecuteCommand("rm -rf %s" % self.definition.args[0])


class DirsAllExistAndPermissionClass(CheckSetupCleanup):
    """Class for check, setup, and cleanup a existence of a list of directories and set permissions."""
    NOTE = "Directories do not all exist or have the specified permission"

    def ValidateInput(self):
        return (len(self.definition.args) is 2 and \
                    len(self.definition.args[0]) == len(self.definition.args[1]),
                "unexpected arguments in definition: %s" % self.definition)

    def Check(self):
        dirs = self.definition.args[0]
        permissions = self.definition.args[1]

        commands = []
        commands.extend(["ls %s" % dir for dir in dirs])
        commands.extend(["stat -c {}a {}".format('%', *pair)
                         for pair in zip(permissions, dirs)])

        return self.ExecuteCommand(commands)

    def Setup(self):
        dirs = self.definition.args[0]
        permissions = self.definition.args[1]

        commands = []
        commands.extend(["mkdir -p %s" % dir for dir in dirs])
        commands.extend(["chmod {} {}".format(*pair)
                         for pair in zip(permissions, dirs)])

        return self.ExecuteCommand(commands)

    def Cleanup(self):
        commands = ["rm -rf %s" % dir for dir in self.definition.args[0]]
        return self.ExecuteCommand(commands)