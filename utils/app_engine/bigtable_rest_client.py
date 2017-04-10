
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import base64
import json
import logging
import subprocess

from oauth2client import service_account

_OPENID_SCOPE = 'openid'

class BigtableClient(object):
    """Instance of the Dashboard Bigtable REST client.

    Attributes:
        table_name: String, The name of the table.
        post_cmd: String, The command-line string to post data to the dashboard,
                  e.g. 'wget <url> --post-data '
        service_json_path: String, The path to the service account keyfile
                           created from Google App Engine settings.
        auth_token: ServiceAccountCredentials object or None if not
                    initialized.
    """

    def __init__(self, table_name, post_cmd, service_json_path):
        self.post_cmd = post_cmd
        self.table_name = table_name
        self.service_json_path = service_json_path
        self.auth_token = None

    def _GetToken(self):
        """Gets an OAuth2 token using from a service account json keyfile.

        Uses the service account keyfile located at 'service_json_path', provided
        to the constructor, to request an OAuth2 token.

        Returns:
            String, an OAuth2 token using the service account credentials.
            None if authentication fails.
        """
        if not self.auth_token:
            try:
                self.auth_token = service_account.ServiceAccountCredentials.from_json_keyfile_name(
                    self.service_json_path, [_OPENID_SCOPE])
            except IOError as e:
                logging.error("Error reading service json keyfile: %s", e)
                return None
            except (ValueError, KeyError) as e:
                logging.error("Invalid service json keyfile: %s", e)
                return None
        return str(self.auth_token.get_access_token().access_token)

    def PutRow(self, row_key, family, qualifier, value):
        """Puts a value into an HBase cell via REST.

        Puts a value in the cell specified by the row, family, and qualifier.
        This assumes that the table has already been created with the column
        family in its schema.

        Args:
            row_key: String, The name of the row in which to insert data
            family: String, The column family in which to insert data
            qualifier: String, The column qualifier in which to insert data
            value: String, The data to insert into the row cell specified

        Returns:
            True if successful, False otherwise
        """
        token = self._GetToken()
        if not token:
            return False

        data = {
            "accessToken" : token,
            "verb" : "insertRow",
            "tableName" : self.table_name,
            "rowKey" : row_key,
            "family" : family,
            "qualifier" : qualifier,
            "value" : base64.b64encode(value)
        }
        p = subprocess.Popen(
            self.post_cmd.format(data=json.dumps(data)),
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        output, err = p.communicate()

        if p.returncode or err:
            logging.error("Row insertion failed: %s", err)
            return False
        return True

    def CreateTable(self, families):
        """Creates a table with the provided column family names.

        Safe to call if the table already exists, it will just fail
        silently.

        Args:
            families: list, The list of column family names with which to create
                      the table
        """
        token = self._GetToken()
        if not token:
            return

        data = {
            "accessToken" : token,
            "verb": "createTable",
            "tableName": self.table_name,
            "familyNames": families
        }
        p = subprocess.Popen(
            self.post_cmd.format(data=json.dumps(data)),
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
