#!/bin/bash

# if "from future import standard_library" fails, please install future:
#   $ sudo apt-get install python-pip
#   $ sudo pip install future
#   $ sudo pip install futures
#   $ sudo pip install enum
#   $ sudo pip install concurrent
#   $ sudo pip install protobuf
# for protoc, please install protoc by:
#   $ apt-get install protobuf-compiler

python -m compileall .

protoc -I=proto --python_out=proto proto/AndroidSystemControlMessage.proto
protoc -I=proto --python_out=proto proto/InterfaceSpecificationMessage.proto
