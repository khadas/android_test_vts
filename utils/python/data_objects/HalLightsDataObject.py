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
import random

from runners.host.proto import InterfaceSpecificationMessage_pb2

LIGHT_ID_BACKLIGHT = "backlight"
LIGHT_ID_KEYBOARD = "keyboard"
LIGHT_ID_BUTTONS = "buttons"
LIGHT_ID_BATTERY = "battery"
LIGHT_ID_NOTIFICATIONS = "notifications"
LIGHT_ID_ATTENTION = "attention"
LIGHT_ID_BLUETOOTH = "bluetooth"
LIGHT_ID_WIFI = "wifi"

# TODO: consider using a swig.
LIGHT_FLASH_NONE = 0
LIGHT_FLASH_TIMED = 1
LIGHT_FLASH_HARDWARE = 2
BRIGHTNESS_MODE_USER = 0
BRIGHTNESS_MODE_SENSOR = 1


def mask_uint32_t():
  shifter = random.randint(0, 31)
  return 1 << shifter


def mask_int32_t():
  return mask_uint32_t()


def light_state_t(color, flashMode, flashOnMs, flashOffMs, brightnessMode):
    """Creates a light_state_t data structure.

    Args:
      color: uint32_t, ARGB
      flashMode: int32_t (see HAL spec)
      flashOffMs: int32_t (see HAL spec)
      brightnessMode: int32_t (see HAL spec)

    Returns:
      ArgumentSpecificationMessage
    """
    aggregate_msg = InterfaceSpecificationMessage_pb2.ArgumentSpecificationMessage()

    color_field = aggregate_msg.primitive_value.add()
    color_field.uint32_t = color

    flashMode_field = aggregate_msg.primitive_value.add()
    flashMode_field.int32_t = flashMode

    flashOnMS_field = aggregate_msg.primitive_value.add()
    flashOnMS_field.int32_t = flashOnMs

    flashOffMS_field = aggregate_msg.primitive_value.add()
    flashOffMS_field.int32_t = flashOffMs

    brightnessMode_field = aggregate_msg.primitive_value.add()
    brightnessMode_field.int32_t = brightnessMode

    logging.info("light_state_t %s", aggregate_msg)
    return aggregate_msg



def light_state_t_fuzz(aggregate_msg):
    """Creates a light_state_t data structure.

    Args:
      ArgumentSpecificationMessage

    Returns:
      ArgumentSpecificationMessage
    """

    variable_index = random.randint(0, 4)
    count = 0
    for data in aggregate_msg.primitive_value:
      if count == variable_index:
        if variable_index == 0:
          data.uint32_t ^= mask_uint32_t()
        else:
          mask = mask_int32_t()
          if mask == (1 << 31):
            data.int32_t *= -1
            data.int32_t += 1
          else:
            data.int32_t ^= mask 
        break
      count += 1

    logging.info("light_state_t fuzzed %s", aggregate_msg)

    return aggregate_msg
