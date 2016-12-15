#!/bin/bash

# Modifies any import statements (to remove subdir path)

## Modifies import statement in proto files.
sed -i 's/import "test\/vts\/proto\/InterfaceSpecificationMessage.proto";/import "InterfaceSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto
sed -i 's/import "test\/vts\/proto\/InterfaceSpecificationMessage.proto";/import "InterfaceSpecificationMessage.proto";/g' proto/ComponentSpecificationMessage.proto

## Compiles modified proto files to .py files.
protoc -I=proto --python_out=proto proto/AndroidSystemControlMessage.proto
protoc -I=proto --python_out=proto proto/ComponentSpecificationMessage.proto

## Restores import statement in proto files.
sed -i 's/import "InterfaceSpecificationMessage.proto";/import "test\/vts\/proto\/InterfaceSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto
sed -i 's/import "InterfaceSpecificationMessage.proto";/import "test\/vts\/proto\/InterfaceSpecificationMessage.proto";/g' proto/ComponentSpecificationMessage.proto

protoc -I=proto --python_out=proto proto/InterfaceSpecificationMessage.proto
protoc -I=proto --python_out=proto proto/VtsReportMessage.proto

# Compiles all the python source codes.
python -m compileall .

