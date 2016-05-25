#!/bin/bash

# if "from future import standard_library" fails, please install future:
#   $ sudo apt-get install python-pip
#   $ sudo pip install future
#   $ sudo pip install futures
#   $ sudo pip install enum
#   $ sudo pip install concurrent
# for protoc, please install protoc by:
#   $ apt-get install protobuf-compiler

python -m compileall .

protoc -I=runners/host/proto --python_out=runners/host/proto runners/host/proto/AndroidSystemControlMessage.proto
protoc -I=runners/host/proto --python_out=runners/host/proto runners/host/proto/InterfaceSpecificationMessage.proto
