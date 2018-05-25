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
ILLEGAL_CHARS = ':\t\r\n '


class TestFrameworkInstrumentationEvent(object):
    """An object that represents an event.

    Attributes:
        category: string, a category mark represents a high level event
                  category such as preparer setup and test execution.
        name: string, a string to mark specific name of an event for human
              reading. Final performance analysis will mostly focus on category
              granularity instead of name granularity.
        status: int, 0 for not started, 1 for started, 2 for ended.
        error: string, None if no error. Otherwise contains error messages such
               as duplicated Begin or End.
    """
    category = None
    name = None
    status = 0
    error = None
    # TODO(yuexima): add on/off toggle param for logging.

    def __init__(self, category, name):
        if set(ILLEGAL_CHARS) & set(category + name):
            logging.warn('TestFrameworkInstrumentation: illegal character detected in '
                          'category or name string. Provided category: %s, name: %s. '
                          'Replacing them as "_"', category, name)
            category = re.sub('|'.join(ILLEGAL_CHARS), '_', category)
            name = re.sub('|'.join(ILLEGAL_CHARS), '_', name)

        self.category = category
        self.name = name

    def Match(self, category, name=''):
        """Checks whether the given category and name matches this event."""
        return category == self.category and name == self.name

    def Begin(self):
        """Performs logging action for the beginning of this event."""
        if self.status == 1:
            logging.error('TestFrameworkInstrumentation: event %s has already began. '
                          'Skipping Begin.', self)
            self.error = 'Tried to Begin but already began'
            return
        elif self.status == 2:
            logging.error('TestFrameworkInstrumentation: event %s has already ended. '
                          'Skipping Begin.', self)
            self.error = 'Tried to Begin but already ended'
            return

        logging.debug(LOGGING_TEMPLATE.format(category=self.category,
                                              name=self.name,
                                              status='BEGIN'))

        self.status = 1

    def End(self):
        """Performs logging action for the end of this event."""
        if self.status > 1:
            logging.error('TestFrameworkInstrumentation: event %s has already ended. '
                          'Skipping End.', self)
            self.error = 'Tried to End but already ended'
            return
        elif self.status < 1:
            logging.error('TestFrameworkInstrumentation: event %s has not yet began. '
                          'Skipping End.', self)
            self.error = 'Tried to End but have not began'
            return

        logging.debug(LOGGING_TEMPLATE.format(category=self.category,
                                              name=self.name,
                                              status='END'))
        self.status = 2

    def __str__(self):
        return 'Event object: @%s #%s' % (self.category, self.name)

