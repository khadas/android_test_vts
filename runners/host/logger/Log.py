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
#

import logging
import sys

_FORMAT = "%(asctime)-15s: %(message)s"
_TAG = "VtsHostRunner"

vts_logger = None


def _GetLogger():
  """Returns the VTS logger."""
  global vts_logger
  if vts_logger:
    return vts_logger

  try:
    logging.basicConfig(format=_FORMAT, level=logging.INFO)
    vts_logger = logging.getLogger(_TAG)
  except AttributeError as e:
    print(e)
    print("Your python version is %s" % sys.version)
    raise
  return vts_logger


def info(format, *args):
  """Handles the given info log message.

  Args:
    format: the string, either format or the complete message to log.
    args: optional - a list of arguments used to replace the % specifiers
        in the given format string.
  """
  logger = _GetLogger()
  if args:
    format = format % args
  logger.info(format)


def error(format, *args):
  """Handles the given error log message.

  Args:
    format: the string, either format or the complete message to log.
    args: optional - a list of arguments used to replace the % specifiers
        in the given format string.
  """
  logger = _GetLogger()
  if args:
    format = format % args
  logger.error(format)
