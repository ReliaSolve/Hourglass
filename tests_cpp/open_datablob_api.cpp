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
      return 1;
    }

    // Attempt to open a stream on a DataBlobSource with an empty name.
    hrgls::StreamProperties sp;
    hrgls::datablob::DataBlobSource stream(api, sp);

    if (hrgls_STATUS_OKAY != stream.GetStatus()) {
      std::cerr << "Could not Open DataBlobSource with default parameters: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 2;
    }

    // Attempt to open a stream on each DataBlobSource by name.
    std::vector<hrgls::DataBlobSourceDescription> dbs = api.GetAvailableDataBlobSources();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not get available DataBlobSources from API: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 3;
    }
    for (size_t i = 0; i < dbs.size(); i++) {
      hrgls::datablob::DataBlobSource stream2(api, sp, dbs[i].Name());
      if (hrgls_STATUS_OKAY != stream2.GetStatus()) {
        std::cerr << "Could not Open DataBlobSource " << i << " with specific name: "
          << hrgls_ErrorMessage(status) << std::endl;
        return 4;
      }
      std::string name = stream2.GetInfo().Name();
      if (hrgls_STATUS_OKAY != stream2.GetStatus()) {
        std::cerr << "Could not Get name for stream " << i << " with specific name: "
          << hrgls_ErrorMessage(status) << std::endl;
        return 5;
      }
      std::cout << "Opened stream " << name << " on DataBlobSource " << dbs[i].Name() << std::endl;
    }
  }

  std::cout << "Success!" << std::endl;
  return 0;
}

