#!/bin/bash

# if "from future import standard_library" fails, please install future:
#   $ sudo apt-get install python-pip
#   $ sudo pip install future
#   $ sudo pip install futures
#   $ sudo pip install enum
#   $ sudo pip install concurrent

protoc -I=runners/host/proto --python_out=runners/host/proto proto/AndroidSystemControlMessage.proto
protoc -I=runners/host/proto --python_out=runners/host/proto proto/InterfaceSpecificationMessage.proto

python -m compileall .
