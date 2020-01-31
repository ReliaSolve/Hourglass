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

#pragma once

/**
* @file hrgls_api.hpp
* @brief Hourglass C++ API.
*
* This is the C++ wrapper for the API, which wraps the C API and handles the
* construction and destruction and copying of objects.  This file defines the interface
* both for the client applications and for the developer (who implements the classes
* and methods found herein).  For understanding the API, you should look at the
* hrgls_api_defs.hpp file.
* @author Russell Taylor.
* @date January 18, 2020.
*/

#include "hrgls_api_defs.hpp"
#include "hrgls_DataBlob_impl.hpp"
#include "hrgls_Message_impl.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <thread>
#include <mutex>

namespace hrgls {

  //-----------------------------------------------------------------------
  class StreamProperties::StreamProperties_private {
  public:
    std::shared_ptr<hrgls_StreamProperties_> m_state;
    // Keep a per-thread status to make it so that a single thread only gets
    // results from methods that it calls.
    std::map<std::thread::id, hrgls_Status> m_status;
  };

  StreamProperties::StreamProperties()
  {
    // Create our private data object
    m_private.reset(new StreamProperties_private);
    hrgls_StreamProperties ptr;
    m_private->m_status[std::this_thread::get_id()] = hrgls_StreamPropertiesCreate(&ptr);
    m_private->m_state.reset(ptr,
      [](hrgls_StreamProperties_ *obj) {hrgls_StreamPropertiesDestroy(obj); }
      );
  }

  StreamProperties::StreamProperties(const StreamProperties& copy)
  {
    m_private.reset(new StreamProperties_private());
    *m_private = *copy.m_private;
  }

  StreamProperties &StreamProperties::operator = (const StreamProperties &copy)
  {
    *m_private = *copy.m_private;
    return *this;
  }

  StreamProperties::~StreamProperties()
  {
    /* This deletion is all handled by the smart pointers.
    if (m_private) {
      if (m_private->m_state) {
        hrgls_StreamPropertiesDestroy(m_private->m_state);
      }
      delete m_private;
    }
    */
  }

  std::shared_ptr<hrgls_StreamProperties_> StreamProperties::GetRawProperties() const
  {
    if (!m_private) {
      return nullptr;
    }
    return m_private->m_state;
  }

  hrgls_Status StreamProperties::GetStatus()
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    hrgls_Status ret = m_private->m_status[std::this_thread::get_id()];
    m_private->m_status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
    return ret;
  }

  double StreamProperties::Rate()
  {
    double ret;
    if (m_private) {
      m_private->m_status[std::this_thread::get_id()] = hrgls_StreamPropertiesGetRate(m_private->m_state.get(), &ret);
    }

    return ret;
  }

  hrgls_Status StreamProperties::Rate(double val)
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    return hrgls_StreamPropertiesSetRate(m_private->m_state.get(), val);
  }


  //-----------------------------------------------------------------------
  class API::API_private {
  public:
    hrgls_API m_api = nullptr;
    // Keep a per-thread status to make it so that a single thread only gets
    // results from methods that it calls.
    std::map<std::thread::id, hrgls_Status> m_status;

    // Information needed by our C callback handler to reformat the data
    // and call the C++ handler.
    LogMessageCallback m_cppHandler = nullptr;
    void *m_cppUserData = nullptr;
    std::mutex m_cppMutex;
  };

  API::API(
    ::std::string user,
    ::std::vector<uint8_t> credentials)
  {
    // Create the private api pointer we're going to use.  Check for
    // exception when creating it, to avoid passing it up to the caller.
    try {
      m_private = new API_private();
    } catch (...) {
      m_private = nullptr;
      return;
    }

    // Create and fill in the parameters to the API creation routine
    hrgls_APICreateParams params;
    m_private->m_status[std::this_thread::get_id()] = hrgls_APICreateParametersCreate(&params);
    if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
      return;
    }
    m_private->m_status[std::this_thread::get_id()] = hrgls_APICreateParametersSetName(params, user.c_str());
    if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
      return;
    }
    m_private->m_status[std::this_thread::get_id()] = hrgls_APICreateParametersSetCredentials(params,
      credentials.data(), static_cast<uint32_t>(credentials.size()));
    if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
      return;
    }

    // Create the API object we're going to use.
    m_private->m_status[std::this_thread::get_id()] = hrgls_APICreate(&m_private->m_api, params);
    if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
      hrgls_APICreateParametersDestroy(params);
      m_private->m_api = nullptr;
      return;
    }

    // Destroy the parameter object we created above.
    m_private->m_status[std::this_thread::get_id()] = hrgls_APICreateParametersDestroy(params);
  }

  API::~API()
  {
    // Destroy any API object we created.
    if (m_private && m_private->m_api) {
      hrgls_APIDestroy(m_private->m_api);
    }
    delete m_private;
  }

  hrgls_Status API::GetStatus()
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    hrgls_Status ret = m_private->m_status[std::this_thread::get_id()];
    m_private->m_status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
    return ret;
  }

  ::std::vector<DataBlobSourceDescription> API::GetAvailableDataBlobSources() const
  {
    ::std::vector<DataBlobSourceDescription> ret;
    if (!m_private) {
      return ret;
    }

    // Find out how many DataBlobSources are in the system, which will also latch the data.
    uint32_t count;
    if (hrgls_STATUS_OKAY !=
      (m_private->m_status[std::this_thread::get_id()] = hrgls_APIGetAvailableDataBlobSourceCount(m_private->m_api, &count))) {
      return ret;
    }

    // Read each DataBlobSource's information and fill it into information to be returned.
    for (uint32_t i = 0; i < count; i++) {
      hrgls_APIDataBlobSourceInfo info;
      if (hrgls_STATUS_OKAY !=
        (m_private->m_status[std::this_thread::get_id()] = hrgls_APIGetAvailableDataBlobSourceInfo(m_private->m_api,
          i, &info))) {
        ret.clear();
        return ret;
      }

      // Get and fill in the name.
      const char *name;
      if (hrgls_STATUS_OKAY !=
        (m_private->m_status[std::this_thread::get_id()] = hrgls_APIDataBlobSourceGetName(info, &name))) {
        ret.clear();
        return ret;
      }

      DataBlobSourceDescription ri;
      ri.Name(name);

      ret.push_back(ri);
    }

    return ret;
  }

  hrgls_VERSION API::GetVersion() const
  {
    hrgls_VERSION ret = { 0, 0, 0 };
    if (!m_private) {
      return ret;
    }

    m_private->m_status[std::this_thread::get_id()] = hrgls_APIGetVersion(m_private->m_api, &ret);
    return ret;
  }

  struct timeval API::GetCurrentSystemTime() const
  {
    struct timeval ret = { 0 , 0 };
    if (!m_private) {
      return ret;
    }

    m_private->m_status[std::this_thread::get_id()] = hrgls_APIGetCurrentSystemTime(m_private->m_api, &ret);
    return ret;
  }

  uint16_t API::GetVerbosity() const
  {
    uint16_t ret = 0;
    if (!m_private) {
      return ret;
    }

    m_private->m_status[std::this_thread::get_id()] = hrgls_APIGetVerbosity(m_private->m_api, &ret);
    return ret;
  }

  hrgls_Status API::SetVerbosity(uint16_t verbosity)
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }

    return hrgls_APISetVerbosity(m_private->m_api, verbosity);
  }

  // Because we are wrapping a C+ callback handler around a C callback handler,
  // this function handles getting the data from the C function, reformatting it
  // to C++, and calling the handler that we have been asked to use.
  static void LogMessageCallbackHandler(hrgls_Message const message, void *userData)
  {
    // Convert the userData into a pointer to the private data we need.
    API::API_private *info =
      static_cast<API::API_private*>(userData);

    // Lock the mutex so that it does not get changed out from under us
    // while we get its info.
    API::LogMessageCallback cb;
    void *ud;
    {
      std::lock_guard<std::mutex> lock(info->m_cppMutex);
      cb = info->m_cppHandler;
      ud = info->m_cppUserData;
    }

    // Reformat the data into a C++ structure.
    // Call the callback handler with the data.
    if (info->m_cppHandler) {
      Message m(message);
      cb(m, ud);
    }

    // In the C API, we always have to destroy the blob that is handed
    // to us, so we do so here.  The DataBlob above will have created
    // a copy and destroyed that copy, but here we destroy the one that
    // was passed to us.
    hrgls_MessageDestroy(message);
  }

  hrgls_Status API::SetLogMessageCallback(LogMessageCallback callback,
    void *userData)
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }

    // Lock the mutex while we're changing the handler data so we don't
    // change it out from under an active callback handler.
    std::lock_guard<std::mutex> lock(m_private->m_cppMutex);

    m_private->m_cppHandler = callback;
    m_private->m_cppUserData = userData;

    // If we are supposed to be calling a callback handler, then set our callback handler
    // as the intercept.  If not, then set it to nullptr so that no callbacks are called.
    if (callback) {
      m_private->m_status[std::this_thread::get_id()] = hrgls_APISetLogMessageCallback(m_private->m_api,
        LogMessageCallbackHandler, m_private);
    } else {
      m_private->m_status[std::this_thread::get_id()] = hrgls_APISetLogMessageCallback(m_private->m_api,
        nullptr, nullptr);
    }
    return m_private->m_status[std::this_thread::get_id()];
  }

  hrgls_Status API::SetLogMessageStreamingState(bool running)
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    m_private->m_status[std::this_thread::get_id()] = hrgls_APISetLogMessageStreamingState(m_private->m_api,
      running);
    return m_private->m_status[std::this_thread::get_id()];
  }

  std::vector<Message> API::GetPendingLogMessages(size_t maxNum)
  {
    std::vector<Message> ret;
    if (!m_private) {
      return ret;
    }

    // Keep getting messages until we either have enough or get a status other
    // than OKAY.  Push each onto the vector, then destroy it.
    m_private->m_status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
    while ((ret.size() < maxNum) || (maxNum == 0)) {
      hrgls_Message m = nullptr;
      m_private->m_status[std::this_thread::get_id()] = hrgls_APIGetNextLogMessage(m_private->m_api, &m);
      if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
        hrgls_MessageDestroy(m);
        if ((m_private->m_status[std::this_thread::get_id()] == hrgls_STATUS_TIMEOUT) &&
            (ret.size() > 0)) {
          // We got at least one message before timing out, so things are
          // okay.
          m_private->m_status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
        }
        return ret;
      }
      ret.push_back(m);
      hrgls_MessageDestroy(m);
    }

    return ret;
  }

  hrgls_Status API::SetLogMessageMinimumLevel(hrgls_MessageLevel level)
  {
    if (!m_private) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    m_private->m_status[std::this_thread::get_id()] = hrgls_APISetLogMessageMinimumLevel(m_private->m_api,
      level);
    return m_private->m_status[std::this_thread::get_id()];
  }

  hrgls_API API::GetRawAPI() const
  {
    if (!m_private) {
      return nullptr;
    }
    return m_private->m_api;
  }

  namespace datablob {

    //-----------------------------------------------------------------------
    class DataBlobSource::DataBlobSource_private {
    public:
      hrgls_DataBlobSource m_stream = nullptr;
      // Keep a per-thread status to make it so that a single thread only gets
      // results from methods that it calls.
      std::map<std::thread::id, hrgls_Status> m_status;

      // Information needed by our C callback handler to reformat the data
      // and call the C++ handler.
      StreamCallback m_cppHandler = nullptr;
      void *m_cppUserData = nullptr;
      std::mutex m_cppMutex;
    };

    DataBlobSource::DataBlobSource(
      API &api,
      StreamProperties &props,
      ::std::string source)
    {
      // Create the private api pointer we're going to use.  Check for
      // exception when creating it, to avoid passing it up to the caller.
      try {
        m_private = new DataBlobSource_private();
      } catch (...) {
        m_private = nullptr;
        return;
      }

      // Create and fill in the parameters to the stream creation routine
      hrgls_DataBlobSourceCreateParams params;
      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceCreateParametersCreate(&params);
      if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
        return;
      }
      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceCreateParametersSetAPI(params, api.GetRawAPI());
      if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
        hrgls_DataBlobSourceCreateParametersDestroy(params);
        return;
      }
      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceCreateParametersSetStreamProperties(params,
        props.GetRawProperties().get());
      if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
        hrgls_DataBlobSourceCreateParametersDestroy(params);
        return;
      }
      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceCreateParametersSetName(params, source.c_str());
      if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
        hrgls_DataBlobSourceCreateParametersDestroy(params);
        return;
      }

      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceCreate(&m_private->m_stream, params);
      if (m_private->m_status[std::this_thread::get_id()] != hrgls_STATUS_OKAY) {
        hrgls_DataBlobSourceCreateParametersDestroy(params);
        return;
      }

      // Delete the structures we created along the way
      hrgls_DataBlobSourceCreateParametersDestroy(params);
    }

    DataBlobSource::~DataBlobSource()
    {
      // Destroy any API object we created.
      if (m_private && m_private->m_stream) {
        hrgls_DataBlobSourceDestroy(m_private->m_stream);
      }
      delete m_private;
    }

    hrgls_Status DataBlobSource::GetStatus()
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      }
      hrgls_Status ret = m_private->m_status[std::this_thread::get_id()];
      m_private->m_status[std::this_thread::get_id()] = hrgls_STATUS_OKAY;
      return ret;
    }

    hrgls_Status DataBlobSource::SetStreamingState(bool running)
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      }
      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceSetStreamingState(m_private->m_stream,
        running);
      return m_private->m_status[std::this_thread::get_id()];
    }

    // Because we are wrapping a C+ callback handler around a C callback handler,
    // this function handles getting the data from the C function, reformatting it
    // to C++, and calling the handler that we have been asked to use.
    static void StreamCallbackHandler(hrgls_DataBlob const blob, void *userData)
    {
      // Convert the userData into a pointer to the private data we need.
      DataBlobSource::DataBlobSource_private *info =
        static_cast<DataBlobSource::DataBlobSource_private*>(userData);

      // Lock the mutex so that it does not get changed out from under us
      // while we get its info.
      hrgls::datablob::StreamCallback cb;
      void *ud;
      {
        std::lock_guard<std::mutex> lock(info->m_cppMutex);
        cb = info->m_cppHandler;
        ud = info->m_cppUserData;
      }

      // Reformat the data into a C++ structure.
      // Call the callback handler with the data.
      if (info->m_cppHandler) {
        DataBlob f(blob);
        cb(f, ud);
      }

      // In the C API, we always have to destroy the blob that is handed
      // to us, so we do so here.  The DataBlob above will have created
      // a copy and destroyed that copy, but here we destroy the one that
      // was passed to us.
      hrgls_DataBlobDestroy(blob);
    }

    hrgls_Status DataBlobSource::SetStreamCallback(StreamCallback callback, void *userdata)
    {
      if (!m_private) {
        return hrgls_STATUS_NULL_OBJECT_POINTER;
      }

      // Lock the mutex while we're changing the handler data so we don't
      // change it out from under an active callback handler.
      std::lock_guard<std::mutex> lock(m_private->m_cppMutex);

      m_private->m_cppHandler = callback;
      m_private->m_cppUserData = userdata;

      // If we are supposed to be calling a callback handler, then set our callback handler
      // as the intercept.  If not, then set it to nullptr so that no callbacks are called.
      if (callback) {
        m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceSetStreamCallback(m_private->m_stream,
          StreamCallbackHandler, m_private);
      } else {
        m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceSetStreamCallback(m_private->m_stream,
          nullptr, nullptr);
      }
      return m_private->m_status[std::this_thread::get_id()];
    }

    DataBlob DataBlobSource::GetNextBlob(struct timeval timeout)
    {
      DataBlob emptyRet;
      if (!m_private) {
        return emptyRet;
      }
      if (!m_private->m_stream) {
        m_private->m_status[std::this_thread::get_id()] = hrgls_STATUS_NULL_OBJECT_POINTER;
        return emptyRet;
      }
      hrgls_DataBlob blob = nullptr;
      m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceGetNextBlob(m_private->m_stream, &blob, timeout);
      if (m_private->m_status[std::this_thread::get_id()] == hrgls_STATUS_TIMEOUT) {
        hrgls_DataBlobDestroy(blob);
        return emptyRet;
      }
      DataBlob ret(blob);
      hrgls_DataBlobDestroy(blob);
      return ret;
    }

    DataBlobSourceDescription DataBlobSource::GetInfo()
    {
      DataBlobSourceDescription ret;
      if (m_private && m_private->m_stream) {

        hrgls_APIDataBlobSourceInfo info;
        if (hrgls_STATUS_OKAY !=
            (m_private->m_status[std::this_thread::get_id()] = hrgls_DataBlobSourceGetInfo(m_private->m_stream, &info))) {
          return DataBlobSourceDescription();
        }

        // Get and fill in the name.
        const char *name;
        if (hrgls_STATUS_OKAY !=
          (m_private->m_status[std::this_thread::get_id()] = hrgls_APIDataBlobSourceGetName(info, &name))) {
          return DataBlobSourceDescription();
        }
        ret.Name(name);
      }
      return ret;
    }

  } // End namespace datablob

} // End namespace hrgls
