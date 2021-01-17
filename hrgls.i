/* IMPORTANT: Be sure to link the library against the hrgls library in
 * the CMakeLists.txt file. */

%module hrgls

/** @todo This may not be needed on all architectures... */
#define SWIGWORDSIZE64 

/* Include required definition files */
%include "cpointer.i"
%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "std_pair.i"
%include "typemaps.i"

%{
/* Includes the headers inside the brackets to put it in the wrapper code */

/* In case we need it, defines a constant to say we are SWIG parsing. */
#define hrgls_SWIG_PARSING_NOW
#include "hrgls_export.h"
#include "hrgls_api.h"
#include "hrgls_api_defs.hpp"
#include "hrgls_api.hpp"
#include <map>

/* Map from DataBlobSource objects to Python callback/userdata pairs.
 * that is used to keep track of the objects that have been registered
 * as part of a DataBlob callback handler. */
static ::std::map<hrgls::datablob::DataBlobSource*, std::vector<PyObject*> >
  hrgls_Python_Render_Callback_Map;

/* Static C++ callback handler method to call a Python callback
 * function and pass it the blob and the userdata that was registered. */
static void hrgls_Python_DataBlob_Callback(hrgls::datablob::DataBlob &blob, void *userData)
{
  /* Return if we don't have anything to do. */
  if (userData == nullptr) {
    return;
  }

  /* Make sure that we have the global interpreter lock for our thread
   * so long as we are using PyC or Python objects. */
  SWIG_PYTHON_THREAD_BEGIN_BLOCK;

  /* userData was set to point to a vector of PyObject pointers, the first
   * of which is the Python function to call and the second of which is the
   * userData parameter that was set when the callback handler was registered. */
  std::vector<PyObject*> *params = static_cast<std::vector<PyObject*> *>(userData);

  /* Make an argument list, where the first argument is the blob and
   * the second is the userData argument passed in when the callback
   * handler was registered. */
  PyObject *arglist = PyTuple_New(2);
  PyTuple_SET_ITEM(arglist, 0, 
    SWIG_NewPointerObj(SWIG_as_voidptr(&blob), SWIGTYPE_p_hrgls__datablob__DataBlob, 0));
  PyTuple_SET_ITEM(arglist, 1, (*params)[1]);
  /* The tuple "steals" a reference, so we need to bump here so it doesn't get
   * deleted when we decrement the reference on arglist below. */
  Py_XINCREF((*params)[1]);

  /* Call the Python function, which is the first entry in the vector. */
  PyObject *result = PyEval_CallObject((*params)[0], arglist);

  /* Remove references to objects that we created so they will not leak memory. */
  Py_DECREF(arglist);
  Py_XDECREF(result);

  /* Release the global interpreter lock. */
  SWIG_PYTHON_THREAD_END_BLOCK;
}

/* Map from API objects to Python callback/userdata pairs.
 * that is used to keep track of the objects that have been registered
 * as part of a LogMessage callback handler. */
static ::std::map<hrgls::API*, std::vector<PyObject*> >
  hrgls_Python_Log_Callback_Map;

/* Static C++ callback handler method to call a Python callback
 * function and pass it the message and the userdata that was registered. */
static void hrgls_Python_Log_Callback(hrgls::Message &message, void *userData)
{
  /* Return if we don't have anything to do. */
  if (userData == nullptr) {
    return;
  }

  /* Make sure that we have the global interpreter lock for our thread
   * so long as we are using PyC or Python objects. */
  SWIG_PYTHON_THREAD_BEGIN_BLOCK;

  /* userData was set to point to a vector of PyObject pointers, the first
   * of which is the Python function to call and the second of which is the
   * userData parameter that was set when the callback handler was registered. */
  std::vector<PyObject*> *params = static_cast<std::vector<PyObject*> *>(userData);

  /* Make an argument list, where the first argument is the message and
   * the second is the userData argument passed in when the callback
   * handler was registered. */
  PyObject *arglist = PyTuple_New(2);
  PyTuple_SET_ITEM(arglist, 0, 
    SWIG_NewPointerObj(SWIG_as_voidptr(&message), SWIGTYPE_p_hrgls__Message, 0));
  PyTuple_SET_ITEM(arglist, 1, (*params)[1]);
  /* The tuple "steals" a reference, so we need to bump here so it doesn't get
   * deleted when we decrement the reference on arglist below. */
  Py_XINCREF((*params)[1]);

  /* Call the Python function, which is the first entry in the vector. */
  PyObject *result = PyEval_CallObject((*params)[0], arglist);

  /* Remove references to objects that we created so they will not leak memory. */
  Py_DECREF(arglist);
  Py_XDECREF(result);

  /* Release the global interpreter lock. */
  SWIG_PYTHON_THREAD_END_BLOCK;
}

%}

/* Parse the header files again outside of the brackets to generate wrappers */

/* Define the export constant to be empty */
/* In case we need it, defines a constant to say we are SWIG parsing. */
/** @todo Check if this works on Windows */
#define HRGLS_EXPORT
#define hrgls_SWIG_PARSING_NOW
%include "hrgls_api.h"
%include "hrgls_api_defs.hpp"

/* SWIG does not provide constants that are not fully expanded by the
 * preprocessor, so we need to add constant values for these.  First
 * we need to undefine them so that they can be replaced by the
 * constant values.
 * WARNING: These must be changed to match the values in hrgls_api.h if
 *          those values are modified. */
#undef hrgls_MESSAGE_MINIMUM_INFO
%constant int hrgls_MESSAGE_MINIMUM_INFO = -2147483647 - 1;
#undef hrgls_MESSAGE_MINIMUM_ERROR
%constant int hrgls_MESSAGE_MINIMUM_ERROR = 2147483647 / 3;
#undef hrgls_MESSAGE_MINIMUM_CRITICAL_ERROR
%constant int hrgls_MESSAGE_MINIMUM_CRITICAL_ERROR = hrgls_MESSAGE_MINIMUM_ERROR * 2;

/* Define templates that we can use to to convert from a list of strings in Python to
 * a vector of strings in C++, and to convert vectors of other classes. */

%template(PairStringStatus) std::pair< std::string, hrgls_Status>;
%template(PairVectorStringStatus) std::pair< std::vector< std::string >, hrgls_Status>;
%template(StringVector) std::vector<std::string>;
%template(DataBlobSourceVector) std::vector<hrgls::DataBlobSourceDescription>;
%template(MessageVector) std::vector<hrgls::Message>;

/* Define structures that we need to access members of. */
struct timeval {
    long tv_sec;
    long tv_usec;
};

/*****************************************************************/
/* Wrap the DataBlob callback handler so that we can call it from Python.
 * When the SetStreamCallback method is called with (None, None), Swig
 * directly calls the C++ method with (nullptr, nullptr) and bypasses
 * this wrapped method. */

%extend hrgls::datablob::DataBlobSource {
  hrgls_Status SetStreamCallback(PyObject *callbackHandler, PyObject *userData)
  {
    /* @todo We would like to have reference counted the objects here so that
     * we make sure the passed-in Python objects do not go out of scope and
     * get deleted while the callback is still active.  However, when we
     * do that, we get segmentation violations when we call the decrement-
     * reference code on the userData object.  It is not clear why that is
     * happening, but to avoid leaking memory by only doing the increments
     * that code is commented out here. */

    /* See if there is already an entry in the map for this object.
     * If so, go ahead and release the earlier references and remove the
     * entry.  Also turn off the old callback handler. */
/*
    auto prev = hrgls_Python_Render_Callback_Map.find($self);
    if (prev != hrgls_Python_Render_Callback_Map.end()) {
        auto cb = prev->second[0];
        if ((cb != nullptr) && (cb != Py_None)) {
          Py_XDECREF(cb);
        }
        auto ud = prev->second[1];
        if ((ud != nullptr) && (ud != Py_None)) {
          Py_XDECREF(ud);
        }
        hrgls_Python_Render_Callback_Map.erase(prev);
        hrgls_Status ret =$self->SetStreamCallback(nullptr, nullptr);
        if (ret != hrgls_STATUS_OKAY) {
            return ret;
        }
    }
*/

    /* Keep the objects from being garbage collected while the callback is active. */
/*
    if ((callbackHandler != nullptr) && (callbackHandler != Py_None)) {
        Py_XINCREF(callbackHandler);
    }
    if ((userData != nullptr) && (userData != Py_None)) {
        Py_XINCREF(userData);
    }
*/

    /* Construct a new vector of objects, one for the handler and one for
     * the user data.  Add it to the map.  Pass a pointer to the vector
     * in the map as userdata to the C++ handler defined above. */
    std::vector<PyObject *> cbAndUd;
    cbAndUd.push_back(callbackHandler);
    cbAndUd.push_back(userData);
    hrgls_Python_Render_Callback_Map[$self] = cbAndUd;
    hrgls_Status ret =$self->SetStreamCallback(hrgls_Python_DataBlob_Callback,
      static_cast<void*>(&hrgls_Python_Render_Callback_Map[$self]));

    return ret;
  }
};

%nothread hrgls::datablob::DataBlobSource::SetStreamCallback;

/*****************************************************************/
/* Wrap the log callback handler so that we can call it from Python.
 * When the SetLogMessageCallback method is called with (None, None), Swig
 * directly calls the C++ method with (nullptr, nullptr) and bypasses
 * this wrapped method. */

%extend hrgls::API {
  hrgls_Status SetLogMessageCallback(PyObject *callbackHandler, PyObject *userData)
  {
    /* @todo We would like to have reference counted the objects here so that
     * we make sure the passed-in Python objects do not go out of scope and
     * get deleted while the callback is still active.  However, when we
     * do that, we get segmentation violations when we call the decrement-
     * reference code on the userData object.  It is not clear why that is
     * happening, but to avoid leaking memory by only doing the increments
     * that code is commented out here. */

    /* See if there is already an entry in the map for this object.
     * If so, go ahead and release the earlier references and remove the
     * entry.  Also turn off the old callback handler. */
/*
    auto prev = hrgls_Python_Log_Callback_Map.find($self);
    if (prev != hrgls_Python_Log_Callback_Map.end()) {
        auto cb = prev->second[0];
        if ((cb != nullptr) && (cb != Py_None)) {
          Py_XDECREF(cb);
        }
        auto ud = prev->second[1];
        if ((ud != nullptr) && (ud != Py_None)) {
          Py_XDECREF(ud);
        }
        hrgls_Python_Log_Callback_Map.erase(prev);
        hrgls_Status ret =$self->SetLogMessageCallback(nullptr, nullptr);
        if (ret != hrgls_STATUS_OKAY) {
            return ret;
        }
    }
*/

    /* Keep the objects from being garbage collected while the callback is active. */
/*
    if ((callbackHandler != nullptr) && (callbackHandler != Py_None)) {
        Py_XINCREF(callbackHandler);
    }
    if ((userData != nullptr) && (userData != Py_None)) {
        Py_XINCREF(userData);
    }
*/

    /* Construct a new vector of objects, one for the handler and one for
     * the user data.  Add it to the map.  Pass a pointer to the vector
     * in the map as userdata to the C++ handler defined above. */
    std::vector<PyObject *> cbAndUd;
    cbAndUd.push_back(callbackHandler);
    cbAndUd.push_back(userData);
    hrgls_Python_Log_Callback_Map[$self] = cbAndUd;
    hrgls_Status ret =$self->SetLogMessageCallback(hrgls_Python_Log_Callback,
      static_cast<void*>(&hrgls_Python_Log_Callback_Map[$self]));

    return ret;
  }
};

%nothread hrgls::API::SetLogMessageCallback;

