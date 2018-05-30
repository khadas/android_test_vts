#
# Copyright (C) 2018 The Android Open Source Project
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

import logging
import re


# Log prefix to be parsed by performance analysis tools
LOGGING_PREFIX = '[Instrumentation]'
# Log line template in the format of <prefix> <type> @<category> #<name>
# Do not use tab(\t) because sometime they can be printed to log as the string '\t'
LOGGING_TEMPLATE = LOGGING_PREFIX + ' {category}: {name} {status}'
# Characters not allowed in provided event category or name
# In event init method, these characters are joint by '|' as regex. Modifications to
# the replacing logic be required if escape character is needed.
ILLEGAL_CHARS = ':\t\r\n'

# A list of Event objects: events that has began and not ended.
event_stack = []


class TestFrameworkInstrumentationEvent(object):
    """An object that represents an event.

    Attributes:
        category: string, a category mark represents a high level event
                  category such as preparer setup and test execution.
        name: string, a string to mark specific name of an event for human
              reading. Final performance analysis will mostly focus on category
              granularity instead of name granularity.
        status: int, 0 for not started, 1 for started, 2 for ended, 3 for removed.
        error: string, None if no error. Otherwise contains error messages such
               as duplicated Begin or End.
        _enable_logging: bool or None. Whether to put the event in logging.
                         Should be set to False when timing small pieces of code that could take
                         very short time to run.
                         If is None, global configuration will be used.
    """
    category = None
    name = None
    status = 0
    error = None
    _enable_logging = False
    # TODO(yuexima): add on/off toggle param for logging.

    def __init__(self, category, name):
        if set(ILLEGAL_CHARS) & set(category + name):
            self.LogW('TestFrameworkInstrumentation: illegal character detected in '
                          'category or name string. Provided category: %s, name: %s. '
                          'Replacing them as "_"', category, name)
            category = re.sub('|'.join(ILLEGAL_CHARS), '_', category)
            name = re.sub('|'.join(ILLEGAL_CHARS), '_', name)

        self.category = category
        self.name = name

    def Match(self, category, name):
        """Checks whether the given category and name matches this event."""
        return category == self.category and name == self.name

    def Begin(self, enable_logging=None):
        """Performs logging action for the beginning of this event.

        Args:
            enable_logging: bool or None. Whether to put the event in logging.
                            Should be set to False when timing small pieces of code that could take
                            very short time to run.
                            If not specified or is None, global configuration will be used.
                            This value can only be set in Begin method to make logging consistent.
        """
        if enable_logging is not None:
            self._enable_logging = enable_logging

        if self.status == 1:
            self.LogE('TestFrameworkInstrumentation: event %s has already began. '
                      'Skipping Begin.', self)
            self.error = 'Tried to Begin but already began'
            return
        elif self.status == 2:
            self.LogE('TestFrameworkInstrumentation: event %s has already ended. '
                      'Skipping Begin.', self)
            self.error = 'Tried to Begin but already ended'
            return
        elif self.status == 3:
            self.LogE('TestFrameworkInstrumentation: event %s has already been removed. '
                      'Skipping Begin.', self)
            self.error = 'Tried to Begin but already been removed'
            return

        self.LogD(LOGGING_TEMPLATE.format(category=self.category,
                                          name=self.name,
                                          status='BEGIN'))

        self.status = 1
        global event_stack
        event_stack.append(self)

    def End(self):
        """Performs logging action for the end of this event."""
        if self.status == 0:
            self.LogE('TestFrameworkInstrumentation: event %s has not yet began. '
                      'Skipping End.', self)
            self.error = 'Tried to End but have not began'
            return
        elif self.status == 2:
            self.LogE('TestFrameworkInstrumentation: event %s has already ended. '
                      'Skipping End.', self)
            self.error = 'Tried to End but already ended'
            return
        elif self.status == 3:
            self.LogE('TestFrameworkInstrumentation: event %s has already been removed. '
                      'Skipping End.', self)
            self.error = 'Tried to End but already been removed'
            return

        self.LogD(LOGGING_TEMPLATE.format(category=self.category,
                                          name=self.name,
                                          status='END'))

        self.status = 2
        global event_stack
        event_stack.remove(self)

    def CheckEnded(self, remove_reason=''):
        """Checks whether this event has ended and remove it if not.

        This method is designed to be used in a try-catch exception block, where it is
        not obvious whether the End() method has ever been called. In such case, if
        End() has not been called, it usually means exception has happened and the event
        should either be removed or automatically ended. Here we choose remove because
        this method could be called in the finally block in a try-catch block, and there
        could be a lot of noise before reaching the finally block.

        This method does not support being called directly in test_framework_instrumentation
        module with category and name lookup, because there can be events with same names.

        Calling this method multiple times on the same Event object is ok.

        Args:
            remove_reason: string, reason to remove to be recorded when the event was
                           not properly ended. Default is empty string.
        """
        if self.status < 2:
            self.Remove(remove_reason)

    def Remove(self, remove_reason=''):
        """Removes this event from reports and record the reason.

        Calling this method multiple times on the same Event object is ok.

        Args:
            remove_reason: string, reason to remove to be recorded when the event was
                           not properly ended. Default is empty string.
        """
        if self.status == 3:
            return

        self.LogD(LOGGING_TEMPLATE.format(category=self.category,
                                          name=self.name,
                                          status='REMOVE') +
                  ' | Reason: %s' % remove_reason)

        self.error = remove_reason
        self.status = 3
        global event_stack
        event_stack.remove(self)

    def LogD(self, *args):
        """Wrapper function for logging.debug"""
        self._Log(logging.debug, *args)

    def LogI(self, *args):
        """Wrapper function for logging.info"""
        self._Log(logging.info, *args)

    def LogW(self, *args):
        """Wrapper function for logging.warn"""
        self._Log(logging.warn, *args)

    def LogE(self, *args):
        """Wrapper function for logging.error"""
        self._Log(logging.error, *args)

    def _Log(self, log_func, *args):
        if self._enable_logging != False:
            log_func(*args)

    def __str__(self):
        return 'Event object: @%s #%s' % (self.category, self.name)
