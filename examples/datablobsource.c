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

#include <stdio.h>
#include <hrgls_api.h>

void HandleBlobCallback(hrgls_DataBlob const blob, void *userData)
{
  // Set *done when it is time to quit.
  bool *done = (bool*)(userData);

  // Get the data pointer and its size from the blob.
  const uint8_t  *data;
  uint32_t size;
  hrgls_Status status = hrgls_DataBlobGetData(blob, &data, &size);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not get blob data and size in callback: %s\n",
      hrgls_ErrorMessage(status));
    *done = true;
    return;
  }

  // Do whatever we want with the blob.
  /// @todo Replace this code with whatever is desired.
  static size_t count = 0;
  if (++count >= 10) {
    *done = true;
  }

  // Release the data from the blob.
  status = hrgls_DataBlobReleaseData(blob);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not get blob data and size in callback: %s\n",
      hrgls_ErrorMessage(status));
    *done = true;
  }

  // In the C interface, we always need to destroy the blob that
  // was returned to us.
  status = hrgls_DataBlobDestroy(blob);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not get destroy blob: %s\n", hrgls_ErrorMessage(status));
    *done = true;
  }
}

int main(int argc, const char *argv[])
{
  // Get a base API object, specifying default parameters.
  hrgls_Status status;
  hrgls_APICreateParams params;
  status = hrgls_APICreateParametersCreate(&params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not create API open parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 1;
  }

  hrgls_API api;
  status = hrgls_APICreate(&api, params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not create API with default parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 2;
  }
  status = hrgls_APICreateParametersDestroy(params);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API open parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 1000;
  }

  hrgls_StreamProperties sp;
  status = hrgls_StreamPropertiesCreate(&sp);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not create stream properties: %s\n", hrgls_ErrorMessage(status));
    return 9;
  }

  // Create the creation properties that we'll need to create a stream
  // and set its values to the ones obtained above.
  hrgls_DataBlobSourceCreateParams rscp;
  status = hrgls_DataBlobSourceCreateParametersCreate(&rscp);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not create stream parameters: %s\n", hrgls_ErrorMessage(status));
    return 10;
  }
  status = hrgls_DataBlobSourceCreateParametersSetAPI(rscp, api);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not create set api: %s\n", hrgls_ErrorMessage(status));
    return 11;
  }
  status = hrgls_DataBlobSourceCreateParametersSetStreamProperties(rscp, sp);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not create set stream properties: %s\n", hrgls_ErrorMessage(status));
    return 16;
  }

  // Create the stream, then destroy the creation parameters to avoid leaking
  // memory.
  hrgls_DataBlobSource stream;
  status = hrgls_DataBlobSourceCreate(&stream, rscp);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not create create stream: %s\n", hrgls_ErrorMessage(status));
    return 17;
  }
  status = hrgls_DataBlobSourceCreateParametersDestroy(rscp);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could destroy stream parameters: %s\n", hrgls_ErrorMessage(status));
    return 18;
  }
  status = hrgls_StreamPropertiesDestroy(sp);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not destroy stream properties: %s\n", hrgls_ErrorMessage(status));
    return 1001;
  }

  { // This shows how to read the blobs using a callback handler.
    printf("Callback-based blob reading\n");

    // Set a callback handler and then start streaming
    bool done = false;
    status = hrgls_DataBlobSourceSetStreamCallback(stream, HandleBlobCallback, &done);
    if (hrgls_STATUS_OKAY != status) {
      fprintf(stderr, "Could not set stream callback: %s\n", hrgls_ErrorMessage(status));
      return 20;
    }
    status = hrgls_DataBlobSourceSetStreamingState(stream, true);
    if (hrgls_STATUS_OKAY != status) {
      fprintf(stderr, "Could not set streaming state on: %s\n", hrgls_ErrorMessage(status));
      return 21;
    }

    // Run until the callback handler sets done.
    while (!done) {};

    // Stop the stream and then unhook the callback handler.
    status = hrgls_DataBlobSourceSetStreamingState(stream, false);
    if (hrgls_STATUS_OKAY != status) {
      fprintf(stderr, "Could not set streaming state off: %s\n", hrgls_ErrorMessage(status));
      return 22;
    }
    status = hrgls_DataBlobSourceSetStreamCallback(stream, NULL, NULL);
    if (hrgls_STATUS_OKAY != status) {
      fprintf(stderr, "Could not reset stream callback: %s\n", hrgls_ErrorMessage(status));
      return 23;
    }
  }

  { // This shows how to read the blobs without a callback handler.
    printf("Get-based blob reading\n");

    // Start streaming
    status = hrgls_DataBlobSourceSetStreamingState(stream, true);
    if (hrgls_STATUS_OKAY != status) {
      fprintf(stderr, "Could not set streaming state on: %s\n", hrgls_ErrorMessage(status));
      return 24;
    }

    size_t count = 0;
    do {
      // Return immediately if we don't get a blob.
      struct timeval timeout = { 0,0 };
      hrgls_DataBlob blob;
      status = hrgls_DataBlobSourceGetNextBlob(stream, &blob, timeout);
      if (hrgls_STATUS_OKAY == status) {

        const uint8_t  *data;
        uint32_t size;
        status = hrgls_DataBlobGetData(blob, &data, &size);
        if (status != hrgls_STATUS_OKAY) {
          fprintf(stderr, "Could not get blob data and size in main: %s\n",
            hrgls_ErrorMessage(status));
          return 27;
        }

        // Do whatever we want with the blob.
        /// @todo Replace the print and increment with the work we want to do.
        if (size >= 2) {
          printf(" first character = %d\n", (int)(data[0]));
          printf(" second character = %d\n", (int)(data[1]));
        }
        count++;

        // Release the data from the blob.
        status = hrgls_DataBlobReleaseData(blob);
        if (status != hrgls_STATUS_OKAY) {
          fprintf(stderr, "Could not get blob data and size in main: %s\n",
            hrgls_ErrorMessage(status));
          return 29;
        }

        // In the C interface, we always need to destroy the blob that
        // was returned to us.
        status = hrgls_DataBlobDestroy(blob);
        if (status != hrgls_STATUS_OKAY) {
          fprintf(stderr, "Could not get destroy blob: %s\n", hrgls_ErrorMessage(status));
          return 30;
        }

      } else if (status != hrgls_STATUS_TIMEOUT) {
        fprintf(stderr, "Bad blob received: %s\n", hrgls_ErrorMessage(status));
        return 31;
      }
    } while (count < 10);

    // Stop streaming.
    status = hrgls_DataBlobSourceSetStreamingState(stream, false);
    if (hrgls_STATUS_OKAY != status) {
      fprintf(stderr, "Could not set streaming state off: %s\n", hrgls_ErrorMessage(status));
      return 32;
    }
  }

  // Done with the stream
  status = hrgls_DataBlobSourceDestroy(stream);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not destroy stream: %s\n", hrgls_ErrorMessage(status));
    return 100;
  }

  // Done with the API
  status = hrgls_APIDestroy(api);
  if (hrgls_STATUS_OKAY != status) {
    fprintf(stderr, "Could not destroy API with default parameters: %s\n",
      hrgls_ErrorMessage(status));
    return 38;
  }

  return 0;
}
