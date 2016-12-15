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
"""Class to fetch manifest XML from internal build server.
"""

import apiclient
import httplib2
import io
import logging
import re
import time
from apiclient.discovery import build
from oauth2client import client as oauth2_client
from oauth2client.service_account import ServiceAccountCredentials
from vts.utils.python.retry import retry

logger = logging.getLogger('manifest_fetcher')

class DriverError(Exception):
    """Base Android GCE driver exception."""

class AndroidBuildClient(object):
    """Client that manages Android Build.

    Attributes:
        service: object, initialized and authorized service object for the
                 androidbuildinternal API.
        API_NAME: string, name of internal API accessed by the client.
        API_VERSION: string, version of the internal API accessed by the client.
        SCOPE: string, URL for which to request access via oauth2.
        DEFAULT_RESOURCE_ID: string, default artifact name to request.
        DEFAULT_ATTEMPT_ID: string, default attempt to request for the artifact.
        DEFAULT_CHUNK_SIZE: int, number of bytes to download at a time.
        RETRY_COUNT: int, max number of retries.
        RETRY_BACKOFF_FACTOR: float, base of exponential determining sleep time.
                              total_time = (backoff_factor^(attempt - 1))*sleep
        RETRY_SLEEP_MULTIPLIER: float, multiplier for how long to sleep between
                                attempts.
        RETRY_HTTP_CODES: int array, HTTP codes for which a retry will be
                          attempted.
        RETRIABLE_AUTH_ERRORS: class tuple, list of error classes for which a
                               retry will be attempted.

    """

    API_NAME = "androidbuildinternal"
    API_VERSION = "v2beta1"
    SCOPE = "https://www.googleapis.com/auth/androidbuild.internal"

    # other variables.
    DEFAULT_RESOURCE_ID = "0"
    DEFAULT_ATTEMPT_ID = "latest"
    DEFAULT_CHUNK_SIZE = 20 * 1024 * 1024

    # Defaults for retry.
    RETRY_COUNT = 5
    RETRY_BACKOFF_FACTOR = 1.5
    RETRY_SLEEP_MULTIPLIER = 1
    RETRY_HTTP_CODES = [
        500,  # Internal Server Error
        502,  # Bad Gateway
        503,  # Service Unavailable
    ]

    RETRIABLE_AUTH_ERRORS = (oauth2_client.AccessTokenRefreshError,)

    def __init__(self, oauth2_service_json):
        """Initialize.

        Args:
          oauth2_service_json: Path to service account json file.
        """
        authToken = ServiceAccountCredentials.from_json_keyfile_name(
            oauth2_service_json, [self.SCOPE])
        http_auth = authToken.authorize(httplib2.Http())
        self.service = retry.RetryException(
            exc_retry=self.RETRIABLE_AUTH_ERRORS,
            max_retry=self.RETRY_COUNT,
            functor=build,
            sleep=self.RETRY_SLEEP_MULTIPLIER,
            backoff_factor=self.RETRY_BACKOFF_FACTOR,
            serviceName=self.API_NAME,
            version=self.API_VERSION,
            http=http_auth)

    def DownloadManifest(self, build_target, build_id, local_dest,
                       attempt_id=None):
        """Get Android build manifest XML file.

        Args:
          build_target: Target name, e.g. "gce_x86-userdebug"
          build_id: Build id, a string, e.g. "2263051", "P2804227"
          local_dest: A local path where the artifact should be stored.
                      e.g. "/tmp/manifest_2263051.xml"
          attempt_id: String, attempt id, will default to DEFAULT_ATTEMPT_ID.
        """
        attempt_id = attempt_id or self.DEFAULT_ATTEMPT_ID
        resource_id = "manifest_%s.xml" % buildId
        api = self.service.buildartifact().get_media(
            buildId=build_id,
            target=build_target, attemptId=attempt_id,
            resourceId=resource_id)
        logger.info("Downloading manifest: target: %s, build_id: %s, "
                    "resource_id: %s, dest: %s", build_target, build_id,
                    resource_id, local_dest)
        try:
            with io.FileIO(local_dest, mode="wb") as fh:
                downloader = apiclient.http.MediaIoBaseDownload(
                    fh, api, chunksize=self.DEFAULT_CHUNK_SIZE)
                done = False
                while not done:
                    _, done = downloader.next_chunk()
            logger.info("Downloaded artifact: %s", local_dest)
        except OSError as e:
            logger.error("Downloading artifact failed: %s", str(e))
            raise DriverError(str(e))
