\page Using Using the API

\example datablobsource.c
\example datablobsource.cpp
\example datablobsource.py

\example list_datablobsources.c
\example list_datablobsources.cpp
\example list_datablobsources.py

\example print_log_messages.c
\example print_log_messages.cpp
\example print_log_messages.py

// List the test programs
\example open_api.c
\example open_api.cpp
\example open_datablob_api.cpp
\example test_log_messages.cpp


\page Using Using the example API

This page describes the example API and provides pointers to example code and also
code snippets to perform some desired actions.

@todo The using.md document should be updated to describe the actual
API once the conversion from the example API is complete.

# Top-level API

There is an API object defined in the hrgls:: namespace.  An object of this type is
required to construct all objects in the system that have internal implementations.
An example of opening an API object with and without specifying a user name and
credentials can be found in \ref open_api.cpp.

The API object has methods to get and set the library verbosity, get the version,
and handle asynchronous log messages coming from objects inside the system.  It also
has a method to get a list of the available DataBlobSource objects in the system.

# Error checking

All C API return a hrgls_Status value indicating whether a warning or error occurred during
their call.  Some methods for C++ API and DataBlobSource classes return hrgls_Status values;
others, including constructors, do not.  To find the status return values for constructors
and other methods, call the GetStatus() method on the object after the methods are called.

The hrgls_Status values are defined in \ref hrgls_api.h, with hrgls_STATUS_OKAY indicating
no warnings or errors.  Values above it and less than or equal to hrgls_STATUS_HIGHEST_WARNING
are warnings, and values above it are errors.  The helper function hrgls_ErrorMessage()
takes in an hrgls_Status and provides a string description of the status.

# DataBlobSource

There is an hrgls::datablob namespace, within which is the DataBlobSource class.  This class
emits "Binary Large OBjects" (BLOBs) of data at a regular interval.  This is like what a
streaming video or audio source might emit.  When the objects are very large, it is important
to minimize the copying from one layer of the Hourglass to the other, so pointers are passed
rather than the data.  To make this possible across all memory models, there is a ReleaseData()
method in the DataBlob that is passed through the system; it calls the appropriate release
function for the data memory.  To avoid leaking memory, the client must call ReleaseData()
separately from destroying the object; this enables the client to keep a reference to the
data in an internal buffer cache for as long as needed after returning from the function that
handles the incoming DataBlob.

The DataBlob class also includes a Time() method as an example of other, copyable, data that
can be part of such a class.

DataBlobSource includes a SetStreamingState() method to start and stop the streams of
data blobs and two approaches to getting the blobs themselves.  One is a polling approach,
where the application calls GetNextBlob() repeatedly to pull from an internal queue that is
filled by the source.  For a callback-based, multi-threaded approach, SetStreamCallback()
is used to define a function that will be called whenever a new DataBlob is available.

Both of these approaches are demonstrated in the example program.  C++: \ref datablobsource.cpp.
Python: \ref datablobsource.py.

# Log messages

The system also provides a way for status, warning, and error reports to be sent from
objects inside the implementation to the client through the API.  These can be generated
asynchronously by one or more internal threads.

Like DataBlobs, the streaming of these messages can be enabled and disabled by calling
SetLogMessageStreamingState().  Also, these messages can be retrieved on demand using
GetPendingLogMessages() or they can be returned asynchronously using SetLogMessageCallback().
Unlike DataBlobs, these messages are expected to be small enough to be copied on their way
through the system so there is not a need to release their data when the client is done with
them.

Both of these approaches are demonstrated in the example program.  C++: \ref print_log_messages.cpp.
Python: \ref print_log_messages.py.
