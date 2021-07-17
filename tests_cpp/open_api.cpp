/*
 * Copyright 2020 ReliaSolve, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <hrgls_api_defs.hpp>

int main(int argc, const char *argv[])
{
  { // Putting test code in a basic block so that objects are destroyed before
    // we print the Success! message below.

    // Get a base API object, specifying default parameters.
    hrgls::API api;
    hrgls_Status status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not Open API with default parameters: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 2;
    }

    // Get a base API object, specifying a name, an non-empty
    // credentials.
    std::string myName = "Test";
    std::vector<uint8_t> myCredentials;

    hrgls::API api2(myName, myCredentials);
    status = api2.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not Open API with name and credentials: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 3;
    }
    hrgls_VERSION version = api2.GetVersion();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not get version: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 4;
    }
    status = api2.SetVerbosity(201);
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set verbosity: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 5;
    }
  }

  std::cout << "Success!" << std::endl;
  return 0;
}
