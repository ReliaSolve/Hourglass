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

void HandleBlobCallback(hrgls::datablob::DataBlob &blob, void *userData)
{
  // Set *done when it is time to quit.
  bool *done = static_cast<bool*>(userData);

  // Do whatever we want with the blob.
  /// @todo Replace this code with whatever is desired.
  static size_t count = 0;
  if (++count >= 10) {
    *done = true;
  }

  // Release the data from the blob.
  blob.ReleaseData();
  hrgls_Status status = blob.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not release blob data in callback: "
      << hrgls_ErrorMessage(status) << std::endl;
    *done = true;
  }
}

int main(int argc, const char *argv[])
{
  // Get a base API object and DataBlobSource, specifying default parameters.
  hrgls::API api;
  hrgls_Status status = api.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not Open API: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 1;
  }

  // Create a stream using default stream properties.
  hrgls::StreamProperties sp;
  status = sp.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not create stream properties: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 8;
  }
  hrgls::datablob::DataBlobSource *stream = new hrgls::datablob::DataBlobSource(
    api, sp);
  status = stream->GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not create stream: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 9;
  }

  { /// This shows how to read the blobs using a callback handler.
    std::cout << "Callback-based blob reading" << std::endl;

    // Set a callback handler for incoming blobs and then start streaming.
    bool done = false;
    if (stream->SetStreamCallback(HandleBlobCallback, &done) != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set callback handler: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 11;
    }
    if (stream->SetStreamingState(true) != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state on: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 12;
    }

    // Run until the callback handler sets done.
    while (!done) {};

    // Unhook the callback handler after stopping the stream.
    if (stream->SetStreamingState(false) != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state off: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 13;
    }
    if (stream->SetStreamCallback(nullptr, nullptr) != hrgls_STATUS_OKAY) {
      std::cerr << "Could not reset callback handler: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 14;
    }
  }

  { /// This shows how to read the blobs without a callback handler.
    std::cout << "Get-based blob reading" << std::endl;

    // Start streaming
    if (stream->SetStreamingState(true) != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state on: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 15;
    }

    size_t count = 0;
    do {
      hrgls::datablob::DataBlob blob = stream->GetNextBlob();
      status = stream->GetStatus();
      if (hrgls_STATUS_OKAY == status) {

        // Do whatever we want with the blob.
        /// @todo Replace the print and increment with the work we want to do.
        if (blob.Size() >= 2) {
          std::cout << " first character = " << static_cast<int>(blob.Data()[0]) << std::endl;
          std::cout << " second character = " << static_cast<int>(blob.Data()[1]) << std::endl;
        }
        count++;

        // Release the data from the blob.
        blob.ReleaseData();
        hrgls_Status status = blob.GetStatus();
        if (status != hrgls_STATUS_OKAY) {
          std::cerr << "Could not release blob data in main program: "
            << hrgls_ErrorMessage(status) << std::endl;
          return 17;
        }
      } else if (status != hrgls_STATUS_TIMEOUT) {
        std::cerr << "Bad blob received: " << hrgls_ErrorMessage(status) << std::endl;
        return 18;
      }

    } while (count < 10);

    // Stop streaming
    if (stream->SetStreamingState(false) != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state off: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 19;
    }
  }

  delete stream;

  return 0;
}
