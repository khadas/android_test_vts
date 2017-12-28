#
# Copyright (C) 2017 The Android Open Source Project
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

import os
import shutil


def FindFile(directory, filename):
    '''Find a file under directory given the filename.

    Args:
        directory: string, directory path
        filename: string, file name to find

    Returns:
        String, path to the file found. None if not found.
    '''
    for (dirpath, dirnames, filenames) in os.walk(
            directory, followlinks=False):
        for fn in filenames:
            if fn == filename:
                return os.path.join(dirpath, filename)

    return None


def Rmdirs(path, ignore_errors=False):
    '''Remove the given directory and its contents recursively.

    Args:
        path: string, directory to delete
        ignore_errors: bool, whether to ignore errors. Defaults to False
    '''
    #TODO(yuexima): capture exceptions and make this function returning a bool
    shutil.rmtree(path, ignore_errors=ignore_errors)


def Makedirs(path, skip_if_exists=True):
    '''Make directories lead to the given path.

    Args:
        path: string, directory to make
        skip_if_exists: bool, True for ignoring exisitng dir. False for throwing
                        error from os.mkdirs. Defaults to True
    '''
    #TODO(yuexima): capture exceptions and make this function returning a bool
    if skip_if_exists and os.path.exists(path):
        return

    os.makedirs(path)
