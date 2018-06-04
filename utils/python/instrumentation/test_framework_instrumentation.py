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

from vts.utils.python.instrumentation import test_framework_instrumentation_categories as tfic
from vts.utils.python.instrumentation import test_framework_instrumentation_event as tfie


# global category listing
categories = tfic.TestFrameworkInstrumentationCategories()


def Begin(category, name=None):
    """Marks the beginning of an event.

    Params:
        category: string, category of the event
        name: string, name of the event. If None or empty, the value category will be copied.

    Returns:
        Event object representing the event
    """
    if not name:
        name = category

    event = tfie.TestFrameworkInstrumentationEvent(category, name)
    event.Begin()
    return event


def End(category, name=None):
    """Marks the end of an event.

    When category string is provided, it will be matched to an event in
    internal event stack.

    Params:
        category: string, category of the event
        name: string, name of the event. If None or empty, the value category will be copied.

    Returns:
        Event object representing the event. None if cannot find an active matching event
    """
    if not name:
        name = category

    event = FindEvent(category, name)
    if not event:
        logging.error('Event with category %s and name %s either does not '
                      'exists or has already ended. Skipping...', category, name)
        return None

    event.End()
    return event


def FindEvent(category, name=None):
    """Finds an existing event that has started given the names.

    Params:
        category: string, category of the event
        name: string, name of the event. If None or empty, the value category will be copied.

    Returns:
        TestFrameworkInstrumentationEvent object if found; None otherwise.
    """
    if not name:
        name = category

    for event in reversed(tfie.event_stack):
        if event.Match(category, name):
            return event

    return None
