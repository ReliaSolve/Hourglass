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
