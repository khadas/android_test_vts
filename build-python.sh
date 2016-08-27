#!/bin/bash

# Modifies any import statements (to remove subdir path)

## Modifies import statement in proto files.
sed -i 's/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/import "ComponentSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto

## Compiles modified proto files to .py files.
protoc -I=proto --python_out=proto proto/AndroidSystemControlMessage.proto
protoc -I=proto --python_out=proto proto/ComponentSpecificationMessage.proto

## Restores import statement in proto files.
sed -i 's/import "ComponentSpecificationMessage.proto";/import "test\/vts\/proto\/ComponentSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto

protoc -I=proto --python_out=proto proto/ComponentSpecificationMessage.proto
protoc -I=proto --python_out=proto proto/VtsReportMessage.proto

# Compiles all the python source codes.
python -m compileall .

