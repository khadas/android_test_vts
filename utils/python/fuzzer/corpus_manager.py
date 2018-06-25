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

from vts.runners.host import keys
from vts.utils.python.gcs import gcs_api_utils
from vts.utils.python.web import feature_utils


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

    #TODO(b/64022625): fetch from the highest priority
    def FetchCorpusSeed(self, test_name, local_temp_dir):
        """Fetches 1 seed corpus from the corpus seed directory of the corresponding
           test from the GCS directory.

        In GCS, moves the seed from corpus_seed directory to corpus_inuse directory.
        From GCS to host, downloads 1 corpus seed from corpus_inuse directory
        to {temp_dir}_{test_name}_corpus_seed in host machine.

        Args:
            test_name: name of the current fuzzing test.
            local_temp_dir: path to temporary directory for this test
                            on the host machine.

        Returns:
            inuse_seed, GCS file path of the seed in use for test case,
                        if fetch was successful.
            None, otherwise.
        """
        if self.enabled:
            logging.debug('Attempting to fetch corpus seed for %s.', test_name)
        else:
            return None

        corpus_seed_dir = self._GetDirPaths('corpus_seed', test_name)
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
            src_dir: source directory in local.
            dest_dir: destination directory in GCS.

        Returns:
            True, if successfully uploaded.
            False, otherwise.
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
            self._ClassifyPriority(test_name, local_temp_dir)
            return True
        else:
            logging.error('Failed to upload corpus output for %s.', test_name)
            return False

    def InuseToSeed(self, test_name, inuse_seed):
        """Moves the a corpus from corpus_inuse to corpus_seed.

        {test_name}_corpus_seed directory is the directory for corpus that are ready
        to be used as input corpus seed.

        Args:
            test_name: name of the current test.
            inuse_seed: path to corpus seed currently in use.

        Returns:
            True, if move was successful.
            False, if the inuse_seed file does not exist or move failed.
        """
        if not self.enabled:
            return False

        if self._gcs_api_utils.FileExists(inuse_seed):
            corpus_seed = self._GetFilePaths('corpus_seed', test_name,
                                             inuse_seed)
            return self._gcs_api_utils.MoveFile(inuse_seed, corpus_seed, True)
        else:
            logging.error('seed in use %s does not exist', inuse_seed)
            return False

    def InuseToComplete(self, test_name, inuse_seed):
        """Moves the a corpus from corpus_inuse to corpus_complete.

        {test_name}_corpus_complete directory is the directory for corpus that have
        been used as an input and the test exited normally.

        Args:
            test_name: name of the current test.
            inuse_seed: path to corpus seed currently in use.

        Returns:
            True, if move was successful.
            False, if the inuse_seed file does not exist or move failed.
        """
        if not self.enabled:
            return False

        if self._gcs_api_utils.FileExists(inuse_seed):
            corpus_complete = self._GetFilePaths('corpus_complete', test_name,
                                                 inuse_seed)
            return self._gcs_api_utils.MoveFile(inuse_seed, corpus_complete,
                                                True)
        else:
            logging.error('seed in use %s does not exist.', inuse_seed)
            return False

    def InuseToCrash(self, test_name, inuse_seed):
        """Moves the a corpus from corpus_inuse to corpus_crash.

        {test_name}_corpus_crash directory is the directory for corpus that have
        caused a fuzz test crash.

        Args:
            test_name: name of the current test.
            inuse_seed: path to corpus seed currently in use.

        Returns:
            True, if move was successful.
            False, if the inuse_seed file does not exist or move failed.
        """
        if not self.enabled:
            return False

        if self._gcs_api_utils.FileExists(inuse_seed):
            corpus_crash = self._GetFilePaths('corpus_crash', test_name,
                                              inuse_seed)
            return self._gcs_api_utils.MoveFile(inuse_seed, corpus_crash, True)
        else:
            logging.error('seed in use %s does not exist.', inuse_seed)
            return False

    #TODO(b/64022625): smart algorithm for classifying corpus into different levels of priority
    def _ClassifyPriority(self, test_name, local_temp_dir):
        """Classifies each of newly genereated corpus into different priorities.

        Args:
            test_name: name of the current test.
            local_temp_dir: path to temporary directory for this test
                            on the host machine.
        """
        incoming_child_dir = self._GetDirPaths('incoming_child', test_name,
                                               local_temp_dir)
        for incoming_seed in self._gcs_api_utils.ListFilesWithPrefix(
                incoming_child_dir):
            corpus_seed = self._GetFilePaths('corpus_seed', test_name,
                                             incoming_seed)
            self._gcs_api_utils.MoveFile(incoming_seed, corpus_seed, True)

    def _GetDirPaths(self, dir_type, test_name, local_temp_dir=None):
        """Generates the required directory path name for the given information.

        Args:
            dir_type: type of the directory requested.
            test_name: name of the current test.
            local_temp_dir: path to temporary directory for this test
                            on the host machine.

        Returns:
            dir_path, generated directory path, if dir_type supported.
            Empty string, if dir_type not supported.
        """
        dir_path = ''

        # ex: corpus/ILight/ILight_corpus_seed
        if dir_type == 'corpus_seed':
            dir_path = 'corpus/%s/%s_corpus_seed' % (test_name, test_name)
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

        return dir_path

    def _GetFilePaths(self, file_type, test_name, seed=None):
        """Generates the required file path name for the given information.

        Args:
            file_type: type of the file requested.
            test_name: name of the current test.
            seed: seed to base new file path name upon.

        Returns:
            file_path, generated file path, if file_type supported.
            Empty string, if file_type not supported.
        """
        file_path = ''

        # ex: corpus/ILight/ILight_corpus_seed/20f5d9b8cd53881c9ff0205c9fdc5d283dc9fc68
        if file_type == 'corpus_seed':
            file_path = 'corpus/%s/%s_corpus_seed/%s' % (
                test_name, test_name, os.path.basename(seed))

        # ex: corpus/ILight/ILight_corpus_inuse/20f5d9b8cd53881c9ff0205c9fdc5d283dc9fc68
        elif file_type == 'corpus_inuse':
            file_path = 'corpus/%s/%s_corpus_inuse/%s' % (
                test_name, test_name, os.path.basename(seed))

        # ex: corpus/ILight/ILight_corpus_complete/20f5d9b8cd53881c9ff0205c9fdc5d283dc9fc68
        elif file_type == 'corpus_complete':
            file_path = 'corpus/%s/%s_corpus_complete/%s' % (
                test_name, test_name, os.path.basename(seed))

        # ex: corpus/ILight/ILight_corpus_crash/20f5d9b8cd53881c9ff0205c9fdc5d283dc9fc68
        elif file_type == 'corpus_crash':
            file_path = 'corpus/%s/%s_corpus_crash/%s' % (
                test_name, test_name, os.path.basename(seed))

        return file_path
