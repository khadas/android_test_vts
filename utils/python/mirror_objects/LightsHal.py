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

from vts.utils.python.mirror_objects import MirrorBase


class LightsHal(MirrorBase.MirrorBase):
  """Lights HAL Mirror Object."""

  _target_class = "hal"
  _target_type = "light"
  _target_version = 1.0
  _target_basepath = "/system/lib/hw"
