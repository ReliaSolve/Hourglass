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

// Include the C++ class definitions, which we will use here to implement the
// C functions.
#include "hrgls_api_defs.hpp"
#include "hrgls_DataBlob_impl.hpp"
#include "hrgls_Message_impl.hpp"
#include <string.h>
#include <iostream>
#include <math.h>
#include <map>
#include <thread>

//----------------------------------------------------------------------------
// Static callback handler function that takes in a C++ callback for a message
// and a pointer to the info needed to turn it into a C callback.
// The function is declared here but defined later, once the required private
// info has been defined.
static void LogMessageCallback(hrgls::Message &message, void *userData);

//----------------------------------------------------------------------------
// Static callback handler function that takes in a C++ callback for a stream
// and a pointer to the info needed to turn it into a C callback.
// The function is declared here but defined later, once the required private
// info has been defined.
static void DataBlobSourceCallback(hrgls::datablob::DataBlob &blob, void *userData);

//----------------------------------------------------------------------------
// The externally-linkable, "C" interface, parts of the wrapper.

extern "C" {

  const struct timeval hrgls_Interval_UNDEFINED_END = { 2147483647 , 2147483647 };

  //----------------------------------------------------------------------------
  /// Error handling.

  const char *hrgls_ErrorMessage(hrgls_Status status)
  {
    switch (status) {
    case hrgls_STATUS_OKAY:
      return "No error";

    case hrgls_STATUS_TIMEOUT:
      return "Timeout";

    case hrgls_STATUS_BAD_PARAMETER:
      return "Bad parameter";
    case hrgls_STATUS_OUT_OF_MEMORY:
      return "Out of memory";
    case hrgls_STATUS_NOT_IMPLEMENTED:
      return "Feature not yet implemented";
    case hrgls_STATUS_DELETE_OF_NULL_POINTER:
      return "Deletion of NULL pointer";
    case hrgls_STATUS_DELETION_FAILED:
      return "Pointer deletion failed";
    case hrgls_STATUS_NULL_OBJECT_POINTER:
      return "Object method called with NULL object pointer";
    case hrgls_STATUS_INTERNAL_EXCEPTION:
      return "Exception thrown inside implementation";

    default:
      return "Unrecognized error code";
    }
  }

  //----------------------------------------------------------------------------
  // Top-level API functions

  //----------------------------------------------------------------------------
  /// APICreateParams structures and methods.

  struct hrgls_APICreateParams_ {
    ::std::string name;
    ::std::vector<uint8_t> credentials;
  };

  hrgls_Status hrgls_APICreateParametersCreate(hrgls_APICreateParams *returnParams)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    hrgls_APICreateParams ret;
    try {
      ret = new hrgls_APICreateParams_;
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }
    *returnParams = ret;
    return s;
  }

  hrgls_Status hrgls_APICreateParametersDestroy(hrgls_APICreateParams params)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_DELETE_OF_NULL_POINTER;
    }
    try {
      delete params;
    } catch (...) {
      s = hrgls_STATUS_DELETION_FAILED;
    }
    return s;
  }

  hrgls_Status hrgls_APICreateParametersGetName(hrgls_APICreateParams params,
    const char **returnName)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnName) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    *returnName = params->name.c_str();

    return s;
  }

  hrgls_Status hrgls_APICreateParametersSetName(hrgls_APICreateParams params,
    const char *name)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!name) {
      params->name = "";
    } else {
      params->name = name;
    }
    return s;
  }

  hrgls_Status hrgls_APICreateParametersGetCredentials(hrgls_APICreateParams params,
    const uint8_t **returnCredentials, uint32_t *returnSize)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnCredentials) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    if (!returnSize) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    *returnCredentials = params->credentials.data();
    *returnSize = static_cast<uint32_t>(params->credentials.size());

    return s;
  }

  hrgls_Status hrgls_APICreateParametersSetCredentials(hrgls_APICreateParams params,
    const uint8_t *credentials, uint32_t size)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!size) {
      params->credentials.clear();
    } else {
      if (!credentials) {
        return hrgls_STATUS_BAD_PARAMETER;
      } else {
        // We have a nonzero size and a non-NULL pointer, so copy the data.
        params->credentials.assign(credentials, credentials + size);
      }
    }
    return s;
  }



  //----------------------------------------------------------------------------
  /// hrgls_APIDataBlobSourceInfo structures and methods.

  struct hrgls_APIDataBlobSourceInfo_ {
    std::string name;
  };

  //----------------------------------------------------------------------------
  /// API structures and methods.

  struct hrgls_API_ {
    hrgls::API *api = nullptr;

    ::std::map< ::std::thread::id, ::std::vector<struct hrgls_APIDataBlobSourceInfo_> >
      latchedDataBlobSourceInfos;

    /// C callback function that was registered
    hrgls_LogMessageCallback CHandler = nullptr;

    /// UserData parameter that was given to us when the callback function was registered.
    void *CUserData = nullptr;
  };

  hrgls_Status hrgls_APICreate(hrgls_API *returnAPI, hrgls_APICreateParams params)
  {
    hrgls_Status s;

    // Check the parameters.
    if (!returnAPI || !params) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    // Pull the parameters out so that we can pass them to the constructor.
    const char *retString;
    if (hrgls_STATUS_OKAY != (s = hrgls_APICreateParametersGetName(params, &retString))) {
      *returnAPI = nullptr;
      return s;
    }
    std::string name = retString;

    uint32_t size;
    const uint8_t *retCreds;
    if (hrgls_STATUS_OKAY != (s = hrgls_APICreateParametersGetCredentials(
        params, &retCreds, &size))) {
      *returnAPI = nullptr;
      return s;
    }
    std::vector<uint8_t> credentials;
    credentials.assign(retCreds, retCreds + size);

    // Attempt to construct the object.
    s = hrgls_STATUS_OKAY;
    hrgls_API ret;
    try {
      ret = new hrgls_API_;
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }
    try {
      ret->api = new hrgls::API(name, credentials);
    } catch (...) {
      s = hrgls_STATUS_INTERNAL_EXCEPTION;
      ret->api = nullptr;
      delete ret;
      ret = nullptr;
    }
    *returnAPI = ret;
    if (ret) {
      return ret->api->GetStatus();
    } else {
      return s;
    }
  }

  hrgls_Status hrgls_APIDestroy(hrgls_API api)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      s = hrgls_STATUS_DELETE_OF_NULL_POINTER;
    } else {
      if (!api->api) {
        s = hrgls_STATUS_DELETE_OF_NULL_POINTER;
      } else {

        try {
          delete api->api;
        } catch (...) {
          s = hrgls_STATUS_DELETION_FAILED;
        }
        api->api = nullptr;
      }
      try {
        delete api;
      } catch (...) {
        s = hrgls_STATUS_DELETION_FAILED;
      }
    }
    return s;
  }

  hrgls_Status hrgls_APIGetAvailableDataBlobSourceCount(hrgls_API api,
    uint32_t *returnCount)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnCount) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    // Try to get the object info.
    ::std::vector<hrgls::DataBlobSourceDescription> objs;
    try {
      objs = api->api->GetAvailableDataBlobSources();
      if (hrgls_STATUS_OKAY != (s = api->api->GetStatus())) {
        return s;
      }
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }

    // Store the  info in our latched location, allocating space as needed.
    // This will enable us to read back from it in a consistent state using the
    // info-reading function.  We delete any old information while we're doing this.
    try {
      api->latchedDataBlobSourceInfos[::std::this_thread::get_id()].clear();
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
    try {
      for (size_t i = 0; i < objs.size(); i++) {
        struct hrgls_APIDataBlobSourceInfo_ info;
        info.name = objs[i].Name().c_str();

        api->latchedDataBlobSourceInfos[::std::this_thread::get_id()].push_back(info);
      }
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
    }

    // Return the count.
    *returnCount = static_cast<uint32_t>(api->latchedDataBlobSourceInfos[::std::this_thread::get_id()].size());
    return s;
  }

  hrgls_Status hrgls_APIGetAvailableDataBlobSourceInfo(hrgls_API api, uint32_t which,
    hrgls_APIDataBlobSourceInfo *returnInfo)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnInfo) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    if (which >= api->latchedDataBlobSourceInfos[::std::this_thread::get_id()].size()) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    *returnInfo = &api->latchedDataBlobSourceInfos[::std::this_thread::get_id()][which];
    return s;
  }

  hrgls_Status hrgls_APIGetVersion(hrgls_API api, hrgls_VERSION *returnVersion)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnVersion) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    try {
      *returnVersion = api->api->GetVersion();
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
    return api->api->GetStatus();
  }

  hrgls_Status hrgls_APIGetCurrentSystemTime(hrgls_API api, struct timeval *returnTime)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnTime) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    try {
      *returnTime = api->api->GetCurrentSystemTime();
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
    return api->api->GetStatus();
  }

  hrgls_Status hrgls_APIGetVerbosity(hrgls_API api, uint16_t *returnVerbosity)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnVerbosity) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    try {
      *returnVerbosity = api->api->GetVerbosity();
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
    return api->api->GetStatus();
  }

  hrgls_Status hrgls_APISetVerbosity(hrgls_API api, uint16_t verbosity)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }

    try {
      api->api->SetVerbosity(verbosity);
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
    return api->api->GetStatus();
  }

  HRGLS_EXPORT hrgls_Status hrgls_APISetLogMessageStreamingState(hrgls_API api, bool running)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    try {
      return api->api->SetLogMessageStreamingState(running);
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  HRGLS_EXPORT hrgls_Status hrgls_APISetLogMessageCallback(hrgls_API api,
    hrgls_LogMessageCallback handler, void *userData)
  {
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    api->CHandler = handler;
    api->CUserData = userData;
    // If we are supposed to be calling a callback handler, then set our callback handler
    // as the intercept.  If not, then set it to nullptr so that no callbacks are called.
    try {
      if (!handler) {
        return api->api->SetLogMessageCallback(nullptr, nullptr);
      }
      return api->api->SetLogMessageCallback(LogMessageCallback, api);
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  HRGLS_EXPORT hrgls_Status hrgls_APIGetNextLogMessage(hrgls_API api, hrgls_Message *message)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!message) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    // Get at most a single message.
    try {
      ::std::vector<::hrgls::Message> ret = api->api->GetPendingLogMessages(1);
      if (ret.size() > 0) {
        // Copy the first (only) message into the pointed-to return location.
        if (hrgls_STATUS_OKAY != (s = hrgls_MessageCopy(message, ret.front().RawMessage()))) {
          return s;
        }
      }
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }

    return api->api->GetStatus();
  }

  HRGLS_EXPORT hrgls_Status hrgls_APISetLogMessageMinimumLevel(hrgls_API api, hrgls_MessageLevel level)
  {
    if (!api) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    try {
      return api->api->SetLogMessageMinimumLevel(level);
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  //----------------------------------------------------------------------------
  /// hrgls_StreamProperties structures and methods.

  struct hrgls_StreamProperties_ {
    hrgls::StreamProperties *props = nullptr;
  };

  hrgls_Status hrgls_StreamPropertiesCreate(hrgls_StreamProperties *returnProp)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (returnProp == nullptr) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    hrgls_StreamProperties ret;
    try {
      // Construct the object
      ret = new hrgls_StreamProperties_;
      ret->props = new hrgls::StreamProperties();
      s = ret->props->GetStatus();
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }

    *returnProp = ret;
    return s;
  }

  hrgls_Status hrgls_StreamPropertiesDestroy(hrgls_StreamProperties prop)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!prop || !prop->props) {
      return hrgls_STATUS_DELETE_OF_NULL_POINTER;
    } try {
      delete prop->props;
      delete prop;
    } catch (...) {
      s = hrgls_STATUS_DELETION_FAILED;
    }
    return s;
  }

  hrgls_Status hrgls_StreamPropertiesGetRate(hrgls_StreamProperties prop, double *val)
  {
    if (!prop || !prop->props) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }

    try {
      *val = prop->props->Rate();
      return prop->props->GetStatus();
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  hrgls_Status hrgls_StreamPropertiesSetRate(hrgls_StreamProperties prop, double val)
  {
    if (!prop || !prop->props) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    try {
      return prop->props->Rate(val);
    } catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  //----------------------------------------------------------------------------
  /// hrgls_Message structures and methods.

  struct hrgls_Message_ {
	  std::string value;
	  struct timeval timeStamp;
	  hrgls_MessageLevel level;
  };

  HRGLS_EXPORT hrgls_Status hrgls_MessageCreate(hrgls_Message *returnMessage)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  hrgls_Message ret;
	  try {
		  ret = new hrgls_Message_;
	  } catch (...) {
		  s = hrgls_STATUS_OUT_OF_MEMORY;
		  ret = nullptr;
	  }
	  *returnMessage = ret;
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageCopy(hrgls_Message *returnMessage, hrgls_Message MessageToCopy)
  {
          if (!MessageToCopy) {
            return hrgls_STATUS_BAD_PARAMETER;
          }

	  hrgls_Status s = hrgls_STATUS_OKAY;
	  hrgls_Message ret;
	  try {
		  ret = new hrgls_Message_;
	  } catch (...) {
		  s = hrgls_STATUS_OUT_OF_MEMORY;
		  ret = nullptr;
	  }

	  *ret = *MessageToCopy;

	  *returnMessage = ret;
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageDestroy(hrgls_Message obj)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_DELETE_OF_NULL_POINTER;
	  }
	  try {
		  delete obj;
	  } catch (...) {
		  s = hrgls_STATUS_DELETION_FAILED;
	  }
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageGetValue(hrgls_Message obj, const char **val)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_NULL_OBJECT_POINTER;
	  }
	  if (!val) {
		  return hrgls_STATUS_BAD_PARAMETER;
	  }

	  *val = obj->value.c_str();
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageSetValue(hrgls_Message obj, const char *val)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_NULL_OBJECT_POINTER;
	  }
	  if (!val) {
		  return hrgls_STATUS_BAD_PARAMETER;
	  }
	  obj->value = val;
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageGetTimeStamp(hrgls_Message obj, struct timeval *val)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_NULL_OBJECT_POINTER;
	  }
	  if (!val) {
		  return hrgls_STATUS_BAD_PARAMETER;
	  }

	  *val = obj->timeStamp;
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageSetTimeStamp(hrgls_Message obj, struct timeval val)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_NULL_OBJECT_POINTER;
	  }
	  obj->timeStamp = val;
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageGetLevel(hrgls_Message obj, hrgls_MessageLevel *val)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_NULL_OBJECT_POINTER;
	  }
	  if (!val) {
		  return hrgls_STATUS_BAD_PARAMETER;
	  }

	  *val = obj->level;
	  return s;
  }

  HRGLS_EXPORT hrgls_Status hrgls_MessageSetLevel(hrgls_Message obj, hrgls_MessageLevel val)
  {
	  hrgls_Status s = hrgls_STATUS_OKAY;
	  if (!obj) {
		  return hrgls_STATUS_NULL_OBJECT_POINTER;
	  }
	  obj->level = val;
	  return s;
  }

  //----------------------------------------------------------------------------
  /// hrgls_DataBlob structures and methods.

  struct hrgls_DataBlob_ {
    const uint8_t *data = nullptr;
    uint32_t size = 0;
    struct timeval time = { 0, 0 };
    hrgls_DeletionFunction deleteFunction = nullptr;
    void *deleteFunctionUserData = nullptr;
  };

  hrgls_Status hrgls_DataBlobCreate(hrgls_DataBlob *returnBlob)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    hrgls_DataBlob ret;
    try {
      ret = new hrgls_DataBlob_;
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }
    *returnBlob = ret;
    return s;
  }

  hrgls_Status hrgls_DataBlobCopy(hrgls_DataBlob* returnBlob, hrgls_DataBlob blobToCopy)
  {
    if (!blobToCopy) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    hrgls_Status s = hrgls_STATUS_OKAY;

    // Create a return blob to store the copy into.
    hrgls_DataBlob ret;
    try {
      ret = new hrgls_DataBlob_;
    }
    catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }

    // Copy the fields from the original blob.
    ret->size = blobToCopy->size;
    ret->data = blobToCopy->data;
    ret->deleteFunction = blobToCopy->deleteFunction;
    ret->deleteFunctionUserData = blobToCopy->deleteFunctionUserData;

    *returnBlob = ret;
    return s;
  }

  hrgls_Status hrgls_DataBlobDestroy(hrgls_DataBlob blob)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!blob) {
      return hrgls_STATUS_DELETE_OF_NULL_POINTER;
    }
    try {
      delete blob;
    }
    catch (...) {
      s = hrgls_STATUS_DELETION_FAILED;
    }
    return s;
  }

  hrgls_Status hrgls_DataBlobGetTime(hrgls_DataBlob blob, struct timeval *val)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!blob) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    *val = blob->time;
    return s;
  }

  hrgls_Status hrgls_DataBlobSetTime(hrgls_DataBlob im, struct timeval val)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!im) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    im->time = val;
    return s;
  }

  hrgls_Status hrgls_DataBlobGetData(hrgls_DataBlob blob, const uint8_t** data, uint32_t* size)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!blob) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    *data = blob->data;
    *size = blob->size;
    return s;
  }

  hrgls_Status hrgls_DataBlobSetData(hrgls_DataBlob blob, const uint8_t* data, uint32_t size,
    hrgls_DeletionFunction deleteFunction, void* userData)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!blob) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    blob->data = data;
    blob->size = size;
    blob->deleteFunction = deleteFunction;
    blob->deleteFunctionUserData = userData;
    return s;
  }

  hrgls_Status hrgls_DataBlobReleaseData(hrgls_DataBlob blob)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!blob) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (blob->deleteFunction) {
      // We check to make sure we have a deletion function before deleting.
      // It is not an error to not have a deletion function, though it is likely
      // to cause a memory leak.
      blob->deleteFunction(blob->deleteFunctionUserData, blob->data);
    }
    return s;
  }

  //----------------------------------------------------------------------------
  /// hrgls_APIDataBlobSourceInfo structures and methods.

  hrgls_Status hrgls_APIDataBlobSourceGetName(
    hrgls_APIDataBlobSourceInfo info, const char **returnName)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!info) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnName) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    *returnName = info->name.c_str();
    return s;
  }


  //----------------------------------------------------------------------------
  /// DataBlobSource structures and methods.

  struct hrgls_DataBlobSourceCreateParams_ {
    hrgls_API api = nullptr;
    hrgls_StreamProperties streamProperties = nullptr;
    ::std::string name;
  };

  hrgls_Status hrgls_DataBlobSourceCreateParametersCreate(
    hrgls_DataBlobSourceCreateParams *returnParams)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    hrgls_DataBlobSourceCreateParams ret;
    try {
      ret = new hrgls_DataBlobSourceCreateParams_;
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }
    *returnParams = ret;
    return s;
  }

  hrgls_Status hrgls_DataBlobSourceCreateParametersDestroy(
    hrgls_DataBlobSourceCreateParams params)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_DELETE_OF_NULL_POINTER;
    }
    try {
      delete params;
    } catch (...) {
      s = hrgls_STATUS_DELETION_FAILED;
    }
    return s;
  }

  hrgls_Status hrgls_DataBlobSourceCreateParametersSetAPI(
    hrgls_DataBlobSourceCreateParams params, hrgls_API api)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    params->api = api;
    return s;
  }

  hrgls_Status hrgls_DataBlobSourceCreateParametersSetStreamProperties(
    hrgls_DataBlobSourceCreateParams params, hrgls_StreamProperties properties)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    params->streamProperties = properties;
    return s;
  }

  hrgls_Status hrgls_DataBlobSourceCreateParametersSetName(
    hrgls_DataBlobSourceCreateParams params, const char *name)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!params) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!name) {
      params->name = "";
    } else {
      params->name = name;
    }
    return s;
  }

  struct hrgls_DataBlobSource_ {
    hrgls::datablob::DataBlobSource *stream = nullptr;

    /// C callback function that was registered
    hrgls_DataBlobSourceCallback CHandler = nullptr;

    /// UserData parameter that was given to us when the callback function was registered.
    void *CUserData = nullptr;

    /// Last info retrieved.
    struct hrgls_APIDataBlobSourceInfo_ info;
  };

  hrgls_Status hrgls_DataBlobSourceCreate(hrgls_DataBlobSource *returnStream,
    hrgls_DataBlobSourceCreateParams params)
  {
    hrgls_Status s;

    // Check the parameters.
    if (!returnStream || !params || !params->api) {
      return hrgls_STATUS_BAD_PARAMETER;
    }

    // Attempt to construct the object.
    s = hrgls_STATUS_OKAY;
    hrgls_DataBlobSource ret;
    try {
      ret = new hrgls_DataBlobSource_;
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret = nullptr;
    }
    try {
      // Get the C++ parameters out of the C structure pointers.
      hrgls::StreamProperties *prop = nullptr;
      if (params->streamProperties) { prop = params->streamProperties->props; }
      ret->stream = new hrgls::datablob::DataBlobSource(*params->api->api, *prop,
        params->name);
    } catch (...) {
      s = hrgls_STATUS_OUT_OF_MEMORY;
      ret->stream = nullptr;
      delete ret;
      ret = nullptr;
    }
    *returnStream = ret;
    return ret->stream->GetStatus();
  }

  hrgls_Status hrgls_DataBlobSourceDestroy(hrgls_DataBlobSource stream)
  {
    hrgls_Status s = hrgls_STATUS_OKAY;
    if (!stream) {
      s = hrgls_STATUS_DELETE_OF_NULL_POINTER;
    } else {
      if (!stream->stream) {
        s = hrgls_STATUS_DELETE_OF_NULL_POINTER;
      } else {
        try {
          delete stream->stream;
        } catch (...) {
          s = hrgls_STATUS_DELETION_FAILED;
        }
        stream->stream = nullptr;
      }
      try {
        delete stream;
      } catch (...) {
        s = hrgls_STATUS_DELETION_FAILED;
      }
    }
    return s;
  }

  hrgls_Status hrgls_DataBlobSourceSetStreamingState(hrgls_DataBlobSource stream,
    bool running)
  {
    if (!stream) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    try {
      return stream->stream->SetStreamingState(running);
    }
    catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  hrgls_Status hrgls_DataBlobSourceSetStreamCallback(hrgls_DataBlobSource stream,
    hrgls_DataBlobSourceCallback handler, void *userData)
  {
    if (!stream) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    stream->CHandler = handler;
    stream->CUserData = userData;
    try {
      // If we are supposed to be calling a callback handler, then set our callback handler
      // as the intercept.  If not, then set it to nullptr so that no callbacks are called.
      if (!handler) {
        return stream->stream->SetStreamCallback(nullptr, nullptr);
      }
      return stream->stream->SetStreamCallback(DataBlobSourceCallback, stream);
    }
    catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  hrgls_Status hrgls_DataBlobSourceGetNextBlob(hrgls_DataBlobSource stream,
    hrgls_DataBlob * blob, struct timeval timeout)
  {
    if (!stream) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!blob) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    try {
      hrgls::datablob::DataBlob f = stream->stream->GetNextBlob(timeout);
      hrgls_Status s = hrgls_DataBlobCopy(blob, f.RawDataBlob());
      if (s != hrgls_STATUS_OKAY) {
        return s;
      }
      return stream->stream->GetStatus();
    }
    catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

  hrgls_Status hrgls_DataBlobSourceGetInfo(hrgls_DataBlobSource stream,
    hrgls_APIDataBlobSourceInfo *returnInfo)
  {
    if (!stream) {
      return hrgls_STATUS_NULL_OBJECT_POINTER;
    }
    if (!returnInfo) {
      return hrgls_STATUS_BAD_PARAMETER;
    }
    try {
      hrgls::DataBlobSourceDescription  info;
      info = stream->stream->GetInfo();
      stream->info.name = info.Name();
      *returnInfo = &stream->info;
      return stream->stream->GetStatus();
    }
    catch (...) {
      return hrgls_STATUS_INTERNAL_EXCEPTION;
    }
  }

}

//-----------------------------------------------------------
// C++ definitions go below here.

static void LogMessageCallback(hrgls::Message &message, void *userData)
{
  // Get a pointer to the information we need to service the callback.
  if (!userData) { return; }
  hrgls_API info = static_cast<hrgls_API>(userData);

  if (info->CHandler) {
    // Make a copy of the message to hand to the callback handler so that
    // they will destroy it just as they would one that is returned by
    // GetNextMessage().  This is extra work for this case, but it makes the
    // interface consistent between these two cases.
    hrgls_Message copy;
    hrgls_MessageCopy(&copy, message.RawMessage());
    info->CHandler(copy, info->CUserData);
  }
}

static void DataBlobSourceCallback(hrgls::datablob::DataBlob &blob, void *userData)
{
  // Get a pointer to the information we need to service the callback.
  if (!userData) { return; }
  hrgls_DataBlobSource info = static_cast<hrgls_DataBlobSource>(userData);

  if (info->CHandler) {
    // Make a copy of the blob to hand to the callback handler so that
    // they will destroy it just as they would one that is returned by
    // GetNextBlob().  This is extra work for this case, but it makes the
    // interface consistent between these two cases.
    hrgls_DataBlob copy;
    hrgls_DataBlobCopy(&copy, blob.RawDataBlob());
    info->CHandler(copy, info->CUserData);
  }
}

//-----------------------------------------------------------
// StreamProperties is merely a container when used in the wrapped
// C++ implementation, so we can implement it here.  The GetRawProperties()
// method should not be called here, so we return a null pointer.
/// @todo Consider adding an _impl.hpp for this class to have it match
/// the others.
class hrgls::StreamProperties::StreamProperties_private {
public:
  hrgls_Status      status = hrgls_STATUS_OKAY;
  double          rate = 30;
};

hrgls::StreamProperties::StreamProperties()
{
  m_private.reset(new StreamProperties_private());
}

hrgls::StreamProperties::~StreamProperties()
{
  //delete m_private;
}

hrgls::StreamProperties::StreamProperties(const hrgls::StreamProperties &copy)
{
  m_private.reset(new StreamProperties_private());
  *m_private = *copy.m_private;
}

hrgls::StreamProperties &hrgls::StreamProperties::operator = (const hrgls::StreamProperties &copy)
{
  *m_private = *copy.m_private;
  return *this;
}

hrgls_Status hrgls::StreamProperties::GetStatus()
{
  if (!m_private) {
    return hrgls_STATUS_NULL_OBJECT_POINTER;
  }
  hrgls_Status ret = m_private->status;
  m_private->status = hrgls_STATUS_OKAY;
  return ret;
}

double hrgls::StreamProperties::Rate()
{
  if (!m_private) {
    return 0;
  }
  else {
    m_private->status = hrgls_STATUS_OKAY;
    return m_private->rate;
  }
}

hrgls_Status hrgls::StreamProperties::Rate(double rate)
{
  if (!m_private) {
    return hrgls_STATUS_NULL_OBJECT_POINTER;
  }
  m_private->rate = rate;
  return hrgls_STATUS_OKAY;
}
