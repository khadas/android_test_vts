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

from vts.utils.python.instrumentation import test_framework_instrumentation_event as tfie


# global instance of TestFrameworkInstrumentation class
global_instance = None


def GetInstance():
    """Creates and gets the global instance of TestFrameworkInstrumentation class."""
    global global_instance
    if global_instance is None:
        global_instance = TestFrameworkInstrumentation()

    return global_instance


def Begin(category, name=''):
    """Marks the beginning of an event.

    Params:
        category: string, category of the event
        name: string, name of the event, default is empty string.

    Returns:
        Event object representing the event
    """
    return GetInstance().Begin(category, name)


def End(category_or_event, name=''):
    """Marks the end of an event.

    When category string is provided, it will be matched to an event in
    internal event stack.

    Params:
        category_or_event: string or Event, category of the event or event object
        name: string, name of the event, default is empty string.

    Returns:
        Event object representing the event
    """
    return GetInstance().End(category_or_event, name)


class TestFrameworkInstrumentation(object):
    """A util class profile framework for performance analysis.

    Attributes:
        event_stack: list of Event objects, events that has began and not ended.
    """
    event_stack = []

    def Begin(self, category, name=''):
        """Marks the beginning of an event.

        Params:
            category: string, category of the event
            name: string, name of the event, default is empty string.

        Returns:
            Event object representing the event
        """
        event = tfie.TestFrameworkInstrumentationEvent(category, name)
        self.event_stack.append(event)
        event.Begin()
        return event

    def End(self, category_or_event, name=''):
        """Marks the end of an event.

        When category string is provided, it will be matched to an event in
        internal event stack.

        Params:
            category_or_event: string or Event, category of the event or event object
            name: string, name of the event, default is empty string.

        Returns:
            Event object representing the event
        """
        if isinstance(category_or_event, tfie.TestFrameworkInstrumentationEvent):
            if name:
                logging.warn('TestFrameworkInstrumentation: Provided event with name. '
                             'Ignoring the name.')
            return self._EndEvent(category_or_event)

        for event in reversed(self.event_stack):
            if event.Match(category_or_event, name):
                return self.End(event)

        logging.error('TestFrameworkInstrumentation: Cannot find given category "%s" '
                      'with name "%s".', category_or_event, name)

        return None

    def _EndEvent(self, event):
        """Internal method to process an End Event.

        Params:
            event: Event object, the event
        """
        if event is None:
            logging.warn('TestFrameworkInstrumentation: None event received and ignored.')
            return

        event.End()

        if event in self.event_stack:
            self.event_stack.remove(event)

    def __del__(self):
        if self.event_stack:
            print('TestFrameworkInstrumentation: WARNING: event stack is not empty in the end: '
                  '%s' % map(str, self.event_stack))
