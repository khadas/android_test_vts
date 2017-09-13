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
import getpass
import httplib2
import json
import logging
import os
import requests
from posixpath import join as path_urljoin

from oauth2client.client import flow_from_clientsecrets
from oauth2client.file import Storage
from oauth2client.tools import argparser
from oauth2client.tools import run_flow

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.support.ui import WebDriverWait

class PartnerAndroidBuildClient(object):
    """Client that manages Partner Android Build downloading.

    Attributes:
        CHROME_DRIVER_LOCATION: string, path to chromedriver
        CHROME_LOCATION: string, path to Chrome browser
        CLIENT_STORAGE: string, path to store credentials.
        DEFAULT_CHUNK_SIZE: int, number of bytes to download at a time.
        GMS_DOWNLOAD_URL: string, base url for downloading artifacts.
        PAB_URL: string, redirect url from Google sign-in to PAB
        SCOPE: string, URL for which to request access via oauth2.
        SVC_URL: string, path to buildsvc RPC
        XSRF_STORE: string, path to store xsrf token
        _credentials : oauth2client credentials object
        _xsrf : string, XSRF token from PAB website. expires after 7 days.
    """
    _credentials = None
    _xsrf = None
    CHROME_DRIVER_LOCATION = '/usr/bin/chromedriver'
    CHROME_LOCATION = '/usr/bin/google-chrome'
    CLIENT_SECRETS = os.path.join(
        os.path.dirname(__file__), 'client_secrets.json')
    CLIENT_STORAGE = os.path.join(os.path.dirname(__file__), 'credentials')
    DEFAULT_CHUNK_SIZE = 1024
    GMS_DOWNLOAD_URL = 'https://partnerdash.google.com/build/gmsdownload'
    PAB_URL = ('https://www.google.com/accounts/Login?&continue='
           'https://partner.android.com/build/')
    # need both of these scopes to access PAB downloader
    scopes = ('https://www.googleapis.com/auth/partnerdash',
              'https://www.googleapis.com/auth/alkali-base')
    SCOPE = ' '.join(scopes)
    SVC_URL = 'https://partner.android.com/build/u/0/_gwt/_rpc/buildsvc'
    XSRF_STORE = os.path.join(os.path.dirname(__file__), 'xsrf')

    def Authenticate(self):
        """Authenticate using OAuth2."""
        logging.info('Parsing flags, use --noauth_local_webserver' \
        'if running on remote machine')

        parser = argparse.ArgumentParser(parents=[argparser])
        flags = parser.parse_args()

        logging.info('Preparing OAuth token')
        flow = flow_from_clientsecrets(self.CLIENT_SECRETS, scope=self.SCOPE)
        storage = Storage(self.CLIENT_STORAGE)
        if self._credentials is None:
            self._credentials = storage.get()
        if self._credentials is None or self._credentials.invalid:
            logging.info('Credentials not found, authenticating.')
            self._credentials = run_flow(flow, storage, flags)

        if self._credentials.access_token_expired:
            logging.info('Access token expired, refreshing.')
            self._credentials.refresh(http=httplib2.Http())

        if self.XSRF_STORE is not None and os.path.isfile(self.XSRF_STORE):
            with open(self.XSRF_STORE, 'r') as handle:
                self._xsrf = handle.read()

    def GetXSRFToken(self):
        """Get artifact from Partner Android Build server.
        Currently takes email/password from command line but could be args

        Returns:
            boolean, whether the token was accessed and stored
        """
        chrome_options = Options()
        chrome_options.add_argument("--headless")
        chrome_options.binary_location = self.CHROME_LOCATION

        driver = webdriver.Chrome(
            executable_path=os.path.abspath(self.CHROME_DRIVER_LOCATION),
            chrome_options=chrome_options)

        driver.set_window_size(1080, 800)
        wait = WebDriverWait(driver, 10)

        driver.get(self.PAB_URL)

        query = driver.find_element_by_id("identifierId")
        query.click()
        query.send_keys(raw_input("Email: "))
        driver.find_element_by_id("identifierNext").click()

        pw = wait.until(EC.element_to_be_clickable((By.NAME, "password")))
        pw.click()
        pw.clear()
        pw.send_keys(getpass.getpass("Password: "))
        driver.find_element_by_id("passwordNext").click()

        try:
            wait.until(EC.title_contains("Partner Android Build"))
        except TimeoutException:
            raise ValueError('Wrong password or non-standard login flow')

        self._xsrf = driver.execute_script("return clientConfig.XSRF_TOKEN;")

        with open(self.XSRF_STORE, 'w') as handle:
            handle.write(self._xsrf)

        return True

    def GetArtifactURL(self, appname, by_method, version, filename,
                       account_id):
        """Get the URL for an artifact on the Partner Android Build server.

        Args:
            appname: string, name of the app (f_companion).
            by_method: string, method used for downloading (label).
            version: string, "latest" or a specific MPM version.
            filename: string, simple file name (no parent dir or path).
            account_id: int, ID associated with the PAB account.

        Returns:
            string, The URL for the resource specified by the parameters
        """
        return path_urljoin(self.GMS_DOWNLOAD_URL, appname, by_method, version,
                            filename) + '?a=' + str(account_id)

    def GetABArtifactURL(self, build_id, target, resource_id, branch,
                         release_candidate_name, internal, account_id):
        """Get the URL for an artifact on the PAB server, using buildsvc.

        Args:
            build_id: string/int, id of the build.
            target: string, "latest" or a specific version.
            resource_id: string, simple file name (no parent dir or path).
            branch: string, branch to pull resource from.
            release_candidate_name: string, Release candidate name, e.g."LDY85C"
            internal: int, whether the request is for an internal build artifact
            account_id: int, ID associated with the PAB account.

        Returns:
            string, The URL for the resource specified by the parameters
        """
        params = {
            "1": build_id,
            "2": target,
            "3": resource_id,
            "4": branch,
            "5": release_candidate_name,
            "6": internal
        }
        params = json.dumps(params)

        data = {
            "method": "downloadBuildArtifact",
            "params": params,
            "xsrf": self._xsrf
        }
        data = json.dumps(data)

        headers = {}
        self._credentials.apply(headers)
        headers['Content-Type'] = 'application/json'
        headers['x-alkali-account'] = account_id

        response = requests.post(self.SVC_URL, data=data, headers=headers)

        responseJSON = {}

        try:
            responseJSON = response.json()
        except ValueError:
            raise ValueError("Backend error -- check your account ID")

        if 'result' in responseJSON:
            result = responseJSON['result']
            if '1' in result:
                return result['1']
            if len(result) == 0:
                raise ValueError("Resource not found -- %s" % params)

        if 'error' in responseJSON and 'code' in responseJSON['error']:
            if responseJSON['error']['code'] == -32000:
                raise ValueError(
            "Bad XSRF token -- must be for the same account as your credentials"
                )
            if responseJSON['error']['code'] == -32001:
                raise ValueError("Expired XSRF token -- please refresh")

        raise ValueError(
            "Unknown response from server -- %s" % json.dumps(responseJSON))

    def GetArtifact(self, download_url, filename):
        """Get artifact from Partner Android Build server.

        Args:
            download_url: location of resource that we want to download
            filename: where the artifact gets downloaded locally.

        Returns:
            boolean, whether the file was successfully downloaded
        """

        headers = {}
        self._credentials.apply(headers)

        response = requests.get(download_url, headers=headers, stream=True)
        response.raise_for_status()

        logging.info('%s now downloading...', download_url)
        with open(filename, 'wb') as handle:
            for block in response.iter_content(self.DEFAULT_CHUNK_SIZE):
                handle.write(block)

        return True
