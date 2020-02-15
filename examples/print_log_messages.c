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

void HandleMessageCallback(hrgls_Message message, void *userData)
{
  // Increment to keep track of how many messages arrived.
  size_t *count = (size_t*)(userData);
  (*count)++;

  // Print information about the message
  int32_t level;
  const char *value;
  if (hrgls_STATUS_OKAY != hrgls_MessageGetLevel(message, &level)) {
    fprintf(stderr, "Could not get message level.\n");
    return;
  }
  if (hrgls_STATUS_OKAY != hrgls_MessageGetValue(message, &value)) {
    fprintf(stderr, "Could not get message value.\n");
    return;
  }
  printf("Callback message with level %d received: %s\n", level, value);
  if (hrgls_STATUS_OKAY != hrgls_MessageDestroy(message)) {
    fprintf(stderr, "Could not destroy message.\n");
    return;
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

  size_t count = 0;
  { /// Read messages using a callback handler.

    // Set a callback handler for incoming messages and then start streaming.
    status = hrgls_APISetLogMessageCallback(api, HandleMessageCallback, &count);
    if (status != hrgls_STATUS_OKAY) {
      fprintf(stderr, "Could not set callback handler: %s\n",
        hrgls_ErrorMessage(status));
      return 2;
    }
    status = hrgls_APISetLogMessageStreamingState(api, true);
    if (status != hrgls_STATUS_OKAY) {
      fprintf(stderr, "Could not set streaming state on: %s\n",
        hrgls_ErrorMessage(status));
      return 3;
    }

    // Run until we have 5 messages or we time out.
    struct timeval start;
    hrgls_APIGetCurrentSystemTime(api, &start);
    while (count < 5) {
      struct timeval now;
      hrgls_APIGetCurrentSystemTime(api, &now);
      if (now.tv_sec - start.tv_sec > 5) {
        fprintf(stderr, "Timeout waiting for callback-based messages.\n");
        return 4;
      }
    };

    // Unhook the callback handler after stopping the stream.
    
    status = hrgls_APISetLogMessageStreamingState(api, false);
    if (status != hrgls_STATUS_OKAY) {
      fprintf(stderr, "Could not set streaming state off: %s\n",
        hrgls_ErrorMessage(status));
      return 5;
    }
    status = hrgls_APISetLogMessageCallback(api, NULL, NULL);
    if (status != hrgls_STATUS_OKAY) {
      fprintf(stderr, "Could not reset callback handler: %s\n",
        hrgls_ErrorMessage(status));
      return 6;
    }
  }

  // Set a minimum threshold for the messages at warning for the
  // get-based test and ensure that we don't get any messages with
  // lower level than that.

  status = hrgls_APISetLogMessageMinimumLevel(api, hrgls_MESSAGE_MINIMUM_WARNING);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not set minimum message level: %s\n",
      hrgls_ErrorMessage(status));
    return 100;
  }

  { /// Read the message without a callback handler.

    // Start streaming
    status = hrgls_APISetLogMessageStreamingState(api, true);
    if (status != hrgls_STATUS_OKAY) {
      fprintf(stderr, "Could not set streaming state on: %s\n",
        hrgls_ErrorMessage(status));
      return 7;
    }

    count = 0;
    struct timeval start;
    hrgls_APIGetCurrentSystemTime(api, &start);
    do {
      hrgls_Message m = NULL;
      status = hrgls_APIGetNextLogMessage(api, &m);
      if (status == hrgls_STATUS_OKAY) {
        count++;
        hrgls_MessageLevel level;
        status = hrgls_MessageGetLevel(m, &level);
        if (hrgls_STATUS_OKAY != status) {
          fprintf(stderr, "Could not get message level: %s\n",
            hrgls_ErrorMessage(status));
          return 2000;
        }
        if (level < hrgls_MESSAGE_MINIMUM_WARNING) {
          fprintf(stderr, "Too low message level: %d\n", (int)level);
          return 101;
        }
        // Handle the message.
        // Print information about the message
        const char *value;
        if (hrgls_STATUS_OKAY != hrgls_MessageGetLevel(m, &level)) {
          fprintf(stderr, "Could not get message level.\n");
          return 2001;
        }
        if (hrgls_STATUS_OKAY != hrgls_MessageGetValue(m, &value)) {
          fprintf(stderr, "Could not get message value.\n");
          return 2002;
        }
        printf("Get-based message with level %d received: %s\n", level, value);
        // Destroy the message.
        status = hrgls_MessageDestroy(m);
        if (hrgls_STATUS_OKAY != status) {
          fprintf(stderr, "Could not destroy message: %s\n",
            hrgls_ErrorMessage(status));
          return 1000;
        }
      } else if (status != hrgls_STATUS_TIMEOUT) {
        if (hrgls_STATUS_OKAY != status) {
          fprintf(stderr, "Could not destroy message: %s\n",
            hrgls_ErrorMessage(status));
          return 1001;
        }
        fprintf(stderr, "Error reading message: %s\n",
          hrgls_ErrorMessage(status));
        return 8;
      }

      struct timeval now;
      hrgls_APIGetCurrentSystemTime(api, &now);
      if (now.tv_sec - start.tv_sec > 5) {
        fprintf(stderr, "Timeout waiting for get-based message.\n");
        return 9;
      }

    } while (count < 5);

    // Stop streaming
    status = hrgls_APISetLogMessageStreamingState(api, false);
    if (status != hrgls_STATUS_OKAY) {
      fprintf(stderr, "Could not set streaming state off: %s\n",
        hrgls_ErrorMessage(status));
      return 10;
    }
  }

  status = hrgls_APIDestroy(api);
  if (status != hrgls_STATUS_OKAY) {
    fprintf(stderr, "Could not destroy API: %s\n",
      hrgls_ErrorMessage(status));
    return 11;
  }

  printf("Success!\n");
  return 0;
}
