#!/bin/bash
./agent.sh 192.168.0.177 $1

export TARGET_IP=127.0.0.1; export TARGET_PORT=$1; export PYTHONPATH=$PYTHONPATH:..; python -m vts.testcases.host.sample.SampleTestcase
