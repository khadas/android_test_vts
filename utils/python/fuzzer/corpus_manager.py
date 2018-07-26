#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os
import random
import time
import uuid

from vts.runners.host import keys
from vts.utils.python.gcs import gcs_api_utils
from vts.utils.python.web import feature_utils
"""
Stage 1: FETCH_ONE_AND_FEED
The stage 1 algorithm collects the corpus output generated from the fuzz test.
Then, the algorithm chooses by random one of the generated seeds in the
next round as input.

Stage 2: FETCH_CRASH_AND_FEED
The stage 2 algorithm classifies generated corpus output into two priorities:
high priority and regular priority. Corpus strings created during a fuzz test
run that revealed a crash will be given a high priority.
On the other hand, corpus strings created during a fuzz test run that did
not lead to a crash will be given the regular priority.

Stage 3: FETCH_ALL_AND_REPEAT
TBA.
"""
FETCH_ONE_AND_FEED = 1
FETCH_CRASH_AND_FEED = 2
FETCH_ALL_AND_REPEAT = 3

SCHEDULING_ALGORITHM = FETCH_ONE_AND_FEED
MEASURE_CORPUS = True
CORPUS_STATES = [
    'corpus_seed_high', 'corpus_seed', 'corpus_seed_low', 'corpus_inuse',
    'corpus_complete', 'corpus_crash', 'corpus_error', 'corpus_trigger'
]
CORPUS_PRIORITIES = ['corpus_seed_high', 'corpus_seed', 'corpus_seed_low']


class CorpusManager(feature_utils.Feature):
    """Manages corpus for fuzzing.

    Features include:
    Fetching corpus input from GCS to host.
    Uploading corpus output from host to GCS.
    Classifying corpus output into different priorities.
    Moving corpus between different states (seed, inuse, complete).

    Attributes:
        _TOGGLE_PARAM: String, the name of the parameter used to toggle the feature.
        _REQUIRED_PARAMS: list, the list of parameter names that are required.
        _OPTIONAL_PARAMS: list, the list of parameter names that are optional.
    """

    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_LOG_UPLOADING
    _REQUIRED_PARAMS = [
        keys.ConfigKeys.IKEY_SERVICE_JSON_PATH,
        keys.ConfigKeys.IKEY_FUZZING_GCS_BUCKET_NAME
    ]
    _OPTIONAL_PARAMS = []

    def __init__(self, user_params):
        """Initializes the gcs util provider.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
        """
        self.ParseParameters(
            toggle_param_name=self._TOGGLE_PARAM,
            required_param_names=self._REQUIRED_PARAMS,
            optional_param_names=self._OPTIONAL_PARAMS,
            user_params=user_params)

        if self.enabled:
            self._key_path = self.service_key_json_path
            self._bucket_name = self.fuzzing_gcs_bucket_name
            self._gcs_api_utils = gcs_api_utils.GcsApiUtils(
                self._key_path, self._bucket_name)

    def FetchCorpusSeed(self, test_name, local_temp_dir):
        """Fetches seed corpus of the corresponding test from the GCS directory.

        Args:
            test_name: string, name of the current fuzzing test.
            local_temp_dir: string, path to temporary directory for this test
                            on the host machine.

        Returns:
            inuse_seed, GCS file path of the seed in use for test case
                        if fetch was successful.
            None otherwise.
        """
        if self.enabled:
            logging.debug('Attempting to fetch corpus seed for %s.', test_name)
        else:
            return None

        if SCHEDULING_ALGORITHM == FETCH_ONE_AND_FEED:
            inuse_seed = self._FetchCorpusSeedFromPriority(
                test_name, local_temp_dir, 'corpus_seed')
            return inuse_seed
        elif SCHEDULING_ALGORITHM == FETCH_CRASH_AND_FEED:
            for CORPUS_PRIORITY in CORPUS_PRIORITIES:
                inuse_seed = self._FetchCorpusSeedFromPriority(
                    test_name, local_temp_dir, CORPUS_PRIORITY)
                if inuse_seed is not None:
                    return inuse_seed
            return None

    def _FetchCorpusSeedFromPriority(self, test_name, local_temp_dir,
                                     CORPUS_PRIORITY):
        """Fetches 1 seed corpus from a corpus seed directory with the given priority.

        In GCS, moves the seed from corpus_seed directory to corpus_inuse directory.
        From GCS to host, downloads 1 corpus seed from corpus_inuse directory
        to {temp_dir}_{test_name}_corpus_seed in host machine.

        Args:
            test_name: string, name of the current fuzzing test.
            local_temp_dir: string, path to temporary directory for this test
                            on the host machine.
            CORPUS_PRIORITY: string, priority of the given directory.

        Returns:
            inuse_seed, GCS file path of the seed in use for test case
                        if fetch was successful.
            None otherwise.
        """
        corpus_seed_dir = self._GetDirPaths(CORPUS_PRIORITY, test_name)
        num_try = 0
        while num_try < 10:
            seed_list = self._gcs_api_utils.ListFilesWithPrefix(
                corpus_seed_dir)

            if len(seed_list) == 0:
                logging.info('No corpus available to fetch from %s.',
                             corpus_seed_dir)
                return None

            target_seed = seed_list[random.randint(0, len(seed_list) - 1)]
            inuse_seed = self._GetFilePaths('corpus_inuse', test_name,
                                            target_seed)
            move_successful = self._gcs_api_utils.MoveFile(
                target_seed, inuse_seed, False)

            if move_successful:
                local_dest_folder = self._gcs_api_utils.PrepareDownloadDestination(
                    corpus_seed_dir, local_temp_dir)
                dest_file_path = os.path.join(local_dest_folder,
                                              os.path.basename(target_seed))
                try:
                    self._gcs_api_utils.DownloadFile(inuse_seed,
                                                     dest_file_path)
                    logging.info('Successfully fetched corpus seed from %s.',
                                 corpus_seed_dir)
                except:
                    logging.error('Download failed, retrying.')
                    continue
                return inuse_seed
            else:
                num_try += 1
                logging.debug('move try %d failed, retrying.', num_try)
                continue

    def UploadCorpusOutDir(self, test_name, local_temp_dir):
        """Uploads the corpus output source directory in host to GCS.

        First, uploads the corpus output sorce directory in host to
        its corresponding incoming directory in GCS.
        Then, calls _ClassifyPriority function to classify each of
        newly generated corpus by its priority.
        Empty directory can be handled in the case no interesting corpus
        was generated.

        Args:
            test_name: string, name of the current fuzzing test.
            local_temp_dir: string, path to temporary directory for this test
                            on the host machine.

        Returns:
            True if successfully uploaded.
            False otherwise.
        """
        if self.enabled:
            logging.debug('Attempting to upload corpus output for %s.',
                          test_name)
        else:
            return False

        local_corpus_out_dir = self._GetDirPaths('local_corpus_out', test_name,
                                                 local_temp_dir)
        incoming_parent_dir = self._GetDirPaths('incoming_parent', test_name,
                                                local_temp_dir)
        if self._gcs_api_utils.UploadDir(local_corpus_out_dir,
                                         incoming_parent_dir):
            logging.info('Successfully uploaded corpus output to %s.',
                         incoming_parent_dir)
            num_unique_corpus = self._ClassifyPriority(test_name,
                                                       local_temp_dir)
            if MEASURE_CORPUS:
                self._UploadCorpusMeasure(test_name, local_temp_dir,
                                          num_unique_corpus)
            return True
        else:
            logging.error('Failed to upload corpus output for %s.', test_name)
            return False

    def _UploadCorpusMeasure(self, test_name, local_temp_dir,
                             num_unique_corpus):
        """Uploads the corpus measurement file to GCS.

        Args:
            test_name: string, name of the current fuzzing test.
            local_temp_dir: string, path to temporary directory for this test
                            on the host machine.
            num_unique_corpus: integer, number of unique corpus generated.
        """
        local_measure_file = os.path.join(
            local_temp_dir,
            '%s_%s.txt' % (test_name, time.strftime('%Y-%m-%d-%H%M')))
        with open(local_measure_file, 'w') as f:
            f.write(str(num_unique_corpus))
        remote_measure_file = os.path.join(
            self._GetDirPaths('corpus_measure', test_name),
            os.path.basename(local_measure_file))
        self._gcs_api_utils.UploadFile(local_measure_file, remote_measure_file)

    def InuseToDest(self, test_name, inuse_seed, destination):
        """Moves the a corpus from corpus_inuse to destination.

        Destinations are as follows:
        corpus_seed directory is the directory for corpus that are ready
        to be used as input corpus seed.
        corpus_complete directory is the directory for corpus that have
        been used as an input, succeeded, and the test exited normally.
        corpus_crash directory is the directory for corpus whose mutant have
        caused a fuzz test crash.
        corpus_error directory is the directory for corpus that have
        caused an error in executing the fuzz test.

        Args:
            test_name: string, name of the current test.
            inuse_seed: string, path to corpus seed currently in use.
            destination: string, destination of the seed.

        Returns:
            True if move was successful.
            False otherwise.
        """
        if not self.enabled:
            return False

        if self._gcs_api_utils.FileExists(inuse_seed):
            if destination in CORPUS_STATES:
                corpus_destination = self._GetFilePaths(
                    destination, test_name, inuse_seed)
                return self._gcs_api_utils.MoveFile(inuse_seed,
                                                    corpus_destination, True)
            else:
                logging.error(
                    'destination is not one of the predefined states')
                return False
        else:
            logging.error('seed in use %s does not exist', inuse_seed)
            return False

    def _CorpusIsDuplicate(self, test_name, incoming_seed):
        """Checks if the newly generated corpus is a duplicate corpus.

        Args:
            test_name: string, name of the current test.
            incoming_seed: string, path to the incoming seed in GCS.

        Returns:
            True if the incoming corpus already exists in the GCS bucket.
            False otherwise.
        """
        for file_type in CORPUS_STATES:
            remote_corpus = self._GetFilePaths(file_type, test_name,
                                               incoming_seed)
            logging.debug(remote_corpus)
            if self._gcs_api_utils.FileExists(remote_corpus):
                logging.info('Corpus %s already exists.', remote_corpus)
                return True
        return False

    def _ClassifyPriority(self, test_name, local_temp_dir):
        """Calls the appropriate classification algorithm.

        Args:
            test_name: string, name of the current test.
            local_temp_dir: string, path to temporary directory for this
                            test on the host machine.

        Returns:
            num_unique_corpus: integer, number of unique corpus generated.
        """
        if SCHEDULING_ALGORITHM == FETCH_ONE_AND_FEED:
            return self._ClassifyPriority1(test_name, local_temp_dir)
        elif SCHEDULING_ALGORITHM == FETCH_CRASH_AND_FEED:
            return self._ClassifyPriority2(test_name, local_temp_dir)

    def _ClassifyPriority1(self, test_name, local_temp_dir):
        """Classifies each of newly genereated corpus into different priorities.

        Uses 1 priority level: corpus_seed.
        This algorithm is a naive implementation.

        Args:
            test_name: string, name of the current test.
            local_temp_dir: string, path to temporary directory for this
                            test on the host machine.

        Returns:
            num_unique_corpus: integer, number of unique corpus generated.
        """
        incoming_child_dir = self._GetDirPaths('incoming_child', test_name,
                                               local_temp_dir)
        num_unique_corpus = 0
        for incoming_seed in self._gcs_api_utils.ListFilesWithPrefix(
                incoming_child_dir):
            if self._CorpusIsDuplicate(test_name, incoming_seed):
                logging.info('Deleting duplicate corpus.')
                self._gcs_api_utils.DeleteFile(incoming_seed)
                continue

            num_unique_corpus += 1
            logging.info(
                'Corpus string %s was classified as regular priority.',
                incoming_seed)
            corpus_destination = self._GetFilePaths('corpus_seed', test_name,
                                                    incoming_seed)
            self._gcs_api_utils.MoveFile(incoming_seed, corpus_destination,
                                         True)

        return num_unique_corpus

    def _ClassifyPriority2(self, test_name, local_temp_dir):
        """Classifies each of newly genereated corpus into different priorities.

        Uses 2 priority levels: corpus_seed_high, corpus_seed.
        This algorithm uses crash occurrence as its classification criteria.

        Args:
            test_name: string, name of the current test.
            local_temp_dir: string, path to temporary directory for this
                            test on the host machine.

        Returns:
            num_unique_corpus: integer, number of unique corpus generated.
        """
        triggered_corpus = os.path.join(
            self._GetDirPaths('local_corpus_trigger', test_name,
                              local_temp_dir), 'crash_report')
        high_priority = os.path.exists(triggered_corpus)
        incoming_child_dir = self._GetDirPaths('incoming_child', test_name,
                                               local_temp_dir)
        num_unique_corpus = 0
        for incoming_seed in self._gcs_api_utils.ListFilesWithPrefix(
                incoming_child_dir):
            if self._CorpusIsDuplicate(test_name, incoming_seed):
                logging.info('Deleting duplicate corpus.')
                self._gcs_api_utils.DeleteFile(incoming_seed)
                continue

            num_unique_corpus += 1
            if high_priority:
                logging.info(
                    'corpus string %s was classified as high priority.',
                    incoming_seed)
                corpus_destination = self._GetFilePaths(
                    'corpus_seed_high', test_name, incoming_seed)
            else:
                logging.info(
                    'corpus string %s was classified as regular priority.',
                    incoming_seed)
                corpus_destination = self._GetFilePaths(
                    'corpus_seed', test_name, incoming_seed)
            self._gcs_api_utils.MoveFile(incoming_seed, corpus_destination,
                                         True)

        if os.path.exists(triggered_corpus):
            corpus_destination = self._GetFilePaths(
                'corpus_trigger', test_name, triggered_corpus)
            corpus_destination += str(uuid.uuid4())
            self._gcs_api_utils.UploadFile(triggered_corpus,
                                           corpus_destination)

        return num_unique_corpus

    def _GetDirPaths(self, dir_type, test_name, local_temp_dir=None):
        """Generates the required directory path name for the given information.

        Args:
            dir_type: string, type of the directory requested.
            test_name: string, name of the current test.
            local_temp_dir: string, path to temporary directory for this
                            test on the host machine.

        Returns:
            dir_path, generated directory path if dir_type supported.
            Empty string if dir_type not supported.
        """
        dir_path = ''

        # ex: corpus/ILight/ILight_corpus_seed_high
        if dir_type in CORPUS_PRIORITIES:
            dir_path = 'corpus/%s/%s_%s' % (test_name, test_name, dir_type)
        # ex: corpus/ILight/ILight_corpus_measure
        elif dir_type == 'corpus_measure':
            dir_path = 'corpus/%s/%s_%s' % (test_name, test_name, dir_type)
        # ex: corpus/ILight/incoming/tmpV1oPTp
        elif dir_type == 'incoming_parent':
            dir_path = 'corpus/%s/incoming/%s' % (
                test_name, os.path.basename(local_temp_dir))
        # ex: corpus/ILight/incoming/tmpV1oPTp/ILight_corpus_out
        elif dir_type == 'incoming_child':
            dir_path = 'corpus/%s/incoming/%s/%s_corpus_out' % (
                test_name, os.path.basename(local_temp_dir), test_name)
        # ex: /tmp/tmpV1oPTp/ILight_corpus_out
        elif dir_type == 'local_corpus_out':
            dir_path = os.path.join(local_temp_dir,
                                    '%s_corpus_out' % test_name)
        # ex: /tmp/tmpV1oPTp/ILight_corpus_trigger
        elif dir_type == 'local_corpus_trigger':
            dir_path = os.path.join(local_temp_dir,
                                    '%s_corpus_trigger' % test_name)

        return dir_path

    def _GetFilePaths(self, file_type, test_name, seed=None):
        """Generates the required file path name for the given information.

        Args:
            file_type: string, type of the file requested.
            test_name: string, name of the current test.
            seed: string, seed to base new file path name upon.

        Returns:
            file_path, generated file path if file_type supported.
            Empty string if file_type not supported.
        """
        # ex: corpus/ILight/ILight_corpus_seed/20f5d9b8cd53881c9ff0205c9fdc5d283dc9fc68
        if file_type in CORPUS_STATES:
            file_path = 'corpus/%s/%s_%s/%s' % (
                test_name, test_name, file_type, os.path.basename(seed))
            return file_path
        else:
            logging.error('invalid file_type argument entered.')
            return ''
