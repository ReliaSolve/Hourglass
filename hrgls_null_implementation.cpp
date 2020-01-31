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

// Implements the C++ interface that is described in the hrgls_api_defs.hpp header file.
// This enables it to be linked into the library with hrgls_internal_wrap.cpp to form a
// complete implementation.  All of the methods here return a status of OKAY, but they
// do not do anything or keep track of any state.  They also do not check their parameters.

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "hrgls_api_defs.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <set>
#include <thread>
#include <mutex>
#include <list>
#include <map>
#include <fstream>
#include <atomic>
#include <string.h>

static std::vector<char> GetFile(std::string fileName)
{
  std::vector<char> ret;
  FILE *f = fopen(fileName.c_str(), "rb");
  if (!f) { return ret; }

  // Read the DataBlob into memory after finding its size.
  fseek(f, 0L, SEEK_END);
  long data_size = ftell(f);
  fseek(f, 0L, SEEK_SET);
  ret.resize(data_size);
  fread(ret.data(), sizeof(ret.data()[0]), data_size, f);
  fclose(f);
  return ret;
}

static void myDelete(void * /*userdata*/, const uint8_t *data)
{
  // delete the data array that was allocated when we set it below.
  delete[] data;
}

namespace hrgls {

  class API::API_private {
  public:
    // Keeps track of current verbosity level, defaults to 0 (no messages)
    uint16_t verbosity = 0;

    // Keep a per-thread status to make it so that a single thread only gets
    // results from methods that it calls.
    std::map<std::thread::id, hrgls_Status> status;
    ::std::vector<DataBlobSourceDescription> rends;

    /// Thread that generates DataBlobs and either sends them to the callback
    /// handler or stores them locally to be gotten one by one.
    std::thread myThread;
    volatile bool quitThread = false;   ///< Time to quit the thread.

    // Are we running?  If so, generate messages asynchronously and put into
    // list.
    bool messageStreaming = false;

    /// Callback handler registered with us, along with its userdata and a mutex
    /// that is used to ensure that we read and update both data values atomically.
    std::mutex callbackMutex;
    LogMessageCallback callbackHandler = nullptr;
    void *callbackUserData = nullptr;

    /// List of messages that have come in with no callback handler to deal
    /// with them.  They are retrieved by GetNextLogMessage()
    std::mutex storedMessagesMutex;
    std::list<Message> storedMessages;
    hrgls_MessageLevel minLevel = hrgls_MESSAGE_MINIMUM_INFO;
  };

  static void LogMessageThread(API::API_private *info)
  {
      if (!info) { return; }

      // Keeps track of how long since we sent a message.
      auto lastMessage = std::chrono::system_clock::now();

      hrgls_MessageLevel level = hrgls_MESSAGE_MINIMUM_INFO;

      do {
          // See if we are streaming.
          if (info->messageStreaming) {
              // See if it is time to create a new message.
              auto now = std::chrono::system_clock::now();
              std::chrono::duration<float> dt = now - lastMessage;
              double interval = 0.1;
              if (dt.count() > interval) {

                  lastMessage = now;

                  // Create a message with cycling level.
                  ::std::chrono::microseconds uSecSinceEpoch =
                      ::std::chrono::duration_cast<::std::chrono::microseconds>(now.time_since_epoch());
                  struct timeval nowTV;
                  nowTV.tv_sec = static_cast<uint32_t>(uSecSinceEpoch.count() / 1000000);
                  nowTV.tv_usec = static_cast<uint32_t>(uSecSinceEpoch.count() - nowTV.tv_sec * 1000000);
                  Message m("value of the message", nowTV, level);

                  // Update the level, cycling between those available.
                  switch (level) {
                  case hrgls_MESSAGE_MINIMUM_INFO:
                      level = hrgls_MESSAGE_MINIMUM_WARNING;
                      break;
                  case hrgls_MESSAGE_MINIMUM_WARNING:
                      level = hrgls_MESSAGE_MINIMUM_ERROR;
                      break;
                  case hrgls_MESSAGE_MINIMUM_ERROR:
                      level = hrgls_MESSAGE_MINIMUM_CRITICAL_ERROR;
                      break;
                  default:
                      level = hrgls_MESSAGE_MINIMUM_INFO;
                  }

                  // If we're above the threshold, insert the message.
                  if (m.Level() >= info->minLevel) {
                      // Need to guard the access to the callback handler and userdata with a
                      // mutex so that we don't get half of the information due to a race with the
                      // main thread.
                      hrgls::API::LogMessageCallback callbackHandler;
                      void *callbackUserData;
                      {
                          std::lock_guard<std::mutex> lock(info->callbackMutex);
                          callbackHandler = info->callbackHandler;
                          callbackUserData = info->callbackUserData;
                      }

                      // If we have a callback handler, call it.  If not, queue the message
                      // for later delivery.
                      if (callbackHandler) {
                          callbackHandler(m, callbackUserData);
                      } else {
                          // Store message onto the vector for GetNextLogMessage.
                          std::lock_guard<std::mutex> lock(info->storedMessagesMutex);
                          info->storedMessages.push_back(m);
                      }
                  }

              } // if time for a new blob

          } // if running

          std::this_thread::sleep_for(std::chrono::milliseconds(1));
      } while (!info->quitThread);
  }

  API::API(
        ::std::string user,
        ::std::vector<uint8_t> credentials)
  {
    //------------------------------------------------------------------------------
    // Construct the data we'll need to enable a test program to try out all of our
    // features.
    m_private = new API_private;
    m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;

    // Construct two DataBlobSource descriptions.
    DataBlobSourceDescription rend;
    rend.Name("/hrgls/null/DataBlobSource/1");
    m_private->rends.push_back(rend);
    rend.Name("/hrgls/null/DataBlobSource/2");
    m_private->rends.push_back(rend);

    /// Start our thread that polls the remote interface and receives messages,
    /// passing it a pointer to our private stream information.
    m_private->myThread = std::thread(LogMessageThread, m_private);
    return;
  }

  API::~API()
  {
    // Stop my thread.
    if (m_private) {
        if (m_private->verbosity > 200) {
          std::cout << "API::~API(): Destroying API:" << std::endl;
        }
        m_private->quitThread = true;
        m_private->myThread.join();
    }

    delete m_private;
  }

  hrgls_Status API::GetStatus()
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    } else {
      hrgls_Status ret = m_private->status[std::this_thread::get_id()];
      m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
      return ret;
    }
  }

  ::std::vector<DataBlobSourceDescription> API::GetAvailableDataBlobSources() const
  {
    if (!m_private) {
      ::std::vector<DataBlobSourceDescription> ret;
      return ret;
    } else {
      return m_private->rends;
    }
  }

  hrgls_VERSION API::GetVersion() const
  {
    /// @todo Update whenever the version changes.
    hrgls_VERSION ret = { 0, 1, 0 };
    return ret;
  }

  struct timeval API::GetCurrentSystemTime() const
  {
    struct timeval ret = {};
    if (m_private) {
      ::std::chrono::time_point<::std::chrono::system_clock> myTime =
        ::std::chrono::system_clock::now();
      ::std::chrono::microseconds uSecSinceEpoch =
        ::std::chrono::duration_cast<::std::chrono::microseconds>(myTime.time_since_epoch());
      ret.tv_sec = static_cast<uint32_t>(uSecSinceEpoch.count() / 1000000);
      ret.tv_usec = static_cast<uint32_t>(uSecSinceEpoch.count() - ret.tv_sec * 1000000);
    }
    return ret;
  }

  uint16_t API::GetVerbosity() const
  {
    uint16_t ret = 0;
    if (m_private) {
      ret = m_private->verbosity;
    }
    return ret;
  }

  hrgls_Status API::SetVerbosity(uint16_t verbosity)
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    m_private->verbosity = verbosity;
    if (m_private->verbosity > 200) {
      std::cout << "API::SetVerbosity(): New verbosity: " << verbosity << std::endl;
    }
    return hrgls_STATUS_OKAY;
  }

  hrgls_Status API::SetLogMessageCallback(LogMessageCallback callback,
      void *userdata)
  {
      if (!m_private) {
          return hrgls_STATUS_NULL_OBJECT_POINTER;
      }

      // Lock the thread mutices for callback and streaming while we change
      // the state.
      {
        std::lock_guard<std::mutex> lock(m_private->callbackMutex);

        // Set the streaming callback handler.
        m_private->callbackHandler = callback;
        m_private->callbackUserData = userdata;
      }

      // Flush all stored messages.
      std::lock_guard<std::mutex> lock2(m_private->storedMessagesMutex);
      m_private->storedMessages.clear();

      return hrgls_STATUS_OKAY;
  }

  hrgls_Status API::SetLogMessageStreamingState(bool running)
  {
      if (!m_private) {
          return hrgls_STATUS_NULL_OBJECT_POINTER;
      }
      m_private->messageStreaming = running;
      return hrgls_STATUS_OKAY;
  }

  std::vector<Message> API::GetPendingLogMessages(size_t maxNum)
  {
      std::vector<Message> ret;
      if (!m_private) {
          return ret;
      }
      m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;

      std::lock_guard<std::mutex> lock2(m_private->storedMessagesMutex);
      while ((m_private->storedMessages.size() > 0) &&
             ((maxNum == 0) || (ret.size() < maxNum)) ) {
        ret.push_back(m_private->storedMessages.front());
        m_private->storedMessages.pop_front();
      }
      if (ret.size() == 0) {
        m_private->status[std::this_thread::get_id()] = hrgls_STATUS_TIMEOUT;
      }
      return ret;
  }

  hrgls_Status API::SetLogMessageMinimumLevel(hrgls_MessageLevel level)
  {
      if (!m_private) {
          return hrgls_STATUS_NULL_OBJECT_POINTER;
      }
      m_private->minLevel = level;
      return m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
  }

  namespace datablob {

    //------------------------------------------------------------------------------
    static std::atomic<size_t> numCreatedDataBlobSources(0);
    class DataBlobSource::DataBlobSource_private {
    public:
      API *api = nullptr;
      std::string streamName;
      StreamProperties properties;

      // Keep a per-thread status to make it so that a single thread only gets
      // results from methods that it calls.
      std::map<std::thread::id, hrgls_Status> status;
      std::string name;
      bool running = false;

      /// Thread that listens for incoming DataBlobs and either sends them to the callback
      /// handler or stores them locally to be gotten one by one.
      std::thread myThread;
      volatile bool quitThread = false;   ///< Time to quit the thread.

      /// Callback handler registered with us, along with its userdata and a mutex
      /// that is used to ensure that we read and update both data values atomically.
      std::mutex callbackMutex;
      StreamCallback callbackHandler = nullptr;
      void *callbackUserData = nullptr;

      /// List of blobs that have come in with no callback handler to deal
      /// with them.  They are retrieved by GetNextBlob()
      std::mutex storedBlobsMutex;
      std::list< DataBlob> storedBlobs;
    };

    static void DataBlobSourceThread(DataBlobSource::DataBlobSource_private *info)
    {
      if (!info) { return; }

      // Keeps track of how long since we sent a blob
      auto lastBlob = std::chrono::system_clock::now();

      // Make the data we're going to send.
      std::vector<char> blobToSend;
      for (size_t i = 0; i < 256; i++) {
        blobToSend.push_back(i % 256);
      }

      do {
        // See if we are streaming.
        if (info->running) {
          // See if it is time to create a new blob.
          auto now = std::chrono::system_clock::now();
          std::chrono::duration<float> dt = now - lastBlob;
          double interval = 1.0 / info->properties.Rate();
          timeval myTime = info->api->GetCurrentSystemTime();
          if (dt.count() > interval) {

            // Create a blob
            hrgls_DataBlob blob;
            hrgls_DataBlobCreate(&blob);
            hrgls_DataBlobSetTime(blob, myTime);

            // Copy the DataBlob data, make a copy, and set the deleter function for it.
            uint8_t *data = new uint8_t[blobToSend.size()];
            memcpy(data, blobToSend.data(), blobToSend.size());
            hrgls_DataBlobSetData(blob, data, static_cast<uint32_t>(blobToSend.size()),
              myDelete, nullptr);

            // Need to guard the access to the callback handler and userdata with a
            // mutex so that we don't get half of the information due to a race with the
            // main thread.
            StreamCallback callbackHandler;
            void *callbackUserData;
            {
              std::lock_guard<std::mutex> lock(info->callbackMutex);
              callbackHandler = info->callbackHandler;
              callbackUserData = info->callbackUserData;
            }

            // If we have a callback handler, call it.  If not, queue the DataBlob
            // for later delivery.
            DataBlob blobpp(blob);
            hrgls_DataBlobDestroy(blob);
            if (callbackHandler) {
              callbackHandler(blobpp, callbackUserData);
            } else {
              // Store a blob with a copy of the data onto the front of the queue for
              // GetNextBlob.
              std::lock_guard<std::mutex> lock(info->storedBlobsMutex);
              info->storedBlobs.push_back(blobpp);
            }
          } // if time for a new blob

        } // if running

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      } while (!info->quitThread);
    }

    DataBlobSource::DataBlobSource(
      API &api,
      StreamProperties &props,
      ::std::string DataBlobSource
    )
    {
      //------------------------------------------------------------------------------
      // Construct the data we'll need to enable a test program to try out all of our
      // features.
      m_private = new DataBlobSource_private;
      m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;

      m_private->streamName = "/hrgls/null/DataBlobSource/" +
        std::to_string(numCreatedDataBlobSources++);
      // If they ask for an empty-named DataBlobSource, return the first one.  Otherwise,
      // make sure we have the one they asked for and return it.
      std::vector<DataBlobSourceDescription> rends = api.GetAvailableDataBlobSources();
      size_t which = 0;
      if (DataBlobSource.size() > 0) {
        bool found = false;
        for (size_t i = 0; i < rends.size(); i++) {
          if (rends[i].Name() == DataBlobSource) {
            found = true;
            which = i;
            break;
          }
        }
        if (!found) {
          m_private->status[std::this_thread::get_id()] = hrgls_STATUS_BAD_PARAMETER;
          return;
        }
      }

      /// Squirrel away all of the information we need in our private store so
      /// that we can implement our methods.
      m_private->name = rends[which].Name();
      m_private->api = &api;
      m_private->properties = props;

      /// Start our thread that polls the remote interface and receives blobs,
      /// passing it a pointer to our private stream information.
      m_private->myThread = std::thread(DataBlobSourceThread, m_private);
    }

    DataBlobSource::~DataBlobSource()
    {
      // Stop my thread.
      if (m_private) {
        m_private->quitThread = true;
        m_private->myThread.join();
      }
      delete m_private;
    }

    hrgls_Status DataBlobSource::GetStatus()
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      } else {
        hrgls_Status ret = m_private->status[std::this_thread::get_id()];
        m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
        return ret;
      }
    }

    hrgls_Status DataBlobSource::SetStreamingState(bool running)
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      }
      m_private->running = running;
      return hrgls_STATUS_OKAY;
    }

    hrgls_Status DataBlobSource::SetStreamCallback(StreamCallback callback, void *userdata)
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      }

      // Lock the thread mutices for callback and streaming while we change
      // the state.
      std::lock_guard<std::mutex> lock(m_private->callbackMutex);
      std::lock_guard<std::mutex> lock2(m_private->storedBlobsMutex);

      // Set the streaming callback handler.
      m_private->callbackHandler = callback;
      m_private->callbackUserData = userdata;

      // Flush all stored blobs.
      m_private->storedBlobs.clear();

      return hrgls_STATUS_OKAY;
    }

    DataBlob DataBlobSource::GetNextBlob(struct timeval timeout)
    {
      DataBlob ret;
      if (!m_private) {
        return ret;
      }

      // Keep checking for a new blob until we either find one or time out.
      bool empty;
      double timeoutSeconds = timeout.tv_sec + timeout.tv_usec / 1e6;
      std::chrono::duration<float> dt;
      auto start = std::chrono::system_clock::now();
      do {
        std::lock_guard<std::mutex> lock(m_private->storedBlobsMutex);
        empty = m_private->storedBlobs.empty();
        auto now = std::chrono::system_clock::now();
        dt = now - start;
      } while (empty && (dt.count() < timeoutSeconds));

      if (!empty) {
        std::lock_guard<std::mutex> lock(m_private->storedBlobsMutex);
        ret = m_private->storedBlobs.front();
        m_private->storedBlobs.pop_front();
        m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
      } else {
        m_private->status[std::this_thread::get_id()] = hrgls_STATUS_TIMEOUT;
      }

      return ret;
    }

    DataBlobSourceDescription DataBlobSource::GetInfo()
    {
      DataBlobSourceDescription ret;
      if (!m_private) {
        m_private->status[std::this_thread::get_id()] = hrgls_STATUS_NULL_OBJECT_POINTER;
        return ret;
      }
      ret.Name(m_private->streamName);
      m_private->status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
      return ret;
    }

  } // End namespace render

} // End namespace hrgls
