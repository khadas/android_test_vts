/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __VTS_DRIVERS_SHELL_MSG_PROTOCOL_H_
#define __VTS_DRIVERS_SHELL_MSG_PROTOCOL_H_

#include "test/vts/proto/VtsDriverControlMessage.pb.h"

namespace android {
namespace vts {

/*
 * Length of buffer to store the length header text
 */
extern int kLengthTextBufferSize;

/*
 * Unix domain socket address prefix
 */
extern char kAddressUnixSocketPrefix[];


/*
 * read the length information stored in header.
 * The length will be the return value; -1 indicates failure.
 */
extern int read_len_header(int fd);


/*
 * write a message to a socket connection using our protocol:
 * The length of the message will be encoded in text and sent at first.
 * A new line character will be used to separate the length text and message.
 */
extern int write_with_length(int connection_fd, char* msg);


/*
 * read a message from a socket connection encoded according to our protocol.
 * Please remember to free the message after use.
 */
extern char* read_with_length(int connection_fd);


/**
 * Write a protocol buffer message to socket.
 */
extern int write_pb_msg(int fd, google::protobuf::Message* msg);


/**
 * Read a protocol buffer message from socket.
 */
extern int read_pb_msg(int fd, google::protobuf::Message* msg);

}  // namespace vts
}  // namespace android

#endif  // __VTS_DRIVERS_SHELL_MSG_PROTOCOL_H_
