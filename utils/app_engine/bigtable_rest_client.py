#!/usr/bin/env python

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

import logging

import base64
from collections import OrderedDict
import json

import requests

_DEFAULT_BASE_URL="http://146.148.105.120:8080"
_PROJECT_ID='google.com:android-vts-internal'
_INSTANCE_ID='vts-dev'
_COLUMN_FAMILY_ID='cf1'


class HbaseRestClient(object):
    """Instane of an Hbase REST client.

    Attributes:
        base_url: The hostname and port of the Hbase REST server.
                  e.g 'http://130.211.170.242:8080'
        table_name: The name of the table
    """

    def __init__(self, table_name, base_url=_DEFAULT_BASE_URL):
        self.base_url = base_url
        self.table_name = table_name

    def PutRow(self, row_key, column, value):
        """Puts a value into an HBase cell via REST.

        This puts a value in the fully qualified column name. This assumes
        that the table has already been created with the column family in its
        schema. If it doesn't exist, you can use create_table() to doso.

        Args:
            row_key: The row we want to put a value into.
            column: The fully qualified column (e.g. my_column_family:content)
            value: A string representing the sequence of bytes we want to
                   put into the cell
        """
        row_key_encoded = base64.b64encode(row_key)
        column_encoded = base64.b64encode(column)
        value_encoded = base64.b64encode(value)

        cell = OrderedDict([
            ("key", row_key_encoded),
            ("Cell", [{"column": column_encoded, "$": value_encoded}])
        ])
        rows = [cell]
        json_output = {"Row": rows}
        r = requests.post(
            self.base_url + "/" + self.table_name + "/" + row_key,
            data=json.dumps(json_output),
            headers={
               "Content-Type": "application/json",
               "Accept": "application/json",
            })
        if r.status_code != 200:
            logging.error("got status code %d when putting", r.status_code)

    def GetRow(self, row_key):
        """Returns a value from the first column in a row.

        Args:
            row_key: The row to return the value from

        Returns:
            The bytes in the cell represented as a Python string.
        """
        request = requests.get(self.base_url + "/" + self.table_name + "/" +
                               row_key,
                               headers={"Accept": "application/json"})
        if request.status_code != 200:
            return None
        text = json.loads(request.text)
        value = base64.b64decode(text['Row'][0]['Cell'][0]['$'])
        return value

    def DeleteRow(self, row_key):
        """Deletes a row.

        Args:
            row_key: The row key of the row to delete
        """
        requests.delete(self.base_url + "/" + self.table_name + "/" + row_key)

    def CreateTable(self, table_name, column_family):
        """Creates a table with a single column family.

        It's safe to call if the table already exists, it will just fail
        silently.

        Args:
            table_name: The name of the
            column_family: The column family to create the table with.
        """
        json_output = {"name": table_name,
                       "ColumnSchema": [{"name": column_family}]}
        requests.post(self.base_url + '/' + table_name + '/schema',
                      data=json.dumps(json_output),
                      headers={
                          "Content-Type": "application/json",
                          "Accept": "application/json"
                      })

    def GetTables(self):
        """Returns a list of the tables in Hbase.

        Returns:
            A list of the table names as strings
        """
        r = requests.get(self.base_url)
        tables = r.text.split('\n')
        return tables

