//
// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "VtsFmqDriver/VtsFmqDriver.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>

using android::hardware::kSynchronizedReadWrite;
using android::hardware::kUnsynchronizedWrite;
using namespace std;

namespace android {
namespace vts {

// A test that initializes a single writer and a single reader.
class SyncReadWrites : public ::testing::Test {
 protected:
  virtual void SetUp() {
    static constexpr size_t NUM_ELEMS = 2048;

    // initialize a writer
    writer_id_ = manager_.CreateFmq("uint16_t", true, NUM_ELEMS, false);
    ASSERT_NE(writer_id_, -1);

    // initialize a reader
    reader_id_ = manager_.CreateFmq("uint16_t", true, writer_id_);
    ASSERT_NE(reader_id_, -1);
  }

  virtual void TearDown() {}

  VtsFmqDriver manager_;
  QueueId writer_id_;
  QueueId reader_id_;
};

// Tests the reader and writer are set up correctly.
TEST_F(SyncReadWrites, SetupBasicTest) {
  static constexpr size_t NUM_ELEMS = 2048;

  // check if the writer has a valid queue
  ASSERT_TRUE(manager_.IsValid("uint16_t", true, writer_id_));

  // check queue size on writer side
  size_t writer_queue_size;
  ASSERT_TRUE(manager_.GetQuantumCount("uint16_t", true, writer_id_,
                                       &writer_queue_size));
  ASSERT_EQ(NUM_ELEMS, writer_queue_size);

  // check queue element size on writer side
  size_t writer_elem_size;
  ASSERT_TRUE(
      manager_.GetQuantumSize("uint16_t", true, writer_id_, &writer_elem_size));
  ASSERT_EQ(sizeof(uint16_t), writer_elem_size);

  // check space available for writer
  size_t writer_available_writes;
  ASSERT_TRUE(manager_.AvailableToWrite("uint16_t", true, writer_id_,
                                        &writer_available_writes));
  ASSERT_EQ(NUM_ELEMS, writer_available_writes);

  // check if the reader has a valid queue
  ASSERT_TRUE(manager_.IsValid("uint16_t", true, reader_id_));

  // check queue size on reader side
  size_t reader_queue_size;
  ASSERT_TRUE(manager_.GetQuantumCount("uint16_t", true, reader_id_,
                                       &reader_queue_size));
  ASSERT_EQ(NUM_ELEMS, reader_queue_size);

  // check queue element size on reader side
  size_t reader_elem_size;
  ASSERT_TRUE(
      manager_.GetQuantumSize("uint16_t", true, reader_id_, &reader_elem_size));
  ASSERT_EQ(sizeof(uint16_t), reader_elem_size);

  // check items available for reader
  size_t reader_available_reads;
  ASSERT_TRUE(manager_.AvailableToRead("uint16_t", true, reader_id_,
                                       &reader_available_reads));
  ASSERT_EQ(0, reader_available_reads);
}

// Tests a basic writer and reader interaction.
// Reader reads back data written by the writer correctly.
TEST_F(SyncReadWrites, ReadWriteSuccessTest) {
  static constexpr size_t DATA_SIZE = 64;
  uint16_t write_data[DATA_SIZE];
  uint16_t read_data[DATA_SIZE];

  // initialize the data to transfer
  for (uint16_t i = 0; i < DATA_SIZE; i++) {
    write_data[i] = i;
  }

  // writer should succeed
  ASSERT_TRUE(manager_.WriteFmq("uint16_t", true, writer_id_,
                                static_cast<void*>(write_data), DATA_SIZE));

  // reader should succeed
  ASSERT_TRUE(manager_.ReadFmq("uint16_t", true, reader_id_,
                               static_cast<void*>(read_data), DATA_SIZE));

  // check if the data is read back correctly
  ASSERT_EQ(0, memcmp(write_data, read_data, DATA_SIZE));
}

}  // namespace vts
}  // namespace android
