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
* @file hrgls_api_defs.hpp
* @brief Hourglass C++ API, an extensible wrapped API that is exposed by the DLLs.
*
* This is the C++ wrapper for the API, which wraps the C API and handles the
* construction and destruction and copying of objects.  This file declares the interface
* both for the client applications and for the developer (who implements the classes
* and methods found herein).  This C++ API is wrapped using SWIG into Python.
* To make use of this file, you should include hrgls_api.hpp in a client source file;
* that file includes the definitions of the methods declared here for the upper
* half of the wrapping (hourglass design).
* @author Russell Taylor.
* @date January 18, 2020.
*/

// Include the C header file to get access to the definitions for the types and
// functions.
#include "hrgls_api.h"

#include <string>
#include <vector>
#include <memory>

// Everything defined here lives in the hrgls namespace.
namespace hrgls {

  // Forward declare classes that we will befriend in namespaces we will use
  namespace datablob {
    class DataBlobSource;
  };

  /// @brief Stores the properties of a DataBlobSource.
  ///
  /// This class stores the properties of a DataBlobSource and it used to describe
  /// the desired properties when a DataBlobSource is created.  It wraps an
  /// hrgls_StreamProperties C structure and provides a C++ interface for it.
  /// The GetStatus() method should be called after each method (including
  /// the constructor) to make sure that the operation was a success.

  class StreamProperties {
  public:

    /// @brief Construct properties with default values for all items.
    StreamProperties();

    /// @brief Destroys the properties.
    ~StreamProperties();

    /// @brief Construct one StreamProperties by copying an existing one.
    /// @param [in] copy StreamProperties to copy from.
    StreamProperties(const StreamProperties& copy);

    /// @brief Set the values of a StreamProperties to match another.
    /// @param [in] copy StreamProperties to copy from.
    StreamProperties &operator = (const StreamProperties &copy);

    /// @brief Returns the status of the most-recent operation and clears error/warnings.
    ///
    /// This should be called after the construction of a StreamProperties and after each method
    /// call that does not itself return an hrgls_Status to ensure that the operation
    /// completed.
    /// @return hrgls_Status returned by the most-recent operation on the wrapped
    ///         class, or other errors in case the StreamProperties itself is broken.
    hrgls_Status GetStatus();

    /// @brief Read the blobs/second for the DataBlobSource.
    /// @return Rate of the DataBlobSource.
    double Rate();
    /// @brief Set the blobs/second for the DataBlobSource.
    /// @param [in] rate Its default value is 30.
    /// @return Returns hrgls_STATUS_OKAY on success and a specific code on failure.
    ///         GetStatus() does not need to be called after this method because it
    ///         is returned here.
    hrgls_Status Rate(double rate);

    /// @brief Private class declared for definition and use by the API implementation.
    class StreamProperties_private;

  protected:
    /// @brief Protected method for use by the API implementation.
    ::std::shared_ptr<hrgls_StreamProperties_> GetRawProperties() const;
    /// @cond INTERNAL
    friend datablob::DataBlobSource;
    /// @endcond

  private:
    ::std::shared_ptr<StreamProperties_private> m_private;
  };

  /// @brief Stores the description of a DataBlobSource.
  ///
  /// This class stores the description of a DataBlobSource.  It is a
  /// C++ analog of the hrgls_APIDataBlobSourceInfo C structure.  It is a data-only
  /// structure and is fully implemented in the header file.  Coupling between this
  /// and the C structure is done inside the implementation.
  ///
  /// The Name in this structure is used to identify the DataBlobSource to other objects in the
  /// system.
  ///
  /// This is returned by hrgls::API::GetAvailableDataBlobSources().

  class DataBlobSourceDescription {
  public:
    DataBlobSourceDescription() {};
    ~DataBlobSourceDescription() {};

    /// @brief Read the name of the DataBlobSource.
    ::std::string Name() const { return m_name; };
    /// @brief Set the name of the DataBlobSource (set in struct, not in actual DataBlobSource).
    void Name(const ::std::string name) { m_name = name; };

  private:
    ::std::string m_name;
  };

  /// @brief Holds the data for a logging or issue Message.
  ///
  /// This class controls and reports a Message.  It wraps an
  /// hrgls_Message C structure and provides a C++ interface for it.
  /// The GetStatus() method should be called after each method (including
  /// the constructor) to make sure that the operation was a success.

  class Message {
  public:
	  /// @brief Creates a Message.
	  Message();

	  /// @brief Creates a Message and sets its entries.  Not used by client code.
	  Message(::std::string value, struct timeval time,
		  hrgls_MessageLevel level);

	  /// @brief Creates a Message from an hrgls_Message.
	  /// @param [in] Message The hrgls_Message to refer to when calling this
	  ///             object's methods.  It is copied on construction and
	  ///             destroyed in ~Message().
	  Message(hrgls_Message Message);

	  /// @brief Destroys a Message.
	  ~Message();

	  /// @brief Copy constructor for a Message, not usually needed by client code.
	  /// @param [in] copy Message to copy.  This performs a deep copy of the the
	  ///        Message to avoid double deletion.
	  Message(const Message& copy);

	  /// @brief Assignment for a Message, not usually needed by client code.
	  /// @param [in] copy Message to copy.  This performs a deep copy of the
	  ///        Message to avoid double deletion.
	  Message &operator = (const Message &copy);

	  /// @brief Returns the status of the most-recent operation and clears error/warnings.
	  ///
	  /// This should be called after the construction of a Message and after each method
	  /// call that does not itself return an hrgls_Status to ensure that the operation
	  /// completed.
	  /// @return hrgls_Status returned by the most-recent operation on the wrapped
	  ///         class, or other errors in case the object itself is broken.
	  hrgls_Status GetStatus();

	  /// @brief Get the value of the message.
	  ::std::string Value() const;

	  /// @brief Set the value of the message.
	  /// @param [in] value Value to set.
    /// @return Returns hrgls_STATUS_OKAY on success and a specific code on failure.
    ///         GetStatus() does not need to be called after this method because it
    ///         is returned here.
    hrgls_Status Value(::std::string value);

	  /// @brief Get the timestamp of the message in UTC.
	  struct timeval TimeStamp() const;

	  /// @brief Set the timestamp of the message in UTC.
	  /// @param [in] value Value to set.
    /// @return Returns hrgls_STATUS_OKAY on success and a specific code on failure.
    ///         GetStatus() does not need to be called after this method because it
    ///         is returned here.
    hrgls_Status TimeStamp(struct timeval value);

	  /// @brief Get the level of the message.
	  hrgls_MessageLevel Level() const;

	  /// @brief Set the level of the message.
	  /// @param [in] value Value to set.
    /// @return Returns hrgls_STATUS_OKAY on success and a specific code on failure.
    ///         GetStatus() does not need to be called after this method because it
    ///         is returned here.
    hrgls_Status Level(hrgls_MessageLevel value);

	  /// @brief Accessor for the passed-in information during construction, not used by client code.
	  hrgls_Message const RawMessage() const;

	  /// @brief Private class declared for definition and use by the API implementation.
	  class Message_private;

  private:
	  Message_private * m_private = nullptr;
  };

  //----------------------------------------------------
  // Constants defined for use below.

  /// @brief Default user value, indicating no user.
  static ::std::string ANONYMOUS_USER;
  /// @brief Default credentials value, indicating no credentials supplied.
  static ::std::vector<uint8_t> NO_CREDENTIALS;
  /// @brief Default DataBlobSource name, indicating "any DataBlobSource".
  static ::std::string ANY_DATABLOBSOURCE;

  /// @brief Implements the root-level API object.
  ///
  /// This class provides access to the top-level API.  It wraps an
  /// hrgls_API C structure and provides a C++ interface for it.
  /// The GetStatus() method should be called after each method (including
  /// the constructor) to make sure that the operation was a success.

  class API {
  public:

    /// @brief Connect to a root level API object.
    ///
    /// Makes a connection to a root-level API object.
    /// @param [in] user Name of the user who is requesting access.
    ///             Will use ANONYMOUS_USER if this is not specified.
    /// @param [in] credentials Binary credentials object for this user.
    ///             This is used to verify the user and provide appropriate access.
    ///             Will use NO_CREDENTIALS if this is not specified.
    API(
      ::std::string user = ANONYMOUS_USER,
      ::std::vector<uint8_t> credentials = NO_CREDENTIALS);
    /// @brief Destroy the object, closing all API objects obtained from it.
    ~API();

    /// @brief Returns the status of the most-recent operation and clears error/warnings.
    ///
    /// This should be called after the construction of an API and after each method
    /// call that does not itself return an hrgls_Status to ensure that the operation
    /// completed.
    /// @return hrgls_Status returned by the most-recent operation on the wrapped
    ///         class, or other errors in case the API itself is broken.
    hrgls_Status GetStatus();

    /// @brief Return a vector of descriptions of available DataBlobSources.
    ::std::vector<DataBlobSourceDescription> GetAvailableDataBlobSources() const;

    /// @brief Return the current version.
    hrgls_VERSION GetVersion() const;

    /// @brief Return the current system time in UTC.
    struct timeval GetCurrentSystemTime() const;

    /// @brief Return the current verbosity.
    uint16_t GetVerbosity() const;

    /// @brief Set the verbosity.
    hrgls_Status SetVerbosity(uint16_t verbosity);

    /// @brief Callback handler type declaration for returning log messages.
    typedef void(*LogMessageCallback)(Message &message, void *userData);

    /// @brief Sets up a handler to be called as log messages come in when enabled.
    ///
    /// This method should be called before SetLogMessageStreamingState() is called to start streaming.
    /// Either this method or GetPendingLogMessages() should be used to retrieve messages; if this
    /// method is used, GetPendingLogMessages() will always return nothing.
    /// @param [in] callback Function pointer to the function that is to be called to
    ///        handle each message as it comes in when streaming is started.  Set to nullptr
    ///        to disable handling streaming messages.  The function must be able to handle
    ///        messages at full rate to avoid filling up memory as un-handled messages queue.
    /// @param [in] userData Pointer that will be passed into the callback handler along with
    ///        each message.  Often type-cast into a class or structure pointer to let the
    ///        handler know what it should do with each message.
    /// @return hrgls_STATUS_OKAY on success, a specific error code on failure.
    ///         GetStatus() should not be called after this method, since it is returned here.
    hrgls_Status SetLogMessageCallback(LogMessageCallback callback,
        void *userData = nullptr);

    /// @brief Turns delivery of log messages on or off.
    ///
    /// The API is not initially sending messages.  Call this
    /// function with true to turn on delivery.
    /// @param [in] running Set to true to start streaming, false to stop streaming.
    /// @return hrgls_STATUS_OKAY on success, a specific error code on failure.
    ///         GetStatus() should not be called after this method, since it is returned here.
    hrgls_Status SetLogMessageStreamingState(bool running = true);

    /// @brief Reads the next-available log message queued by streaming.
    ///
    /// This method should be called after SetLogMessageStreamingState() is called to start streaming.
    /// Either this method or SetLogMessageCallback() should be used to retrieve messages; if
    /// SetLogMessageCallback() method is used, GetPendingLogMessages() will always return nothing.
    /// @param maxNum Maximum number of messages to return, default is 0 for unlimited.
    /// @return Retrieves all currently available queued messages or an empty vector
    ///         if none are available.
    ::std::vector<Message> GetPendingLogMessages(size_t maxNum = 0);

    /// @brief Sets the range of message levels to be returned.
    ///
    /// This method filters log messages so that only those of sufficient urgency
    /// are returned.  It should be called before streaming is enabled.
    /// @param level Minimum level to be returned, defaults to hrgls_MESSAGE_MINIMUM_INFO.
    /// @return hrgls_STATUS_OKAY on success, a specific error code on failure.
    ///         GetStatus() should not be called after this method, since it is returned here.
    hrgls_Status SetLogMessageMinimumLevel(hrgls_MessageLevel level);

    /// @brief Private class declared for definition and use by the API implementation.
    class API_private;

  protected:
    /// @brief Protected method for use by the API implementation.
    hrgls_API GetRawAPI() const;

    // Share our protected information with classes that make use of us.
    /// @cond INTERNAL
    friend datablob::DataBlobSource;
    /// @endcond

  private:
    API_private *m_private = nullptr;
  };

  // hrgls::datablob namespace.
  namespace datablob {

  /// @brief Holds the data for a DataBlob, which comes from a DataBlobSource.
  ///
  /// This class stores and provides access to a DataBlob.  It wraps an
  /// hrgls_DataBlob C structure and provides a C++ interface for it.
  /// The GetStatus() method should be called after each method (including
  /// the constructor) to make sure that the operation was a success.
  ///
  /// The client code must call ReleaseData() at least once to avoid
  /// leaking the blob memory.  It must not attempt to access the pointer
  /// returned by Data() once ReleaseData() has been called.  Destroying the
  /// object does not release the underlying data.

    class DataBlob {
    public:
      /// @brief Default constructor.
      DataBlob();

      /// @brief Used internally to construct based on an hrgls_DataBlob.
      ///
      /// Makes a copy of the hrgls_DataBlob and destroys the copy during the
      /// deconstruction.  Does not call ReleaseData() upon destruction -- the
      /// client is responsible for doing this.
      /// @param [in] blob hrgls_DataBlob that is copied to construct this class.
      DataBlob(hrgls_DataBlob blob);
      /// @brief Destroy the blob object.  Does not release blob data.
      ~DataBlob();

      /// @brief Constructs by copying the DataBlob passed in.
      DataBlob(const DataBlob& copy);
      /// @brief Destroys any previous blob and copies from the specified blob.
      /// @return Reference to the DataBlob.
      DataBlob& operator = (const DataBlob& copy);

      /// @brief Returns the status of the most-recent operation and clears error/warnings.
      ///
      /// This should be called after the construction of an DataBlob and after each method
      /// call that does not itself return an hrgls_Status to ensure that the operation
      /// completed.
      /// @return hrgls_Status returned by the most-recent operation on the wrapped
      ///         class, or other errors in case the DataBlob itself is broken.
      hrgls_Status GetStatus();

      /// @brief Read the time of the blob in UTC.
      struct timeval Time() const;

      /// @brief Const pointer to the binary blob data.
      ///
      /// This pointer remains valid until the client code calls ReleaseData() on any
      /// of the copies of this DataBlob or on the underlying C struct.
      const uint8_t* Data() const;
      /// @brief Size of the binary DataBlob data.
      uint32_t Size() const;

      /// @brief Release the underlying DataBlob data associated with this DataBlob.
      ///
      /// DataBlobs are large enough that copying their data can cause significant
      /// performance issues, so the API passes pointers to data that is allocated
      /// when the DataBlob is received rather than copying the data.  Calling
      /// ReleaseData() passed through the API and does the appropriate deletion
      /// for this memory.  This must be done explicitly by the client code to avoid
      /// leaking memory.  Once this has been done, the pointer returned by Data()
      /// becomes invalid and must not be accessed.
      void ReleaseData();

      /// @brief Used internally to get access to the harnessed C struct.
      hrgls_DataBlob const RawDataBlob() const;

      /// @brief Private class declared for definition and use by the API implementation.
      class DataBlob_private;

    private:
      DataBlob_private* m_private = nullptr;
    };

    /// @brief Callback handler type declaration for returning DataBlobs from a DataBlobSource.
    ///
    /// The callback handler must call ReleaseData() on each blob it receives to avoid
    /// leaking memory.  It can queue blobs for processing by other threads, but then these
    /// threads must call ReleaseData() on one of the copies of the blobs.
    typedef void (*StreamCallback)(DataBlob &blob, void *userData);

    /// @brief Holds the data and methods for controlling a DataBlobSource.
    ///
    /// This class controls and reports a DataBlobSource.  It wraps an
    /// hrgls_DataBlobSource C structure and provides a C++ interface for it.
    /// The GetStatus() method should be called after each method (including
    /// the constructor) to make sure that the operation was a success.
    ///
    /// The DataBlobSource class controls the sending of DataBlob stream.
    class DataBlobSource {
    public:

      /// @brief Creates a DataBlobSource object and specifies its characteristics and controls.
      ///
      /// The default stream starts with with streaming turned off.
      /// Call SetStreamCallback() to point to a function to handle incoming blobs before
      /// turning streaming on, or else call GetNextBlob() repeatedly after streaming has
      /// been turned on to retrieve the blobs.
      /// Call SetStreamingState() to begin getting blobs.
      /// @param [in] api API object that the DataBlobSource lives inside.
      /// @param [in] props Used to control the stream properties (rate, etc.)
      /// @param [in] source Entity name of the DataBlobSource (available by calling
      ///        hrgls::API::GetAvailableDataBlobSources(); defaults to any available DataBlobSource.
      DataBlobSource(
        API &api,
        StreamProperties &props,
        ::std::string source = ANY_DATABLOBSOURCE
      );

      /// @brief Destroys a DataBlobSource, also clears callback.
      ~DataBlobSource();

      /// @brief Returns the status of the most-recent operation and clears error/warnings.
      ///
      /// This should be called after the construction of a DataBlobSource and after each method
      /// call that does not itself return an hrgls_Status to ensure that the operation
      /// completed.
      /// @return hrgls_Status returned by the most-recent operation on the wrapped
      ///         class, or other errors in case the object itself is broken.
      hrgls_Status GetStatus();

      /// @brief Sets up a handler to be called as blobs come in once streaming.
      ///
      /// This method should be called before SetStreamingState() is called to start streaming.
      /// Either this method or GetNextBlob() should be used to retrieve blobs; if this
      /// method is used, GetNextBlob() will always return empty blobs.
      /// @param [in] callback Function pointer to the function that is to be called to
      ///        handle each blob as it comes in when streaming is started.  Set to nullptr
      ///        to disable handling streaming blobs.  The function must be able to handle
      ///        blobs at full rate to avoid filling up memory as un-handled blobs queue.
      ///        The callback handler must call ReleaseData() on each blob it receives to avoid
      ///        leaking memory.  It can queue blobs for processing by other threads, but then these
      ///        threads must call ReleaseData() on one of the copies of the blobs.
      /// @param [in] userData Pointer that will be passed into the callback handler along with
      ///        each blob.  Often type-cast into a class or structure pointer to let the
      ///        handler know what it should do with each blob.
      /// @return hrgls_STATUS_OKAY on success, a specific error code on failure.
      ///         GetStatus() should not be called after this method, since it is returned here.
      hrgls_Status SetStreamCallback(StreamCallback callback, void *userData = nullptr);

      /// @brief Turns streaming on or off.
      ///
      /// The stream is not initially sending blobs. Call this
      /// function with true to turn on streaming.
      /// @param [in] running Set to true to start streaming, false to stop streaming.
      /// @return hrgls_STATUS_OKAY on success, a specific error code on failure.
      ///         GetStatus() should not be called after this method, since it is returned here.
      hrgls_Status SetStreamingState(bool running = true);

      /// @brief Reads the next-available blob queued by streaming.
      ///
      /// This method should be called after SetStreamingState() is called to start streaming.
      /// Either this method or SetStreamCallback() should be used to retrieve blobs; if
      /// SetStreamCallback() method is used, GetNextBlob() will always return empty blobs.
      /// @param [in] timeout How long to wait for a new blob, default returns immediately
      ///         if no blob is available.
      /// @return Retrieves the next available queued blob on the stream, or a blob
      ///         with empty data if none is available.  The receiver must call ReleaseData()
      ///         on any non-empty blob when it is done with it to avoid leaking memory.
      DataBlob GetNextBlob(struct timeval timeout = {});

      /// @brief Get the description (including the name) about the DataBlobSource.
      ///
      /// This returns the information needed to refer to the DataBlobSource in
      /// low-level API functions, such as GetDetailedStatus() and SetDetailedStatus().
      /// @return Structure containing the name and other information about the
      ///         DataBlobSource on success, default-constructed structure on failure.
      DataBlobSourceDescription GetInfo();

      /// @brief Private class declared for definition and use by the API implementation.
      class DataBlobSource_private;

    private:
      DataBlobSource_private *m_private = nullptr;
    };

  } // End datablob namespace

} // End hrgls namespace
