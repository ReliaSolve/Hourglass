# Hourglass

This repository contains the interface definition and example programs to implement an example
"hourglass" interface.  This type of interface has C++ at the lower layer, a thin
C dynamic library interface in the middle through which all things pass, and another
C++ interface at the upper layer.  This approach enables both the library implementation
and applications to use C++ while providing the benefits of a separately-build DLL
that works reliably in many environments.  This implementation also includes a
Python interface on top of the C++ interface that is built when SWIG is available.

Hourglass encapsulates experience from building hourglass interfaces for two internal
projects; one for a virtual reality interface and one for a security camera interface.
It also includes experience from the the design of VRPN, experience from the design
of a distributed VR library, and experience passed on from
the designer of the C API used at UNC for VR.  The design choices and reasons behind
them are documented near the end of this page, but first let's see how to build and
extend the library.

## Getting Started

**Environment:** To be able to compile both Python and C++ correctly on Linux, your shell
should not be set up to start in a conda environment.  Numpy must be available in the
system Python3 distribution to be able to build for Python.  SWIG must also be installed
to be able to build a Python library.

**Build:** Hourglass uses CMake to configure the builds (though other build
systems could be used).  On Ubuntu Linux, this can be done as follows

    sudo apt install cmake
    cd; mkdir src; cd src; git clone https://github.com/reliasolve/Hourglass
    cd; mkdir -p build/hourglass; cd build/hourglass
    cmake ../../src/hourglass
    make

Note that on Windows, Visual Studio puts the pxflpy.py file in the root of the build
directory by the associated library and other files like _pxflpy.pdb in the selected
build directory (like RelWithDebInfo or Release or Debug).  The pxflpy.py file must
be copied into the same location as the other files before importing.

**Documentation:** The primary documentation for the API is available in DOxygen, and
is generated by as part of the build process when DOxygen is available.  On Ubuntu
Linux, this can be generated as follows:
* `sudo apt install doxygen`
* When you build using CMake, it will build DOxygen by default.
* Open doc_doxygen/html/index.html with a web browser to view the documentation.

**Test:** The library can be built with a "NULL" implementation, which will enable applications
to be built and linked to the interface, and then run against an actual DLL implementation.

**Fork:** To use this to define an actual API interface, find and replace all instances of "hrgls" with
a prefix that matches the name of the project being implemented.  Then fill in the
locations marked with \@todo in the code and documentation and copy/paste to add new
definitions and functions as needed.

**Implement:** To implement a library using the API, make this repository
a submodule and then build a DLL according to the same recipe used for the
hourglass shared library here, except for replacing hrgls_null_implementation.cpp with
a different file that implements the actual interface.  You can copy this file
and rename it to start with. Doing this enables multiple implementation by different
vendors to share the same interface definition, and enables the interface to be
linked and tested using the "NULL" implementation separately from the hourglass.

## Client Programs

The C++ interface is the primary interface.  It is declared in
hrgls_api_defs.hpp (which is the file to look at for a description of the interface) but
a program that uses the interface should include hrgls_api.hpp in exactly one source file.
The C interface is defined in hrgls_api.h; it is very complicated due to the need to
support binary compatibility between different versions of DLLs and to support different
memory allocators within the API and within the application.  The Python interface is
automatically generated from the C++ interface (with custom wrappers to handle
callback-based data retrieval), so the C++ documentation can be used along with the
Python example programs.

### Running Python Programs

The Python library is installed for Python 3.5, so the Python example programs can be run as follows:

``python3 datablobsource.py``

### Compiling C and C++ programs against the installed library

Compiling the C and C++ programs consists of including the header files (which are installed
in /usr/local/include on Linux) and linking against the hrgls.so library (installed in /usr/local/lib).
The C++ programs use C++-11.  An example compilation for the datablobsource.cpp program follows:

``c++ -std=c++11 datablobsource.cpp -lhrgls -o datablobsource``

An example compilation for the datablobsource.c program follows:

``cc datablobsource.c -lhrgls -o datablobsource``

Once built, these programs can be run against a different DLL than the included NULL
implementation without changes to other than the DLL load path.  This allows the
interface library to be distributed to developers so they can compile and link their
applications, which can then be used with any vendor's implementation.

## Design

In an "hourglass API", there is a C++ API at the "top" end (used by client code) and at the
"bottom" end (where the functions are actually implemented) with an "extern C" api in the middle.
This complicated structure is designed to meet the needs of an extensible API usable across a wide
range of platforms.

### Decisions

Hourglass-specific design decisions:

* Because the only well-defined library interface is the "extern C" interface, the dynamic library
that implements the interface must provide this.
* Because functions loaded as "extern C" from DLLs do not have parameters encoded as part of their name,
it is not possible to have two functions with the same name.  Making backwards-compatible
extentions possible thus requires passing opaque structures to its functions so that the structures
themselves can be changed in the future, enabling parameter counts to change.
* Because debug and release memory management on some architectures are incompatible, memory
allocation and deletion must be done on the same side of the interface (either both on the
client side or both on the library side), requiring the callback-based deletion of large data blocks
and the copying of other data.  This also requires providing constructor- and destructor-like
functions for each of the parameters that may change in the future.
* To provide an easier-to-use C++ API (which can be extended by adding parameters) while
ensuring that all allocations and memory layouts are done consistently, the client-side C++
implementation is a header-only implementation so that it is recompiled each time the client
application is rebuilt.
* To provide an easier-to-implement C++ interface, all of the details of converting parameters
and copying them from C to C++ is wrapped internally by hrgls_internal_wrap.cpp, so that the
developer only needs to implement a single, C++ API; an example of that API is in
hrgls_null_implementation.cpp, which should be copied and modified to implement
the library for each vendor.
* To enable thread-safe behavior, all of the header-wrapped C++ functions maintain a map of
status values with an entry for each thread.  This makes it so that method calls from one
thread do not change the status seen by another thread.  Also, all of the C-layer implementations
keep a map of data values associated with each count type per thread so that calls from
one thread do not replace values from another thread while it is still using them.
The underlying implementation must still be thread safe.

SWIG-based decisions:

* To provide atomic assignment of return codes with methods, it is tempting to make
all C++ methods return the error code from the C methods that they call.  This avoids
a case where one thread calls a getter (for example) to retrieve a value and then another
thread checks the return code with a second call.  Unfortunately, this breaks the Python
wrapping because it requires returning the value by reference or pointer in the parameters.
This could be worked around by returning both the value and the status as a std::pair,
but this is clunky for both C++ and Python use.  Hourglass works around this by keeping
track of the status internally and providing a GetStatus() call that can be used to
report the status of the most-recent call.  This also allows client code to determine
the status of object construction by calling it on the object right after it was
constructed.
* SWIG does not know how to automatically generate wrappers around combinations
of standard template classes and enumerations, but it is able to generate them
for templates and base classes.  To enable this automatic generation to work, the various
integral types within Hourglass are type-defined to integers with \#define used
to construct their values.  Enumerations would be a more type-safe way to do
this for the C and C++ interfaces, but it does not extend easily through SWIG.
* Callback-based reading of large data blocks is handled by custom code implemented
in the hrgls.i SWIG configuration file.  To support this happening in another thread from
the main thread, CMakeLists.txt includes a specific -threads argument to SWIG and the
implementation explicitly turns on and off the Python global interpreter lock (GIL).

General library design decisions:

* The library should never cause the program calling it to exit, even
for catastrophic failures.  It can return error codes but must always
pass control back to the caller to deal with them.  This is particularly
challenging when dealing with C++ exceptions and signals (like segmentation
faults).  Hourglass does not deal with signal handling, but it does deal with
C++ exceptions by wrapping all calls inside the internal C API with try..catch
so that implementation exceptions are not passed back to the caller.
It does not stop out-of-memory exceptions generated by the C++ runtime in the top of the
hourglass when a new object is constructed.
* Place a distinguishing prefix on EVERYTHING THAT CAN BE SEEN outside
the library.  This includes such things as global file variables,
which are not used by the user code, but certainly can cause name
collisions.  The Hourglass prefix is *hrgls_*.  Everything that does not
have this prefix should be either local to a routine or declared static.
This includes type definitions in the header (.h) files.  For the C++
layers, the *hrgls* namespace is used for classes and definitions.  Where
these classes use the underlying C types and definitions, the hrgls_
prefix shows through.
* Provide a function to get the version of the library.  This should
include at least major version (which changes when not all previous code will
continue to work with the new version) and minor version, but may also include
additional levels.
* All routines should return some value that specifies whether they
worked correctly or not.  It is good for these to be uniform
across the library as much as possible.  In Hourglass, every
routine returns an hrgls_Status value.  Note that some routines seem like they cannot
fail, but then later they are changed to do something that can
fail, so it is worth your while to have them return always with
success and have higher routines check the return value so that
when it later changes the other code will be prepared.
* To provide a human-readible error message for the return values,
a function such as hrgls_ErrorMessage() should be provided that converts
the value into a string.  This is useful to application that want to let
the user know what went wrong.  All internally-defined status returns should have
an entry in this function (OKAY, warnings, and errors).
* To support diagnostics and debugging, provide a routine that allows the
user to choose whether messages are printed by the library routines when errors
are detected. Higher levels put the library into "verbose"
mode, where it will print a message every time it does something.
That way, the user can get a trace of what happened which will aid
in debugging.  For Hourglass, hrgls_APIGetVerbosity and hrgls_APISetVerbosity
do this.
* Name things consistently within the library.  All type definitions
should have the same format (in Hourglass, all in capitals except for the
prefix) and all routine names should have the same format (Hourglass
uses camelCase).
* Limit unexpected side-effects of routines.  Ideally, all
of the information passed to or from the routines should be in
the parameters and return values.  Having global variables that
are set to communicate with the library causes confusion.  Within Hourglass,
the hrgls_set_verbosity() function breaks this rule.
* Wherever possible, limit the external exposure of dependencies on
other libraries.  The most problematic such exposure is in the header
files that must be included by the client code.  This can often be
handled by forward declaring types in the headers and including the
sub-library headers in the C/C++ files.  The next level is to not
require the client to link to the sub-libraries by using static
library linkage for other libraries that are used internally.
* For libraries that are designed to work across architectures,
the basic C types have the problem that they can vary in size.  This
means that the various specific-sized types such as uint23_t should be
used where possible to maintain the same ranges across all builds.

### Implementation details

The StreamProperties has two different implementations, one at the upper layer
inside hrgls_api.hpp where it constructs a C object that can be used to
pass data to the C API and once in the lower layer inside hrgls_internal_wrap.cpp
where it is simply a container format.  The lower-level one is needed to pass as
a parameter to constructors.

The code that handles Messages and DataBlobs is handled differently: a separate
_impl.hpp file is created for each of these and included in both the upper
hourglass (hrgls_api.hpp) and non-exported in the lower half
(hrgls_internal_wrap.cpp).  Each of these wraps the C implementation,
which is itself implemented directly in the hrgls_internal_wrap.cpp file
without calling methods on the C++ objects.  Messages and DataBlobs are only
passed back up to the upper level, never passed to the lower level, so they
don't need a separate implementation there.

## Extending

This section describes how to add parameters and functions to the API in ways that
do not break backwards compatibility.  The description above provided an overview of
the system, these provide specific examples.
**Note** that modifying the interface will require all vendors to implement the new C++
features added before they will be able to build the new version.

@todo The README.md file should be replaced with a description of the actual API rather
than the example API once it has been modified.

### Adding a new function to a DataBlobSource

As an example of adding a new method to a DataBlobSource, consider adding a Refresh() function
on the DataBlobSource class.  Adding it requires the following:
* Add hrgls_DataBlobSourceRefresh() to hrgls_api.h. This function requires no
parameters besides the stream itself.  If the function requires parameters and the set of parameters
might change with different versions, then a structure should be defined and passed in (see
hrgls_DataBlobSourceCreateParams for an example).  This is because the function parameters cannot be
changed in a dynamic library once they are added without breaking backwards compatibility.
* Declare Refresh() to the DataBlobSource object in hrgls_api_defs.hpp.
* Implement Refresh() in hrgls_api.hpp, having it check for the stream object and then call
hrgls_DataBlobSourceRefresh() and return its result.
* Implement hrgls_DataBlobSourceRefresh() in hrgls_internal_wrap.cpp, having it check
for the stream object and then call Refresh() and return its result.
* Implement Refresh() in hrgls_null_implementation (and in any actual implementations that use
the interface).

### Adding a new parameter to the API constructor

The opaque structure hrgls_APICreateParams_ declared in hrgls_api.h contains the parameters passed to
hrgls_APICreate().  To maintain backwards compatibility, adding a parameter requires performing
the following steps:
* **Do not** add another parameter to the hrgls_APICreate() function directly, because this will
break backwards compatibility in the sense that the new dynamic library will be incompatible with the old
versions.
* Add new functions to get and set the new parameter; if the parameter is called Bob and is
a 32-bit signed integer, declare the following two functions in hrgls_api.h (note the use of
a specific type rather than the C type integer which may differ between platforms):
  * hrgls_APICreateParametersGetBob(hrgls_APICreateParams params, int32_t *returnBob);
  * hrgls_APICreateParametersSetBob(hrgls_APICreateParams params, int32_t bob);
* Add a new entry in struct hrgls_APICreateParams_ in hrgls_internal_wrap.cpp and
modify the hrgls_APICreateParametersCreate() definition to set a default value for this entry.
The default value is needed because older code running against the new dynamic library will
not set a value for this new parameter so the create function must pick one for them.
* (If the entry needs to be cleaned up (freeing memory), modify hrgls_APICreateParametersDestroy()
in the same file to do any required cleanup; this is not needed for an integer).
* Implement hrgls_APICreateParametersGetBob() and hrgls_APICreateParametersSetBob() in
hrgls_internal_wrap.cpp by having them get and set this value.  (For arrays and other complicated
elements, see the other structures defined in hrgls_api.h.)
* Either add another optional parameter to the API() constructor in hrgls_api_defs.hpp or
else make a different constructor with a new set of parameters.  (Unlike the C API,
the C++ API can be modified in ways that are backwards compatible because it is header-only
compiled.  Do not remove an existing constructor or method, which will break existing code.)
* Modify hrgls_APICreate() in hrgls_internal_wrap.cpp to get the new parameter and pass it into
the modified C++ constructor.
* Modify or add the API() constructor in hrgls_api.hpp so that it sets the new parameter
by calling hrgls_APICreateParametersSetBob() before it calls hrgls_APICreate().
* Implement the modified or new constructor in hrgls_null_implementation.cpp so that
it produces the desired effect.
* Modify or write a new test program to verify that this parameter is set as expected.

### Adding a new object type

When adding a new object type, either as a new type of object or a new object that can be used
by existing objects or adding a new basic entity type, follow the example from an existing
object.  Remember to provide test and example programs for the new object.

If the new object passes a vector of types not yet encountered, the hrgls.i file may need to
be extended to describe the new type so that it can be properly wrapped in Python.
