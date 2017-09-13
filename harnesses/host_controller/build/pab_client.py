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
"""Class to fetch artifacts from Partner Android Build server
"""

import argparse
import httplib2
import logging
import os
import requests
from posixpath import join as path_urljoin

from oauth2client.client import flow_from_clientsecrets
from oauth2client.file import Storage
from oauth2client.tools import argparser
from oauth2client.tools import run_flow

class PartnerAndroidBuildClient(object):
    """Client that manages Partner Android Build downloading.

    Attributes:
        CLIENT_STORAGE: string, path to store credentials.
        DEFAULT_CHUNK_SIZE: int, number of bytes to download at a time.
        GMS_DOWNLOAD_URL: string, base url for downloading artifacts.
        SCOPE: string, URL for which to request access via oauth2.
        credentials : oauth2client credentials object
    """
    credentials = None
    CLIENT_SECRETS = os.path.join(
        os.path.dirname(__file__), 'client_secrets.json')
    CLIENT_STORAGE = os.path.join(os.path.dirname(__file__), 'credentials')
    DEFAULT_CHUNK_SIZE = 1024
    GMS_DOWNLOAD_URL = 'https://partnerdash.google.com/build/gmsdownload'
    # need both of these scopes to access PAB downloader
    scopes = ('https://www.googleapis.com/auth/partnerdash',
              'https://www.googleapis.com/auth/alkali-base')
    SCOPE = ' '.join(scopes)

    def Authenticate(self):
        """Authenticate using OAuth2."""
        logging.info('Parsing flags, use --noauth_local_webserver' \
        'if running on remote machine')

        parser = argparse.ArgumentParser(parents=[argparser])
        flags = parser.parse_args()

        logging.info('Preparing OAuth token')
        flow = flow_from_clientsecrets(
            self.CLIENT_SECRETS,
            scope=self.SCOPE)
        storage = Storage(self.CLIENT_STORAGE)
        if self.credentials is None:
            self.credentials = storage.get()
        if self.credentials is None or self.credentials.invalid:
            logging.info('Credentials not found, authenticating.')
            self.credentials = run_flow(flow, storage, flags)

        if self.credentials.access_token_expired:
            logging.info('Access token expired, refreshing.')
            self.credentials.refresh(http=httplib2.Http())

    def GetArtifactURL(self, appname, by_method, version, filename):
        """Get the URL for an artifact on the Partner Android Build server.

        Args:
            appname: string, name of the app (f_companion).
            by_method: string, method used for downloading (label).
            version: string, "latest" or a specific MPM version.
            filename: string, simple file name (no parent dir or path).

        Returns:
            string, The URL for the resource specified by the parameters
        """
        return path_urljoin(self.GMS_DOWNLOAD_URL, appname, by_method,
            version, filename)

    def GetArtifact(self,
                     appname,
                     by_method,
                     version,
                     filename,
                     account_id,
                     local_filename=None):
        """Get artifact from Partner Android Build server.

        Args:
            appname: string, name of the app (f_companion).
            by_method: string, method used for downloading (label).
            version: string, "latest" or a specific MPM version.
            filename: string, simple file name (no parent dir or path).
            account_id: int, ID associated with the PAB account.
            local_filename: where the artifact gets downloaded locally.
            defaults to filename.

        Returns:
            boolean, whether the file was successfully downloaded
        """
        download_url = self.GetArtifactURL(appname, by_method, version,
                                             filename)

        headers = {}
        self.credentials.apply(headers)

        response = requests.get(
            download_url,
            params={'a': account_id},
            headers=headers,
            stream=True)
        response.raise_for_status()

        # if download filename is not specified, default to name on PAB
        if local_filename is None:
            local_filename = filename

        logging.info('%s now downloading...', download_url)
        with open(local_filename, 'wb') as handle:
            for block in response.iter_content(self.DEFAULT_CHUNK_SIZE):
                handle.write(block)

        return True
