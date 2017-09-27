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
        BAD_XSRF_CODE: int, error code for bad XSRF token error
        BUILDARTIFACT_NAME_KEY: string, index in artifact containing name
        BUILD_BUILDID_KEY: string, index in build containing build_id
        BUILD_COMPLETED_STATUS: int, value of 'complete' build
        BUILD_STATUS_KEY: string, index in build object containing status.
        CHROME_DRIVER_LOCATION: string, path to chromedriver
        CHROME_LOCATION: string, path to Chrome browser
        CLIENT_STORAGE: string, path to store credentials.
        DEFAULT_CHUNK_SIZE: int, number of bytes to download at a time.
        DOWNLOAD_URL_KEY: string, index in downloadBuildArtifact containing url
        EXPIRED_XSRF_CODE: int, error code for expired XSRF token error
        GETBUILD_ARTIFACTS_KEY, string, index in build obj containing artifacts
        GMS_DOWNLOAD_URL: string, base url for downloading artifacts.
        LISTBUILD_BUILD_KEY: string, index in listBuild containing builds
        PAB_URL: string, redirect url from Google sign-in to PAB
        SCOPE: string, URL for which to request access via oauth2.
        SVC_URL: string, path to buildsvc RPC
        XSRF_STORE: string, path to store xsrf token
        _credentials : oauth2client credentials object
        _xsrf : string, XSRF token from PAB website. expires after 7 days.
    """
    _credentials = None
    _xsrf = None
    BAD_XSRF_CODE = -32000
    BUILDARTIFACT_NAME_KEY = '1'
    BUILD_BUILDID_KEY = '1'
    BUILD_COMPLETED_STATUS = 7
    BUILD_STATUS_KEY = '7'
    CHROME_DRIVER_LOCATION = '/usr/bin/chromedriver'
    CHROME_LOCATION = '/usr/bin/google-chrome'
    CLIENT_SECRETS = os.path.join(
        os.path.dirname(__file__), 'client_secrets.json')
    CLIENT_STORAGE = os.path.join(os.path.dirname(__file__), 'credentials')
    DEFAULT_CHUNK_SIZE = 1024
    DOWNLOAD_URL_KEY = '1'
    EXPIRED_XSRF_CODE = -32001
    GETBUILD_ARTIFACTS_KEY = '2'
    GMS_DOWNLOAD_URL = 'https://partnerdash.google.com/build/gmsdownload'
    LISTBUILD_BUILD_KEY = '1'
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

    def GetXSRFToken(self, email=None, password=None):
        """Get XSRF token. Prompt if email/password not provided.

        Args:
            email: string, optional. Gmail account of user logging in
            password: string, optional. Password of user logging in

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
        if email is None:
            email = raw_input("Email: ")
        query.send_keys(email)
        driver.find_element_by_id("identifierNext").click()

        pw = wait.until(EC.element_to_be_clickable((By.NAME, "password")))
        pw.clear()

        if password is None:
            pw.send_keys(getpass.getpass("Password: "))
        else:
            pw.send_keys(password)

        driver.find_element_by_id("passwordNext").click()

        try:
            wait.until(EC.title_contains("Partner Android Build"))
        except TimeoutException:
            raise ValueError('Wrong password or non-standard login flow')

        self._xsrf = driver.execute_script("return clientConfig.XSRF_TOKEN;")
        with open(self.XSRF_STORE, 'w') as handle:
            handle.write(self._xsrf)

        return True

    def CallBuildsvc(self, method, params, account_id):
        """Call the buildsvc RPC with given parameters.

        Args:
            method: string, name of method to be called in buildsvc
            params: dict, parameters to RPC call
            account_id: int, ID associated with the PAB account.

        Returns:
            dict, result from RPC call
        """
        params = json.dumps(params)

        data = {"method": method, "params": params, "xsrf": self._xsrf}
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
            return responseJSON['result']

        if 'error' in responseJSON and 'code' in responseJSON['error']:
            if responseJSON['error']['code'] == self.BAD_XSRF_CODE:
                raise ValueError(
            "Bad XSRF token -- must be for the same account as your credentials"
                )
            if responseJSON['error']['code'] == self.EXPIRED_XSRF_CODE:
                raise ValueError("Expired XSRF token -- please refresh")

        raise ValueError(
            "Unknown response from server -- %s" % json.dumps(responseJSON))

    def GetBuildList(self,
                     account_id,
                     branch,
                     target,
                     page_token="",
                     max_results=10,
                     include_internal_build_info=1):
        """Get the list of builds for a given account, branch and target
        Args:
            account_id: int, ID associated with the PAB account.
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.
            page_token: string, token used for pagination
            max_results: maximum build results the build list contains, e.g. 25
            include_internal_build_info: int, whether to query internal build

        Returns:
            list of dicts representing the builds, descending in time
        """
        params = {
            "1": branch,
            "2": target,
            "3": page_token,
            "4": max_results,
            "7": include_internal_build_info
        }

        result = self.CallBuildsvc("listBuild", params, account_id)
        # in listBuild response, index '1' contains builds
        if self.LISTBUILD_BUILD_KEY in result:
            return result[self.LISTBUILD_BUILD_KEY]
        raise ValueError("Build list not found -- %s" % params)

    def GetLatestBuildId(self, account_id, branch, target):
        """Get the most recent build_id for a given account, branch and target
        Args:
            account_id: int, ID associated with the PAB account.
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.

        Returns:
            string, most recent build id
        """
        # TODO: support pagination, maybe?
        build_list = self.GetBuildList(
            account_id=account_id, branch=branch, target=target)
        if len(build_list) == 0:
            raise ValueError(
                'No builds found for account_id=%s, branch=%s, target=%s' %
                (account_id, branch, target))
        for build in build_list:
            # get build status: 7 = completed build
            if build.get(self.BUILD_STATUS_KEY,
                         None) == self.BUILD_COMPLETED_STATUS:
                # return build id (index '1')
                return build[self.BUILD_BUILDID_KEY]
        raise ValueError(
            'No complete builds found: %s failed or incomplete builds found' %
            len(build_list))

    def GetBuildArtifacts(self, account_id, build_id, branch, target):
        """Get the list of build artifacts for an account, build, target, branch
        Args:
            account_id: int, ID associated with the PAB account.
            build_id: string, ID of the build
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.

        Returns:
            string, most recent build id
        """
        params = {"1": build_id, "2": target, "3": branch}

        result = self.CallBuildsvc("getBuild", params, account_id)
        # in getBuild response, index '2' contains the artifacts
        if self.GETBUILD_ARTIFACTS_KEY in result:
            return result[self.GETBUILD_ARTIFACTS_KEY]
        if len(result) == 0:
            raise ValueError("Build artifacts not found -- %s" % params)

    def GetArtifactURL(self, account_id, appname, by_method, version,
                       filename):
        """Get the URL for an artifact on the Partner Android Build server.

        Args:
            account_id: int, ID associated with the PAB account.
            appname: string, name of the app (f_companion).
            by_method: string, method used for downloading (label).
            version: string, "latest" or a specific MPM version.
            filename: string, simple file name (no parent dir or path).

        Returns:
            string, The URL for the resource specified by the parameters
        """
        return path_urljoin(self.GMS_DOWNLOAD_URL, appname, by_method, version,
                            filename) + '?a=' + str(account_id)

    def GetABArtifactURL(self, account_id, build_id, target, resource_id,
                         branch, release_candidate_name, internal):
        """Get the URL for an artifact on the PAB server, using buildsvc.

        Args:
            account_id: int, ID associated with the PAB account.
            build_id: string/int, id of the build.
            target: string, "latest" or a specific version.
            resource_id: string, simple file name (no parent dir or path).
            branch: string, branch to pull resource from.
            release_candidate_name: string, Release candidate name, e.g."LDY85C"
            internal: int, whether the request is for an internal build artifact

        Returns:
            string, The URL for the resource specified by the parameters
        """
        params = {
            "1": str(build_id),
            "2": target,
            "3": resource_id,
            "4": branch,
            "5": release_candidate_name,
            "6": internal
        }

        result = self.CallBuildsvc(
            method='downloadBuildArtifact',
            params=params,
            account_id=account_id)

        # in downloadBuildArtifact response, index '1' contains the url
        if self.DOWNLOAD_URL_KEY in result:
            return result[self.DOWNLOAD_URL_KEY]
        if len(result) == 0:
            raise ValueError("Resource not found -- %s" % params)

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

    def GetLatestArtifact(self, account_id, branch, target, artifact_name):
        """Get the most recent artifact for an account, branch, target and name
        Args:
            account_id: int, ID associated with the PAB account.
            branch: string, branch to pull resource from.
            target: string, "latest" or a specific version.
            artifact_name: name of artifact, e.g. aosp_arm64_ab-img-4353141.zip
                ({id} will automatically get replaced with build ID)

        Returns:
            string, filename of downloaded artifact
        """
        build_id = self.GetLatestBuildId(
            account_id=account_id, branch=branch, target=target)

        artifacts = self.GetBuildArtifacts(
            account_id=account_id,
            build_id=build_id,
            branch=branch,
            target=target)

        if len(artifacts) == 0:
            raise ValueError(
                "No artifacts found for build_id=%s, branch=%s, target=%s" %
                (build_id, branch, target))

        artifact_name = artifact_name.format(id=build_id)
        # in build artifact response, index '1' contains the name
        artifact_names = [
            artifact[self.BUILDARTIFACT_NAME_KEY] for artifact in artifacts
        ]
        if artifact_name not in artifact_names:
            raise ValueError("%s not found in artifact list" % artifact_name)

        url = self.GetABArtifactURL(
            account_id=account_id,
            build_id=build_id,
            target=target,
            resource_id=artifact_name,
            branch=branch,
            release_candidate_name="",
            internal=0)
        self.GetArtifact(url, artifact_name)
        return artifact_name
