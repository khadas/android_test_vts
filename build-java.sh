#!/bin/bash

protoc -I=proto --java_out=web/dashboard/appengine/servlet/src/main/java proto/VtsReportMessage.proto
