#!/bin/bash

python -m compileall .

protoc -I=proto --python_out=proto proto/AndroidSystemControlMessage.proto
protoc -I=proto --python_out=proto proto/InterfaceSpecificationMessage.proto
