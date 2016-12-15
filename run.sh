#!/bin/bash

export PYTHONPATH=$PYTHONPATH:..; python -m vts.runners.host.fuzzer.VtsFuzzerRunnerTest
