cmake_minimum_required(VERSION 3.1.0)
project(Hourglass_interface)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#-----------------------------------------------------------------------------
if(WIN32)
    add_definitions(-DNOMINMAX)
endif(WIN32)

#-----------------------------------------------------------------------------
# Local CMake Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#-----------------------------------------------------------------------------
# Enable sorting projects within the solution on Visual Studio, to group
# Test and Example files together.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-----------------------------------------------------------------------------
# Export libraries along with our DLLs if we want to build shared
# Allow the developer to select if Dynamic or Static libraries are built

include (GenerateExportHeader)

#-----------------------------------------------------------------------------
# Checking for optional and required packages

find_package(SWIG)
find_package(PythonLibs)
find_package(Doxygen)

#-----------------------------------------------------------------------------
# Build options.
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_NULL_IMPLEMENTATION "Build NULL library implementation" ON)
if (SWIG_FOUND AND PYTHONLIBS_FOUND)
  option(BUILD_PYTHON "Generate Python library" ON)
endif()

if (DOXYGEN_FOUND)
    option(BUILD_DOXYGEN "Build DOxygen documentation" ON)
endif (DOXYGEN_FOUND)

if (BUILD_DOXYGEN)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # note the option ALL which allows to build the docs together with the application
    add_custom_target( doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
endif (BUILD_DOXYGEN)

#-----------------------------------------------------------------------------
# Set things up for optional parameters

if (BUILD_PYTHON)
    include(${SWIG_USE_FILE})
    include_directories(${PYTHON_INCLUDE_PATH}
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_INSTALL_PREFIX}/include
    )
    # -threads: Release the Python Global Interpreter lock inside C functions.
    #           This enables multiple threads to run at the same time but
    #           requires us to obtain the lock inside our functions that use
    #           Python or PyC objects.
    set(CMAKE_SWIG_FLAGS "-threads")
    set_source_files_properties(hrgls.i PROPERTIES CPLUSPLUS ON)
endif (BUILD_PYTHON)

#-----------------------------------------------------------------------------
# Set up build product locations

include(GNUInstallDirs)

# Win-specific: we want shared libs (dlls) in same dir as exe files.
if(WIN32)
	set(hrgls_SHARED_LIBRARY_DIR "${CMAKE_INSTALL_BINDIR}")
else()
	set(hrgls_SHARED_LIBRARY_DIR "${CMAKE_INSTALL_LIBDIR}")
endif()

set (hrgls_SOURCES hrgls_internal_wrap.cpp)
if(BUILD_NULL_IMPLEMENTATION)
  set (hrgls_SOURCES ${hrgls_SOURCES}
    hrgls_null_implementation.cpp
  )
endif(BUILD_NULL_IMPLEMENTATION)

#-----------------------------------------------------------------------------
# Build the library, perhaps with a NULL implementation.

set (hrgls_HEADERS
  hrgls_api.h
  hrgls_api_defs.hpp
  hrgls_api.hpp
  hrgls_DataBlob_impl.hpp
  hrgls_Message_impl.hpp
)

add_library(hrgls ${hrgls_SOURCES} ${hrgls_HEADERS})
set_target_properties(hrgls PROPERTIES LINKER_LANGUAGE CXX)
# Hide all symbols that are not explicitly exported from the library
set_target_properties(hrgls PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties(hrgls PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(hrgls PROPERTIES VISIBILITY_INLINES_HIDDEN 1)
target_include_directories(hrgls PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_INSTALL_PREFIX}/include
)
if(UNIX)
    target_link_libraries(hrgls PUBLIC pthread)
endif(UNIX)
set_target_properties(hrgls PROPERTIES PUBLIC_HEADER "${hrgls_HEADERS}")
generate_export_header(hrgls)

install(TARGETS hrgls EXPORT ${PROJECT_NAME}
  RUNTIME DESTINATION bin COMPONENT lib
  LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT lib
  ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT lib
  INCLUDES DESTINATION include
  PUBLIC_HEADER DESTINATION include
)
install(FILES ${PROJECT_BINARY_DIR}/hrgls_export.h
  DESTINATION include COMPONENT headers
)

if (BUILD_PYTHON)
    SWIG_ADD_MODULE(hrgls python hrgls.i)
    SWIG_LINK_LIBRARIES(hrgls hrgls)
    SWIG_LINK_LIBRARIES(hrgls stdc++)
endif (BUILD_PYTHON)

#-----------------------------------------------------------------------------
# Build examples if we've been asked to.

if(BUILD_EXAMPLES)
  set (EXAMPLES
    list_datablobsources
    datablobsource
    print_log_messages
  )
  foreach (BASE ${EXAMPLES})
    # C++ example
    set (APP ${BASE})
    add_executable (${APP} examples/${BASE}.cpp)
    set_target_properties(${APP} PROPERTIES FOLDER examples/C++)
    target_link_libraries(${APP} hrgls)
    install(TARGETS ${APP} EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION bin
    )

    # C example
    set (APP ${BASE}_C)
    add_executable (${APP} examples/${BASE}.c)
    set_target_properties(${APP} PROPERTIES FOLDER examples/C)
    target_link_libraries(${APP} hrgls)
    install(TARGETS ${APP} EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION bin
    )

    # Python example
    if (BUILD_PYTHON)
      install(FILES examples/${BASE}.py DESTINATION bin)
    endif (BUILD_PYTHON)
  endforeach (BASE)

endif(BUILD_EXAMPLES)

#-----------------------------------------------------------------------------
# Build tests if we've been asked to.

if(BUILD_TESTS)

  enable_testing()

  set (CPP_TESTS
    open_api
    open_datablob_api
    test_log_messages
  )
  foreach (BASE ${CPP_TESTS})
    set (APP ${BASE}_cpp)
    add_executable (${APP} tests_cpp/${BASE}.cpp)
    set_target_properties(${APP} PROPERTIES FOLDER tests/C++)
    target_link_libraries(${APP}
      hrgls
    )
    install(TARGETS ${APP} EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION bin
    )
    add_test(${APP} ${APP})
  endforeach (BASE)

  set (C_TESTS
    open_api
  )
  foreach (BASE ${C_TESTS})
    set (APP ${BASE}_c)
    add_executable (${APP} tests_c/${BASE}.c)
    set_target_properties(${APP} PROPERTIES FOLDER tests/C)
    target_link_libraries(${APP} hrgls)
    install(TARGETS ${APP} EXPORT ${PROJECT_NAME}
      RUNTIME DESTINATION bin
    )
    add_test(${APP} ${APP})
  endforeach (BASE)

endif(BUILD_TESTS)
