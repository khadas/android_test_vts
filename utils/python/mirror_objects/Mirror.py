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


class Mirror(MirrorBase.MirrorBase):
    """HAL Mirror Object."""

    def __init__(self, target_basepath=None):
        if target_basepath:
            self._target_basepath = target_basepath

    def InitHal(self, target_type, target_version, target_basepath=None):
        """Initializes a HAL.

    Args:
      target_type: string, the target type name (e.g., light, camera).
      target_version: float, the target component version (e.g., 1.0).
      target_basepath: string, the base path of where a target file is stored
          in.
    """
        super(Mirror, self).Init("hal", target_type, target_version,
                                 target_basepath)

    def InitLegacyHal(self, target_type, target_version, target_basepath=None):
        """Initializes a legacy HAL (e.g., wifi).

    Args:
      target_type: string, the target type name (e.g., light, camera).
      target_version: float, the target component version (e.g., 1.0).
      target_basepath: string, the base path of where a target file is stored
          in.
    """
        super(Mirror, self).Init("legacy_hal", target_type, target_version,
                                 target_basepath)
