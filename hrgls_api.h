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
 * @file hrgls_api.h
 * @brief Hourglass C API, an extensible wrapped API that is exposed by the DLLs.
 *
 * This is the C implementation of the API, which is wrapped both above and below
 * by C++ (an hourglass design) and whose C++ API is wrapped using SWIG into Python.
 * You probably do not want to use this C API directly, but rather want to include
 * the hrgls_api.hpp file, but look at the hrgls_api_defs.hpp file for the objects that
 * you will actually use.  If you want to write C code directly, then the current file
 * is the one you'll want.  Each of the sets of functions use an opaque pointer to
 * a structure to enable future expansion without requiring code to be recompiled.
* @author Russell Taylor.
* @date January 18, 2020.
*/

//----------------------------------------------------------------------------------------
// Include the configuration file that defines the import or export for DLLs on Windows.
// These are left undefined on other platforms.  The CMake system adds a definition that
// causes export when the library is built and import for other applications.
#include <hrgls_export.h>
#include <stdint.h>
#include <stdbool.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <Winsock2.h> // For struct timeval
#endif

//----------------------------------------------------------------------------------------
// If we're compiling with a C++ compiler, explicitly mark this section for C export.
// This will keep the symbols unmangled.
#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------------
/// @brief Status enumeration, returned by all hrgls_* C API functions.
typedef int hrgls_Status;
  /// @brief All is okay.
  #define hrgls_STATUS_OKAY (0)

  /// @brief A timeout was exceeded (not an error).
  #define hrgls_STATUS_TIMEOUT (1)

  /// @brief Can be used to see if the return was a system error.
  #define hrgls_STATUS_HIGHEST_WARNING (1000)

  /// @brief Error: Bad parameter passed to function.
  #define hrgls_STATUS_BAD_PARAMETER (1001)
  /// @brief Error: Out of memory when trying to execute function.
  #define hrgls_STATUS_OUT_OF_MEMORY (1002)
  /// @brief Error: Function not yet implemented.
  #define hrgls_STATUS_NOT_IMPLEMENTED (1003)
  /// @brief Error: Attempted deletion of a NULL pointer.
  #define hrgls_STATUS_DELETE_OF_NULL_POINTER (1004)
  /// @brief Error: Attempting to delete an object failed.
  #define hrgls_STATUS_DELETION_FAILED (1005)
  /// @brief Error: Internal failure: calling an object with a NULL object pointer.
  #define hrgls_STATUS_NULL_OBJECT_POINTER (1006)
  /// @brief Error: Internal failure: Exception inside the hourglass.
  #define hrgls_STATUS_INTERNAL_EXCEPTION (1007)

/// @brief Helper function to return a descriptive error message based on a status value.
/// @param [in] status Return value from an hrgls_* C API or C++ API call.
/// @return Null-terminated string describing the status condition.
HRGLS_EXPORT const char *hrgls_ErrorMessage(hrgls_Status status);

//---------------------------------------------------------------------------
// This section includes definitions and functions to deal with log messages
// coming from objects implemented inside the API.  These can be informational,
// warning, or error messages.  They are created asynchronously and can be
// delivered either via a queue or by callbacks.

//---------------------------------------------------------------------------
/// @brief Data type enumeration for message levels in the logging and issue
/// systems.
typedef int32_t hrgls_MessageLevel;
/// @brief Minimum number representing an information message.
#define hrgls_MESSAGE_MINIMUM_INFO (INT32_MIN)
/// @brief Minimum number representing a warning message.
#define hrgls_MESSAGE_MINIMUM_WARNING (0)
/// @brief Minimum number representing an error message.
#define hrgls_MESSAGE_MINIMUM_ERROR (INT32_MAX/3)
/// @brief Minimum number representing a critical error message.
#define hrgls_MESSAGE_MINIMUM_CRITICAL_ERROR (2*hrgls_MESSAGE_MINIMUM_ERROR)

//---------------------------------------------------------------------------
/// @brief Opaque pointer to a C structure that stores a logging message.
///
/// Use the hrgls_MessageGet* functions to read its values.  Call the
/// hrgls_MessageSet* functions to set them (not normally called by client code).
/// When done with the info, call hrgls_MessageDestroy().

typedef struct hrgls_Message_ *hrgls_Message;

/// @brief Create a Message and initialize with default values.
/// @param [out] returnMessage Pointer to the Message to be constructed.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageCreate(hrgls_Message *returnMessage);

/// @brief Create a Message and initialize with copies of values from another.
/// @param [out] returnMessage Pointer to the Message to be constructed.
/// @param [in] MessageToCopy The message to copy from.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageCopy(hrgls_Message *returnMessage,
	hrgls_Message MessageToCopy);

/// @brief Destroy a Message.
///
/// Used to destroy Messages obtained from hrgls_MessageCreate() or
/// hrgls_GetNextLogMessage().
/// @param [in] obj Structure to be destroyed.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageDestroy(hrgls_Message obj);

/// @brief Read the Message value parameter.
/// @param [in] obj Structure to use.
/// @param [out] val Pointer to the location to store a pointer to the result.
///        This value is valid until hrgls_MessageDestroy() is called on
///        this Message.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageGetValue(hrgls_Message obj,
	const char **val);

/// @brief Set the Message value parameter.
/// @param [in] obj Structure to use.
/// @param [in] val New value to set.  This value is copied in.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageSetValue(hrgls_Message obj,
	const char *val);

/// @brief Read the Message timestamp parameter in UTC.
/// @param [in] obj Structure to use.
/// @param [out] val Pointer to the location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageGetTimeStamp(hrgls_Message obj,
  struct timeval* val);

/// @brief Set the Message timestamp parameter in UTC.
/// @param [in] obj Structure to use.
/// @param [in] val New value to set.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageSetTimeStamp(hrgls_Message obj,
  struct timeval val);

/// @brief Read the Message level parameter.
/// @param [in] obj Structure to use.
/// @param [out] val Pointer to the location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageGetLevel(hrgls_Message obj,
	hrgls_MessageLevel *val);

/// @brief Set the Message level parameter.
/// @param [in] obj Structure to use.
/// @param [in] val New value to set.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_MessageSetLevel(hrgls_Message obj,
	hrgls_MessageLevel val);

//----------------------------------------------------------------------------------------
/// @brief Opaque pointer to a C structure that stores parameters to pass to hrgls_APICreate().
typedef struct hrgls_APICreateParams_ *hrgls_APICreateParams;

/// @brief Construct a set of parameters that can be used to construct an API.
///
/// Constructs a set of parameters to be passed to hrgls_APICreate().  The various
/// parameters should be set here before calling that routine.  Once the API has been
/// created, call hrgls_APICreateParamatersDestroy() to free the resources associated
/// with this parameter set.
/// @param [out] returnParams Pointer to location to store the result, not changed on error.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreateParametersCreate(hrgls_APICreateParams *returnParams);

/// @brief Destroy a set of parameters created by hrgls_APICreateParametersCreate().
/// @param [in] params Object that was created by hrgls_APICreateParametersCreate().
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreateParametersDestroy(hrgls_APICreateParams params);

/// @brief Get the user name previously set by hrgls_APICreateParametersSetName().
/// @param [in] params Object that was created by hrgls_APICreateParametersCreate().
/// @param [out] returnName Pointer to a location to store a pointer to the name.
///        This pointer will be valid until hrgls_APICreateParametersDestroy() is called
///        on these params.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreateParametersGetName(
  hrgls_APICreateParams params,
  const char **returnName);

/// @brief Set the name of the user who is connecting to the API.
/// @param [in] params Object that created by hrgls_APICreateParametersCreate().
/// @param [in] name Name of the user who is connecting to the API.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreateParametersSetName(
  hrgls_APICreateParams params,
  const char *name);

/// @brief Get the user name previously set by hrgls_APICreateParametersSetCredentials().
/// @param [in] params Object that was created by hrgls_APICreateParametersCreate().
/// @param [out] returnCredentials Pointer to a location to store a pointer to the credentials.
///        This pointer will be valid until hrgls_APICreateParametersDestroy() is called
///        on these params.
/// @param [out] returnSize Pointer to a location to store the size of the pointed-to
///        credentials.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreateParametersGetCredentials(
  hrgls_APICreateParams params,
  const uint8_t **returnCredentials, uint32_t *returnSize);

/// @brief Set the credentials of the user who is connecting to the API.
/// @param [in] params Object that created by hrgls_APICreateParametersCreate().
/// @param [in] credentials Credentials of the user who is connecting to the API.
/// @param [in] size Size in bytes of the credentials.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreateParametersSetCredentials(
  hrgls_APICreateParams params,
  const uint8_t *credentials, uint32_t size);

//----------------------------------------------------------------------------------------
/// @brief Opaque pointer to a C structure that manages an API.
typedef struct hrgls_API_ *hrgls_API;

/// @brief Construct an API.
///
/// hrgls_APIDestroy() should be called when the application is finished with the API
/// to avoid leaking resources.
/// @param [out] returnAPI Pointer to location to store the result, not changed on error.
/// @param [in] params Parameters to use when constructing the API.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APICreate(hrgls_API *returnAPI, hrgls_APICreateParams params);

/// @brief Destroy an API.
/// @param [in] api API returned from hrgls_APICreate().
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APIDestroy(hrgls_API api);

/// @brief Structure used to store an API version
typedef struct {
  uint16_t major;   ///< Major version number the same means backwards compatible.
  uint16_t minor;   ///< Minor version number change means new features.
  uint16_t patch;   ///< Patch version number change means bug fixes.
} hrgls_VERSION;
/// @brief Reads the API version.
/// @param [in] api Object returned by hrgls_APICreate().
/// @param [out] returnVersion Pointer to location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APIGetVersion(hrgls_API api, hrgls_VERSION *returnVersion);

/// @brief Reads the current system time in UTC.
/// @param [in] api Object returned by hrgls_APICreate().
/// @param [out] returnTime Pointer to location to store the result in UTC.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APIGetCurrentSystemTime(hrgls_API api, struct timeval *returnTime);

/// @brief Reads the current verbosity.
/// @param [in] api Object returned by hrgls_APICreate().
/// @param [out] returnVerbosity Pointer to location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APIGetVerbosity(hrgls_API api, uint16_t *returnVerbosity);

/// @brief Sets the verbosity.
/// @param [in] api Object returned by hrgls_APICreate().
/// @param [in] verbosity New value, 0 (default) means that not even error messages are printed.
///         1-100 includes error messages to stderr, 101-200 includes warnings to stderr.
///         201+ includes ever more verbose information and code tracing to stdout.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APISetVerbosity(hrgls_API api, uint16_t verbosity);

//----------------------------------------------------------------------------------------
/// @brief Opaque pointer to a C structure that stores a DataBlobSource description.
///
/// The Name in this structure is used to identify the datablob source to other objects in the
/// system.
///
/// Use the hrgls_APIDataBlobSourceGet* functions to read its values.

typedef struct hrgls_APIDataBlobSourceInfo_ *hrgls_APIDataBlobSourceInfo;

/// @brief Read the name of the DataBlobSource.
/// @param [in] info Structure to get the information from.
/// @param [out] returnName Pointer to location to store a pointer to the constant
///              null-terminated string holding the DataBlobSource name.  This pointer will
///              remain valid until the next call to hrgls_APIGetAvailableDataBlobSourceCount()
///              or until hrgls_APIDestroy() is called, so it should be copied if it is to be
///              used after that.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APIDataBlobSourceGetName(
  hrgls_APIDataBlobSourceInfo info, const char **returnName);

/// @brief Latches DataBlobSource information for retrieval by hrgls_APIGetAvailableDataBlobSourceInfo().
///
/// This function latches the DataBlobSource information until the next call to this
/// function, so that calls to hrgls_APIGetAvailableDataBlobSourceInfo() will return
/// consistent results.
/// This function must be called before hrgls_APIGetAvailableDataBlobSourceInfo() will return
/// non-empty results.
/// @param [in] api Object returned by hrgls_APICreate().
/// @param [out] returnCount Pointer to location to store the result.
/// @return Status of the API after attempting the operation, hrgls_STATUS_OKAY on success.
HRGLS_EXPORT hrgls_Status hrgls_APIGetAvailableDataBlobSourceCount(hrgls_API api,
  uint32_t *returnCount);

/// @brief Reads information latched by hrgls_APIGetAvailableDataBlobSourceCount().
///
/// The function hrgls_APIGetAvailableDataBlobSourceCount() must be called to latch the DataBlobSource
/// information before this function will return anything.
/// @param [in] api Object returned by hrgls_APICreate().
/// @param [in] which Index of the DataBlobSource info to read, with the first
///        being 0.  Before calling this, you can call hrgls_APIGetAvailableDataBlobSourceCount()
///        to find out how many DataBlobSources there are.
/// @param [out] returnInfo Pointer to location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APIGetAvailableDataBlobSourceInfo(hrgls_API api, uint32_t which,
  hrgls_APIDataBlobSourceInfo *returnInfo);

//----------------------------------------------------------------------------------------
/// @brief Type declaration for log message callback handler function.
///
/// Function declaration for the type of a callback handler that will be called
/// to handle a new log message.  This includes the message data and a user-data pointer
/// that the caller passed in.
/// @param [in] message The message that has been received.
/// @param [in] userData Pointer to the data that was passed into the userData
///             parameter for the hrgls_LogMessageCallback function, handed
///             back here so that the client can tell itself how to behave.  This
///             is usually cast into a pointer to the actual object type.
typedef void(*hrgls_LogMessageCallback)(hrgls_Message const message, void *userData);

/// @brief Set the streaming state for log messages.  Must be set true to receive messages.
/// @param [in] api hrgls_API created by calling hrgls_APICreate().
/// @param [in] running Set to true begin streaming, false to stop.  The API
///        starts out not streaming.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APISetLogMessageStreamingState(hrgls_API api, bool running);

/// @brief Set a callback function to be called on each new log message.
///
/// Either hrgls_LogMessageSetStreamCallback() or hrgls_GetNextLogMessage() should
/// be used to get messages, but not both.  If the callback handler
/// is set, it will be called from a separate thread whenever a new message is available.
/// If there is no callback handler set, messages will queue up in memory until the program calls
/// hrgls_GetNextLogMessage().
/// @param [in] api hrgls_API created by calling hrgls_APICreate().
/// @param [in] handler Function to call from the receiving thread whenever a new message
///        is received.  Set to NULL to disable.    Note: The receiver
///        must destroy the message by calling hrgls_MessageDestroy() when it is
///        done with it.
/// @param [in] userData Pointer that is passed into the function whenever it is called.
///        Should point to a variable or structure or class object that has all of the state
///        that the function needs to process a message.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APISetLogMessageCallback(hrgls_API api,
	hrgls_LogMessageCallback handler, void *userData);

/// @brief Get the next available log message.
///
/// Either hrgls_APISetLogMessageCallback() or hrgls_APIGetNextLogMessage() should
/// be used to get log messages, but not both.  If the callback handler
/// is set, it will be called from a separate thread whenever a new message is available.
/// If there is no callback handler set, messages will queue up in memory until the program calls
/// hrgls_APIGetNextLogMessage().
/// @param [in] api hrgls_API created by calling hrgls_APICreate().
/// @param [in] message A pointer to the message that has been received.  Note: The receiver
///        must destroy the message by calling hrgls_MessageDestroy() when it is
///        done with it.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.  Returns
///        hrgls_STATUS_TIMEOUT and sets the message pointer to NULL if there is
///        not a message available.
HRGLS_EXPORT hrgls_Status hrgls_APIGetNextLogMessage(hrgls_API api, hrgls_Message *message);

/// @brief Sets the range of message levels to be returned.
///
/// This method filters log messages so that only those of sufficient urgency
/// are returned.  It should be called before streaming is enabled.
/// @param [in] api hrgls_API created by calling hrgls_APICreate().
/// @param level Minimum level to be returned, defaults to hrgls_MESSAGE_MINIMUM_INFO.
/// @return hrgls_STATUS_OKAY on success, a specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_APISetLogMessageMinimumLevel(hrgls_API api, hrgls_MessageLevel level);

//---------------------------------------------------------------------------
// DataBlobSource API class and its parameters and methods.

//----------------------------------------------------------------------------------------
/// @brief Opaque pointer to a C structure that stores the properties of an hrgls_DataBlobSource.
///
/// Stores the properties of a DataBlobSource.  Passed into the DataBlobSource to describe how
/// it should behave.
typedef struct hrgls_StreamProperties_ *hrgls_StreamProperties;

/// @brief Construct an hrgls_StreamProperties with default values.
///
/// The application should call hrgls_StreamPropertiesDestroy() when it is done
/// with them to avoid leaking resources.
/// @param [out] returnProp Pointer to location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_StreamPropertiesCreate(hrgls_StreamProperties *returnProp);

/// @brief Destroy an hrgls_StreamProperties, freeing its resources.
/// @param [in] prop Properties returned by hrgls_StreamPropertiesCreate().
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_StreamPropertiesDestroy(hrgls_StreamProperties prop);

/// @brief Read the blobs/second for the stream.
/// @param [in] prop Structure to use.
/// @param [out] val Pointer to the location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_StreamPropertiesGetRate(hrgls_StreamProperties prop, double *val);

/// @brief Set the blobs/second for the stream.
/// @param [in] prop Structure to use.
/// @param [in] val Its default value is 30.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_StreamPropertiesSetRate(hrgls_StreamProperties prop, double val);

//----------------------------------------------------------------------------------------
/// @brief Stores a blob from an hrgls_DataBlobSource.
typedef struct hrgls_DataBlob_ *hrgls_DataBlob;

/// @brief Create a DataBlob and initialize with default values.
///
/// Call hrgls_DataBlobDestroy() when done with the DataBlob (and after
/// calling release on its data) to avoid leaking resources.
/// @param [out] returnBlob Pointer to the DataBlob to be constructed.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobCreate(hrgls_DataBlob*returnBlob);

/// @brief Create a DataBlob and initialize with values from another DataBlob.
///
/// Call hrgls_DataBlobDestroy() when done with the DataBlob (and after
/// calling release on its data) to avoid leaking resources.
/// @param [out] returnBlob Pointer to the DataBlob to be constructed.
/// @param [in] blobToCopy The DataBlob to copy information from.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobCopy(hrgls_DataBlob*returnBlob,
  hrgls_DataBlob blobToCopy);

/// @brief Destroy a DataBlob.
///
/// Used to destroy a DataBlob obtained from hrgls_DataBlobCreate(),
/// hrgls_DataBlobCopy(), or hrgls_DataBlobSourceGetNextBlob(), or
/// in a registered callback handler on a render stream.
/// Does not free the underlying data -- call hrgls_DataBlobReleaseData()
/// on it to do that.
/// @param [in] blob DataBlob to be destroyed.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobDestroy(hrgls_DataBlob blob);

/// @brief Read the blob creation time in UTC.
/// @param [in] blob Structure to use.
/// @param [out] val Pointer to the location to store the result.
/// @return hrgls_Status on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobGetTime(hrgls_DataBlob blob, struct timeval *val);

/// @brief Set the blob creation time in UTC.  Not normally called by client code.
/// @param [in] blob Structure to use.
/// @param [in] val New time to set.
/// @return hrgls_Status on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSetTime(hrgls_DataBlob blob, struct timeval val);


/// @brief Read a pointer to the blob data and size.
/// @param [in] blob Structure to use.
/// @param [out] data Pointer to a pointer to the binary data.
///              The data is not copied, only a pointer to the data is stored.
/// @param [out] size Pointer to a location to store the blob size.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobGetData(hrgls_DataBlob blob, const uint8_t** data, uint32_t* size);

/// @brief Type of a deletion function, not normally used by client code.
typedef void(*hrgls_DeletionFunction)(void* userData, const uint8_t* dataToDelete);

/// @brief Set the blob data.  Not normally called by client code.
/// @param [in] blob Structure to use.
/// @param [in] data Pointer to the binary blob data.
///              The data is not copied, only a pointer to the data is stored.
/// @param [in] size The blob size.
/// @param [in] deleteFunction Pointer to a deletion function, or NULL for no deletion.
/// @param [in] userData Passed back to the deleteFunction, may be NULL.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSetData(hrgls_DataBlob blob, const uint8_t* data, uint32_t size,
  hrgls_DeletionFunction deleteFunction, void* userData);

/// @brief Release the data associated with a DataBlob.
///
/// This function will be implemented in such a way that it is robust to being
/// called multiple times to release the same data, with the second and later calls
/// doing nothing.
/// @param [in] blob Structure to use.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobReleaseData(hrgls_DataBlob blob);

//----------------------------------------------------------------------------------------
/// @brief Opaque pointer to C structure that stores parameters to pass to hrgls_DataBlobSourceCreate.
typedef struct hrgls_DataBlobSourceCreateParams_ *hrgls_DataBlobSourceCreateParams;

/// @brief Construct a set of parameters that can be used to construct an hrgls_DataBlobSource.
///
/// Constructs a set of parameters to be passed to hrgls_DataBlobSourceCreate().  The various
/// parameters should be set here before calling that routine.  Once the stream has been
/// created, call hrgls_DataBlobSourceCreateParametersDestroy() to free the resources associated
/// with this parameter set.
/// @param [out] returnParams Pointer to location to store the result, not changed on error.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceCreateParametersCreate(
  hrgls_DataBlobSourceCreateParams *returnParams);

/// @brief Destroy a set of parameters created by hrgls_DataBlobSourceCreateParametersCreate().
/// @param [in] params Object that was created by hrgls_DataBlobSourceCreateParametersCreate().
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceCreateParametersDestroy(
  hrgls_DataBlobSourceCreateParams params);

/// @brief Set the API in which the hrgls_DataBlobSource will be created.
/// @param [in] params Object that created by hrgls_DataBlobSourceCreateParametersCreate().
/// @param [in] api API in which the stream will be created.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceCreateParametersSetAPI(
  hrgls_DataBlobSourceCreateParams params,
  hrgls_API api);

/// @brief Set the name of the hrgls_DataBlobSource.
/// @param [in] params Object that created by hrgls_DataBlobSourceCreateParametersCreate().
/// @param [in] name Name for the stream.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceCreateParametersSetName(
  hrgls_DataBlobSourceCreateParams params,
  const char *name);

/// @brief Enable or disable letterbox cropping.
/// @param [in] params Object that created by hrgls_DataBlobSourceCreateParametersCreate().
/// @param [in] properties Stream properties to set.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceCreateParametersSetStreamProperties(
  hrgls_DataBlobSourceCreateParams params,
  hrgls_StreamProperties properties);

//----------------------------------------------------------------------------------------
/// @brief Type declaration for data-blob callback handler function.
///
/// Function declaration for the type of a callback handler that will be called
/// to handle a new blob.  This includes the data and a user-data pointer
/// that the caller passed in.
/// @param [in] blob The blob that has been received from the DataBlobSource.
///             The handler must call ReleaseData() on the blob when it is done with it.
/// @param [in] userData Pointer to the data that was passed into the userData
///             parameter for the hrgls_DataBlobSourceSetStreamCallback function, handed
///             back here so that the client can tell itself how to behave.  This
///             is usually cast into a pointer to the actual object type.
typedef void(*hrgls_DataBlobSourceCallback)(hrgls_DataBlob const blob, void *userData);

//----------------------------------------------------------------------------------------
/// @brief Opaque pointer to a C structure that represents a rendering stream.
typedef struct hrgls_DataBlobSource_ *hrgls_DataBlobSource;

/// @brief Construct a DataBlobSource object.
///
/// hrgls_DataBlobSourceDestroy() should be called when the application is finished with the object
/// to avoid leaking resources.
/// @param [out] returnStream Pointer to location to store the result, not changed on error.
/// @param [in] params Parameters to use when constructing the object.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceCreate(hrgls_DataBlobSource*returnStream,
  hrgls_DataBlobSourceCreateParams params);

/// @brief Destroy a DataBlobSource object.
/// @param [in] stream DataBlobSource created by calling hrgls_DataBlobSourceCreate().
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceDestroy(hrgls_DataBlobSource stream);

/// @brief Set the streaming state for a DataBlobSource.  Must be set true to receive blobs.
/// @param [in] stream DataBlobSource created by calling hrgls_DataBlobSourceCreate().
/// @param [in] running Set to true begin streaming, false to stop.  DataBlobSources
///        start out not streaming.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceSetStreamingState(hrgls_DataBlobSource stream,
  bool running);

/// @brief Set a callback function to be called by the receiving thread on each new blob.
///
/// Either hrgls_DataBlobSourceSetStreamCallback() or hrgls_DataBlobSourceGetNextBlob() should
/// be used to get blobs from the DataBlobSource, but not both.  If the callback handler
/// is set, it will be called from a separate thread whenever a new blob is available.
/// If there is no callback handler set, blobs will queue up in memory until the program calls
/// hrgls_DataBlobSourceGetNextBlob().
/// @param [in] stream DataBlobSource created by calling hrgls_DataBlobSourceCreate().
/// @param [in] handler Function to call from the receiving thread whenever a new blob
///        is received.  Set to NULL to disable.
/// @param [in] userData Pointer that is passed into the function whenever it is called.
///        Should point to a variable or structure or class object that has all of the state
///        that the function needs to process a blob.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceSetStreamCallback(hrgls_DataBlobSource stream,
  hrgls_DataBlobSourceCallback handler, void *userData);

/// @brief Get the next blob available from a render stream.
///
/// Either hrgls_DataBlobSourceSetStreamCallback() or hrgls_DataBlobSourceGetNextBlob() should
/// be used to get blobs from the DataBlobSource, but not both.  If the callback handler
/// is set, it will be called from a separate thread whenever a new blob is available.
/// If there is no callback handler set, blobs will queue up in memory until the program calls
/// hrgls_DataBlobSourceGetNextBlob().
/// @param [in] stream DataBlobSource created by calling hrgls_DataBlobSourceCreate().
/// @param [in] blob A pointer to the blob that has been received.  Note: The receiver
///        must call ReleaseData() on the blob when it is done with it and must
///        also destroy the blob by calling hrgls_DataBlobDestroy().
/// @param [in] timeout Specifies how long to busy-wait for a blob before returning.  If
///        set to 0, returns immediately whether or not there is a blob.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.  Returns
///        hrgls_STATUS_TIMEOUT and a blob that has no data if there is not a blob available
///        within the specified time.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceGetNextBlob(hrgls_DataBlobSource stream,
  hrgls_DataBlob *blob, struct timeval timeout);

/// @brief Gets information (including the name) about the DataBlobSource.
///
/// @param [in] stream DataBlobSource created by calling hrgls_DataBlobSourceCreate().
/// @param [out] returnInfo Pointer to location to store the result.
/// @return hrgls_STATUS_OKAY on success, specific error code on failure.
HRGLS_EXPORT hrgls_Status hrgls_DataBlobSourceGetInfo(hrgls_DataBlobSource stream,
  hrgls_APIDataBlobSourceInfo *returnInfo);

#ifdef __cplusplus
}
#endif
