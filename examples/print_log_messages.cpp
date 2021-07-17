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

void HandleMessageCallback(hrgls::Message &message, void *userData)
{
  // Increment to keep track of how many messages arrived.
  size_t *count = static_cast<size_t*>(userData);
  (*count)++;

  // Print the message
  std::cout << "Callback message with level " << message.Level()
    << " received: " << message.Value() << std::endl;
}

int main(int argc, const char *argv[])
{
  // Get a base API object, specifying default parameters.
  hrgls::API api;
  hrgls_Status status = api.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not Open API: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 1;
  }

  size_t count = 0;
  { /// Read messages using a callback handler.

    // Set a callback handler for incoming messages and then start streaming.
    api.SetLogMessageCallback(HandleMessageCallback, &count);
    status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set callback handler: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 2;
    }
    api.SetLogMessageStreamingState(true);
    status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state on: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 3;
    }

    // Run until we have 5 messages or we time out.
    struct timeval start = api.GetCurrentSystemTime();
    while (count < 5) {
      struct timeval now = api.GetCurrentSystemTime();
      if (now.tv_sec - start.tv_sec > 5) {
        std::cerr << "Timeout waiting for callback-based messages." << std::endl;
        return 4;
      }
    };

    // Unhook the callback handler after stopping the stream.
    api.SetLogMessageStreamingState(false);
    status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state off: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 5;
    }
    api.SetLogMessageCallback(nullptr, nullptr);
    status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not reset callback handler: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 6;
    }
  }

  // Set a minimum threshold for the messages at warning for the
  // get-based test and ensure that we don't get any messages with
  // lower level than that.

  api.SetLogMessageMinimumLevel(hrgls_MESSAGE_MINIMUM_WARNING);
  status = api.GetStatus();
  if (status != hrgls_STATUS_OKAY) {
    std::cerr << "Could not set minimum message level: "
      << hrgls_ErrorMessage(status) << std::endl;
    return 100;
  }

  { /// Read the message without a callback handler.

    // Start streaming
    api.SetLogMessageStreamingState(true);
    status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state on: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 7;
    }

    count = 0;
    struct timeval start = api.GetCurrentSystemTime();
    do {
      std::vector<hrgls::Message> messages = api.GetPendingLogMessages();
      hrgls_Status status = api.GetStatus();
      if (hrgls_STATUS_OKAY == status) {
        count += messages.size();
        // Make sure we didn't get any messages below the minimum level.
        for (auto message : messages) {
          if (message.Level() < hrgls_MESSAGE_MINIMUM_WARNING) {
            std::cerr << "Message received with too-low level: "
              << message.Level() << std::endl;
            return 101;
          }
          // Print the message
          std::cout << "Get-based message with level " << message.Level()
            << " received: " << message.Value() << std::endl;
        }
      } else if (status != hrgls_STATUS_TIMEOUT) {
        std::cerr << "Error reading messages: " << hrgls_ErrorMessage(status) << std::endl;
        return 8;
      }

      struct timeval now = api.GetCurrentSystemTime();
      if (now.tv_sec - start.tv_sec > 5) {
        std::cerr << "Timeout waiting for get-based messages." << std::endl;
        return 9;
      }

    } while (count < 5);

    // Stop streaming
    api.SetLogMessageStreamingState(false);
    status = api.GetStatus();
    if (status != hrgls_STATUS_OKAY) {
      std::cerr << "Could not set streaming state off: "
        << hrgls_ErrorMessage(status) << std::endl;
      return 10;
    }
  }

  return 0;
}
