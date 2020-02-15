/*
 * Copyright 2020 ReliaSolve, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <hrgls_api.hpp>

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

