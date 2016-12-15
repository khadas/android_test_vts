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


class ArcSummary(object):
    """Summarizes an arc from a .gcno file.

    Attributes:
        src_block_index: integer index of the source basic block.
        dstBlockIndex: integer index of the destination basic block.
        on_tree: True iff arc has flag GCOV_ARC_ON_TREE.
        fake: True iff arc has flag GCOV_ARC_FAKE.
        fallthrough: True iff arc has flag GCOV_ARC_FALLTHROUGH.
    """

    GCOV_ARC_ON_TREE = 1 << 0
    GCOV_ARC_FAKE = 1 << 1
    GCOV_ARC_FALLTHROUGH = 1 << 2

    def __init__(self, src_block_index, dst_block_index, flag):
        """Inits the arc summary with provided values.

        Stores the source and destination block indices and parses
        the arc flag.

        Args:
            src_block_index: integer index of the source basic block.
            dst_block_index: integer index of the destination basic block.
            flag: integer flag for the given arc.
        """

        self.src_block_index = src_block_index
        self.dst_block_index = dst_block_index
        self.on_tree = bool(flag & self.GCOV_ARC_ON_TREE)
        self.fake = bool(flag & self.GCOV_ARC_FAKE)
        self.fallthrough = bool(flag & self.GCOV_ARC_FALLTHROUGH)
