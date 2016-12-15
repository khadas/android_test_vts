#!/bin/bash

# Modify any import statements (to remove subdir path)

## Modify import statement in proto/AndroidSystemControlMessage.proto
sed -i 's/import "test\/vts\/proto\/InterfaceSpecificationMessage.proto";/import "InterfaceSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto
## Compile proto/AndroidSystemControlMessage.proto to .py code
protoc -I=proto --python_out=proto proto/AndroidSystemControlMessage.proto
## Restore import statement in proto/AndroidSystemControlMessage.proto
sed -i 's/import "InterfaceSpecificationMessage.proto";/import "test\/vts\/proto\/InterfaceSpecificationMessage.proto";/g' proto/AndroidSystemControlMessage.proto

protoc -I=proto --python_out=proto proto/InterfaceSpecificationMessage.proto
protoc -I=proto --python_out=proto proto/VtsReportMessage.proto

# Compile all the python source codes
python -m compileall .
