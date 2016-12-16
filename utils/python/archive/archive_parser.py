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
"""Parses the contents of a Unix archive file generated using the 'ar' command.

The constructor returns an Archive object, which contains dictionary from
file name to file content.


    Typical usage example:

    archive = Archive(content)
    archive.Parse()
"""

import io

class Archive(object):
    """Archive object parses and stores Unix archive contents.

    Stores the file names and contents as it parses the archive.

    Attributes:
        files: a dictionary from file name (string) to file content (binary)
    """

    GLOBAL_SIG = '!<arch>\n'  # Unix global signature
    FILE_ID_LENGTH = 16  # Number of bytes to store file identifier
    FILE_TIMESTAMP_LENGTH = 12  # Number of bytes to store file mod timestamp
    OWNER_ID_LENGTH = 6  # Number of bytes to store file owner ID
    GROUP_ID_LENGTH = 6  # Number of bytes to store file group ID
    FILE_MODE_LENGTH = 8  # Number of bytes to store file mode
    CONTENT_SIZE_LENGTH = 10  # Number of bytes to store content size
    END_TAG = '`\n'  # Header end tag

    def __init__(self, file_content):
        """Initialize and parse the archive contents.

        Args:
          file_content: Binary contents of the archive file.
        """

        self.files = {}
        self._content = file_content
        self._cursor = 0

    def ReadBytes(self, n):
        """Reads n bytes from the content stream.

        Args:
            n: The integer number of bytes to read.

        Returns:
            The n-bit string (binary) of data from the content stream.

        Raises:
            ValueError: invalid file format.
        """
        if self._cursor + n > len(self._content):
            raise ValueError('Invalid file. EOF reached unexpectedly.')

        content = self._content[self._cursor : self._cursor + n]
        self._cursor += n
        return content

    def Parse(self):
        """Verifies the archive header and arses the contents of the archive.

        Raises:
            ValueError: invalid file format.
        """
        # Check global header
        sig = self.ReadBytes(len(self.GLOBAL_SIG))
        if sig != self.GLOBAL_SIG:
            raise ValueError('File is not a valid Unix archive.')

        # Read files in archive
        while self._cursor < len(self._content):
            self.ReadFile()

    def ReadFile(self):
        """Reads a file from the archive content stream.

        Raises:
            ValueError: invalid file format.
        """
        name = self.ReadBytes(self.FILE_ID_LENGTH).strip().strip('/')
        self.ReadBytes(self.FILE_TIMESTAMP_LENGTH)
        self.ReadBytes(self.OWNER_ID_LENGTH)
        self.ReadBytes(self.GROUP_ID_LENGTH)
        self.ReadBytes(self.FILE_MODE_LENGTH)
        content_size = int(self.ReadBytes(self.CONTENT_SIZE_LENGTH).strip())

        if self.ReadBytes(len(self.END_TAG)) != self.END_TAG:
            raise ValueError('File is not a valid Unix archive. Missing end tag.')

        content = self.ReadBytes(content_size)
        self.files[name] = content
