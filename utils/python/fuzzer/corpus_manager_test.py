#!/usr/bin/env python
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

import mock
import unittest

from vts.utils.python.fuzzer import corpus_manager


class CorpusManagerTest(unittest.TestCase):
    """Unit tests for corpus_manager module."""

    def SetUp(self):
        """Setup tasks."""
        self.category = "category_default"
        self.name = "name_default"

    def testInitializationDisabled(self):
        """Tests the disabled initilization of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        self.assertEqual(_corpus_manager.enabled, False)

    def testInitializationEnabled(self):
        """Tests the enabled initilization of a CorpusManager object.

        If we initially begin with enabled=True, it will automatically
        attempt to connect GCS API.
        """
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        self.assertEqual(_corpus_manager.enabled, True)

    def testFetchCorpusSeedEmpty(self):
        """Tests the FetchCorpusSeed function of a CorpusManager object with
        an empty seed directory."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager._gcs_api_utils.ListFilesWithPrefix.return_value = []
        _corpus_manager.FetchCorpusSeed('ILight', '/tmp/tmpDir1')
        _corpus_manager._gcs_api_utils.ListFilesWithPrefix.assert_called_with(
            'corpus/ILight/ILight_corpus_seed')
        _corpus_manager._gcs_api_utils.MoveFile.assert_not_called()

    def testFetchCorpusSeedValid(self):
        """Tests the FetchCorpusSeed function of a CorpusManager object with
        a valid seed directory."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager._gcs_api_utils.ListFilesWithPrefix.return_value = [
            'dir/file1', 'dir/file2', 'dir/file3', 'dir/file4', 'dir/file5'
        ]
        _corpus_manager._gcs_api_utils.MoveFile.return_value = True
        _corpus_manager.FetchCorpusSeed('ILight', '/tmp/tmpDir1')
        _corpus_manager._gcs_api_utils.ListFilesWithPrefix.assert_called_with(
            'corpus/ILight/ILight_corpus_seed')
        _corpus_manager._gcs_api_utils.MoveFile.assert_called()
        _corpus_manager._gcs_api_utils.PrepareDownloadDestination.assert_called(
        )
        _corpus_manager._gcs_api_utils.DownloadFile.assert_called()

    def testUploadCorpusOutDir(self):
        """Tests the UploadCorpusOutDir function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager.UploadCorpusOutDir('ILight', '/tmp/tmpDir1')
        _corpus_manager._gcs_api_utils.UploadDir.assert_called_with(
            '/tmp/tmpDir1/ILight_corpus_out', 'corpus/ILight/incoming/tmpDir1')
        _corpus_manager._gcs_api_utils.ListFilesWithPrefix.assert_called_with(
            'corpus/ILight/incoming/tmpDir1/ILight_corpus_out')

    def testInuseToDestSeed(self):
        """Tests the InuseToDest function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager.InuseToDest(
            'ILight', 'corpus/ILight/ILight_corpus_inuse/corpus_number_1',
            'corpus_seed')
        _corpus_manager._gcs_api_utils.MoveFile.assert_called_with(
            'corpus/ILight/ILight_corpus_inuse/corpus_number_1',
            'corpus/ILight/ILight_corpus_seed/corpus_number_1', True)

    def testInuseToDestComplete(self):
        """Tests the InuseToDest function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager.InuseToDest(
            'ILight', 'corpus/ILight/ILight_corpus_inuse/corpus_number_1',
            'corpus_complete')
        _corpus_manager._gcs_api_utils.MoveFile.assert_called_with(
            'corpus/ILight/ILight_corpus_inuse/corpus_number_1',
            'corpus/ILight/ILight_corpus_complete/corpus_number_1', True)

    def testInuseToDestCrash(self):
        """Tests the InuseToDest function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager.InuseToDest(
            'ILight', 'corpus/ILight/ILight_corpus_inuse/corpus_number_1',
            'corpus_crash')
        _corpus_manager._gcs_api_utils.MoveFile.assert_called_with(
            'corpus/ILight/ILight_corpus_inuse/corpus_number_1',
            'corpus/ILight/ILight_corpus_crash/corpus_number_1', True)

    def test_ClassifyPriority(self):
        """Tests the _ClassifyPriority function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        _corpus_manager.enabled = True
        _corpus_manager._gcs_api_utils = mock.MagicMock()
        _corpus_manager._ClassifyPriority('ILight', '/tmp/tmpDir1')
        _corpus_manager._gcs_api_utils.ListFilesWithPrefix.assert_called_with(
            'corpus/ILight/incoming/tmpDir1/ILight_corpus_out')

    def test_GetDirPaths(self):
        """Tests the _GetDirPaths function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        self.assertEqual(
            _corpus_manager._GetDirPaths('corpus_seed', 'ILight'),
            'corpus/ILight/ILight_corpus_seed')
        self.assertEqual(
            _corpus_manager._GetDirPaths('incoming_parent', 'ILight',
                                         '/tmp/tmpDir1'),
            'corpus/ILight/incoming/tmpDir1')
        self.assertEqual(
            _corpus_manager._GetDirPaths('incoming_child', 'ILight',
                                         '/tmp/tmpDir1'),
            'corpus/ILight/incoming/tmpDir1/ILight_corpus_out')
        self.assertEqual(
            _corpus_manager._GetDirPaths('corpus_seed', 'ILight'),
            'corpus/ILight/ILight_corpus_seed')

    def test_GetFilePaths(self):
        """Tests the _GetFilePaths function of a CorpusManager object."""
        _corpus_manager = corpus_manager.CorpusManager({})
        self.assertEqual(
            _corpus_manager._GetFilePaths('corpus_seed', 'ILight',
                                          'some_dir/corpus_number_1'),
            'corpus/ILight/ILight_corpus_seed/corpus_number_1')
        self.assertEqual(
            _corpus_manager._GetFilePaths('corpus_inuse', 'ILight',
                                          'some_dir/corpus_number_1'),
            'corpus/ILight/ILight_corpus_inuse/corpus_number_1')
        self.assertEqual(
            _corpus_manager._GetFilePaths('corpus_complete', 'ILight',
                                          'somedir/corpus_number_1'),
            'corpus/ILight/ILight_corpus_complete/corpus_number_1')
        self.assertEqual(
            _corpus_manager._GetFilePaths('corpus_crash', 'ILight',
                                          'somedir/corpus_number_1'),
            'corpus/ILight/ILight_corpus_crash/corpus_number_1')


if __name__ == "__main__":
    unittest.main()
