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

#include <iostream>

#include "BinderClient.h"

using namespace std;


int main(int /*argc*/, char** /*argv*/) {
  android::sp<android::vts::IVtsFuzzer> client = android::vts::GetBinderClient();
  if (!client.get()) return -1;

  int v = 10;
  client->Status(v);
  const int32_t adder = 5;
  int32_t sum = client->Call(v, adder);
  cout << "Addition result: " << v << " + " << adder << " = " << sum << endl;
  client->Exit();

  return 0;
}
