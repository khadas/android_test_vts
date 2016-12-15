#!/bin/bash

PYTHONPATH=$PYTHONPATH:.. python -m vts.runners.host.tcp_server.vts_tcp_server_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.app_engine.bigtable_client_test
PYTHONPATH=$PYTHONPATH:.. python -m vts.utils.python.coverage.GCNO_test
