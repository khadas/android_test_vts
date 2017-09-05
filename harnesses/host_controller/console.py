#
# Copyright (C) 2017 The Android Open Source Project
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

import argparse
import cmd
import sys

from vts.harnesses.host_controller.tfc import request


class ConsoleArgumentError(Exception):
    """Raised when the console fails to parse commands."""
    pass


class ConsoleArgumentParser(argparse.ArgumentParser):
    """The argument parser for a console command."""

    def __init__(self, command_name, description):
        """Initializes the ArgumentParser without --help option.

        Args:
            command_name: A string, the first argument of the command.
            description: The help message about the command.
        """
        super(ConsoleArgumentParser, self).__init__(prog=command_name,
                                                    description=description,
                                                    add_help=False)

    def ParseLine(self, line):
        """Parses a command line.

        Args:
            line: A string, the command line.

        Returns:
            An argparse.Namespace object.
        """
        return self.parse_args(line.split())

    # @Override
    def error(self, message):
        """Raises an exception when failing to parse the command.

        Args:
            message: The error message.

        Raises:
            ConsoleArgumentError.
        """
        raise ConsoleArgumentError(message)


class Console(cmd.Cmd):
    """The console for host controllers.

    Attributes:
        _tfc_client: The TfcClient that the host controllers connect to.
        _hosts: A list of HostController objects.
        _in_file: The input file object.
        _out_file: The output file object.
        prompt: The prompt string at the beginning of each command line.
        _lease_parser: The parser for lease command.
        _list_parser: The parser for list command.
        _request_parser: The parser for request command.
    """

    def __init__(self, tfc, host_controllers,
                 in_file=sys.stdin, out_file=sys.stdout):
        """Initializes the attributes and the parsers."""
        # cmd.Cmd is old-style class.
        cmd.Cmd.__init__(self, stdin=in_file, stdout=out_file)
        self._tfc_client = tfc
        self._hosts = host_controllers
        self._in_file = in_file
        self._out_file = out_file
        self.prompt = "> "

        self._InitLeaseParser()
        self._InitListParser()
        self._InitRequestParser()

    def _InitRequestParser(self):
        """Initializes the parser for request command."""
        self._request_parser = ConsoleArgumentParser(
                "request", "Send TFC a request to execute a command.")
        self._request_parser.add_argument(
                "--cluster", required=True,
                help="The cluster to which the request is submitted.")
        self._request_parser.add_argument(
                "--run-target", required=True,
                help="The target device to run the command.")
        self._request_parser.add_argument(
                "--user", required=True,
                help="The name of the user submitting the request.")
        self._request_parser.add_argument(
                "command", metavar="COMMAND", nargs="+",
                help='The command to be executed. If the command contains '
                     'arguments starting with "-", place the command after '
                     '"--" at end of line.')

    def do_request(self, line):
        """Sends TFC a request to execute a command."""
        args = self._request_parser.ParseLine(line)
        req = request.Request(cluster=args.cluster,
                              command_line=" ".join(args.command),
                              run_target=args.run_target,
                              user=args.user)
        self._tfc_client.NewRequest(req)

    def help_request(self):
        self._request_parser.print_help(self._out_file)

    def _InitListParser(self):
        """Initializes the parser for list command."""
        self._list_parser = ConsoleArgumentParser(
                "list", "Show information about the hosts.")
        self._list_parser.add_argument("--host", type=int,
                                       help="The index of the host.")
        self._list_parser.add_argument("type",
                                       choices=("hosts", "devices"),
                                       help="The type of the shown objects.")

    def do_list(self, line):
        """Shows information about the hosts."""
        args = self._list_parser.ParseLine(line)
        if args.host is None:
            hosts = enumerate(self._hosts)
        else:
            hosts = [(args.host, self._hosts[args.host])]
        if args.type == "hosts":
            self._PrintHosts(self._hosts)
        elif args.type == "devices":
            for ind, host in hosts:
                devices = host.ListDevices()
                self._Print("[%3d]  %s" % (ind, host.hostname))
                self._PrintDevices(devices)

    def help_list(self):
        self._list_parser.print_help(self._out_file)

    def _PrintHosts(self, hosts):
        """Shows a list of host controllers.

        Args:
            hosts: A list of HostController objects.
        """
        self._Print("index  name")
        for ind, host in enumerate(hosts):
            self._Print("[%3d]  %s" % (ind, host.hostname))

    def _PrintDevices(self, devices):
        """Shows a list of devices.

        Args:
            devices: A list of DeviceInfo objects.
        """
        attr_names = ("device_serial", "state", "run_target", "build_id",
                      "sdk_version", "stub")
        self._PrintObjects(devices, attr_names)

    def _PrintObjects(self, objects, attr_names):
        """Shows objects as a table.

        Args:
            object: The objects to be shown, one object in a row.
            attr_names: The attributes to be shown, one attribute in a column.
        """
        width = [len(name) for name in attr_names]
        rows = [attr_names]
        for dev_info in objects:
            attrs = [_ToPrintString(getattr(dev_info, name, ""))
                     for name in attr_names]
            rows.append(attrs)
            for index, attr in enumerate(attrs):
                width[index] = max(width[index], len(attr))

        for row in rows:
            self._Print("  ".join(attr.ljust(width[index])
                                  for index, attr in enumerate(row)))

    def _InitLeaseParser(self):
        """Initializes the parser for lease command."""
        self._lease_parser = ConsoleArgumentParser(
                "lease", "Make a host lease command tasks from TFC.")
        self._lease_parser.add_argument("--host", type=int,
                                        help="The index of the host.")

    def do_lease(self, line):
        """Makes a host lease command tasks from TFC."""
        args = self._lease_parser.ParseLine(line)
        if args.host is None:
            if len(self._hosts) > 1:
                raise ConsoleArgumentError("More than one hosts.")
            args.host = 0
        tasks = self._hosts[args.host].LeaseCommandTasks()
        self._PrintTasks(tasks)

    def help_lease(self):
        self._lease_parser.print_help(self._out_file)

    def _PrintTasks(self, tasks):
        """Shows a list of command tasks.

        Args:
            devices: A list of DeviceInfo objects.
        """
        attr_names = ("request_id", "command_id", "task_id", "device_serials",
                      "command_line")
        self._PrintObjects(tasks, attr_names)

    def do_exit(self, line):
        """Terminates the console.

        Returns:
            True, which stops the cmdloop.
        """
        return True

    def help_exit(self):
        self._Print("Terminate the console.")

    def _Print(self, string):
        """Prints a string and a new line character.

        Args:
            string: The string to be printed.
        """
        self._out_file.write(string + "\n")

    # @Override
    def onecmd(self, line):
        """Executes a command and prints any exception."""
        try:
            return cmd.Cmd.onecmd(self, line)
        except Exception as e:
            self._Print("%s: %s" % (type(e).__name__, e))
            return None

    # @Override
    def emptyline(self):
        """Ignores empty lines."""
        pass

    # @Override
    def default(self, line):
        """Handles unrecognized commands.

        Returns:
            True if receives EOF; otherwise delegates to default handler.
        """
        if line == "EOF":
            return self.do_exit(line)
        return cmd.Cmd.default(self, line)


def _ToPrintString(obj):
    """Converts an object to printable string on console.

    Args:
        obj: The object to be printed.
    """
    if isinstance(obj, (list, tuple, set)):
        return ",".join(str(x) for x in obj)
    return str(obj)
