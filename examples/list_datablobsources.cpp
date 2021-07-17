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
  // Get a base API object.
  hrgls::API api;
  hrgls_Status status = api.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not Open API: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 1;
  }

  // Ask the object for the list of available DataBlobSources.
  std::vector<hrgls::DataBlobSourceDescription> rends =
    api.GetAvailableDataBlobSources();
  status = api.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not get available DataBlobSources: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 2;
  }

  // Print the information about each of the DataBlobSources.
  std::cout << "Found " << rends.size() << " DataBlobSources" << std::endl;
  for (size_t i = 0; i < rends.size(); i++) {
    std::cout << " DataBlobSource name: " << rends[i].Name() << std::endl;
  }

  return 0;
}

