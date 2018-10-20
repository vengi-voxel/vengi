# Patrick Wieschollek, <mail@patwie.com>
# FindTENSORFLOW.cmake
# https://github.com/PatWie/tensorflow-cmake/blob/master/cmake/modules/FindTensorFlow.cmake
# -------------
#
# Find TensorFlow library and includes
#
# Automatically set variables have prefix "TensorFlow",
# while variables you need to specify have prefix "TENSORFLOW"
# This module will set the following variables in your project:
#
# ``TensorFlow_VERSION``
#   exact TensorFlow version obtained from runtime
# ``TensorFlow_ABI``
#   ABI specification of TensorFlow library obtained from runtime
# ``TensorFlow_INCLUDE_DIR``
#   where to find tensorflow header files obtained from runtime
# ``TensorFlow_LIBRARY``
#   the libraries to link against to use TENSORFLOW obtained from runtime
# ``TensorFlow_FOUND TRUE``
#   If false, do not try to use TENSORFLOW.
#
#  for some examples, you will need to specify on of the following paths
# ``TensorFlow_SOURCE_DIR``
#   Path to source of TensorFlow, when env-var 'TENSORFLOW_SOURCE_DIR' is set and path exists
# ``TensorFlow_C_LIBRARY``
#   Path to libtensorflow_cc.so (require env-var 'TENSORFLOW_BUILD_DIR')
#
#
# USAGE
# ------
# add "list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}../../path/to/this/file)" to your project
#
# "add_tensorflow_gpu_operation" is a macro to compile a custom operation
#
# add_tensorflow_gpu_operation("<op-name>") expects the following files to exists:
#   - kernels/<op-name>_kernel.cc
#   - kernels/<op-name>_kernel_gpu.cu.cc (kernels/<op-name>_kernel.cu is supported as well)
#   - kernels/<op-name>_op.cc
#   - kernels/<op-name>_op.h
#   - ops/<op-name>.cc

if(APPLE)
  message(WARNING "This FindTensorflow.cmake is not tested on APPLE\n"
                  "Please report if this works\n"
                  "https://github.com/PatWie/tensorflow-cmake")
endif(APPLE)

if(WIN32)
  message(WARNING "This FindTensorflow.cmake is not tested on WIN32\n"
                  "Please report if this works\n"
                  "https://github.com/PatWie/tensorflow-cmake")
endif(WIN32)

set(PYTHON_EXECUTABLE "python3" CACHE STRING "specify the python version TensorFlow is installed on.")

if(TensorFlow_FOUND)
  # reuse cached variables
  message(STATUS "Reuse cached information from TensorFlow ${TensorFlow_VERSION} ")
else()
  message(STATUS "Detecting TensorFlow using ${PYTHON_EXECUTABLE}"
          " (use -DPYTHON_EXECUTABLE=... otherwise)")
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import tensorflow as tf; print(tf.__version__); print(tf.__cxx11_abi_flag__); print(tf.sysconfig.get_include()); print(tf.sysconfig.get_lib() + '/libtensorflow_framework.so')"
    OUTPUT_VARIABLE TF_INFORMATION_STRING
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE retcode)

  if(NOT "${retcode}" STREQUAL "0")
    message(FATAL_ERROR "Detecting TensorFlow info - failed  \n Did you installed TensorFlow?")
  else()
    message(STATUS "Detecting TensorFlow info - done")
  endif()

  string(REPLACE "\n" ";" TF_INFORMATION_LIST ${TF_INFORMATION_STRING})
  list(GET TF_INFORMATION_LIST 0 TF_DETECTED_VERSION)
  list(GET TF_INFORMATION_LIST 1 TF_DETECTED_ABI)
  list(GET TF_INFORMATION_LIST 2 TF_DETECTED_INCLUDE_DIR)
  list(GET TF_INFORMATION_LIST 3 TF_DETECTED_LIBRARY)

  # set(TF_DETECTED_VERSION 1.8)

  set(_packageName "TF")
  if (DEFINED TF_DETECTED_VERSION)
      string (REGEX MATCHALL "[0-9]+" _versionComponents "${TF_DETECTED_VERSION}")
      list (LENGTH _versionComponents _len)
      if (${_len} GREATER 0)
          list(GET _versionComponents 0 TF_DETECTED_VERSION_MAJOR)
      endif()
      if (${_len} GREATER 1)
          list(GET _versionComponents 1 TF_DETECTED_VERSION_MINOR)
      endif()
      if (${_len} GREATER 2)
          list(GET _versionComponents 2 TF_DETECTED_VERSION_PATCH)
      endif()
      if (${_len} GREATER 3)
          list(GET _versionComponents 3 TF_DETECTED_VERSION_TWEAK)
      endif()
      set (TF_DETECTED_VERSION_COUNT ${_len})
  else()
      set (TF_DETECTED_VERSION_COUNT 0)
  endif()


  # -- prevent pre 1.9 versions
  # Note: TensorFlow 1.7 supported custom ops and all header files.
  # TensorFlow 1.8 broke that promise and 1.9, 1.10 are fine again.
  # This cmake-file is only tested against 1.9+.
  if("${TF_DETECTED_VERSION}" VERSION_LESS "1.9")
    message(FATAL_ERROR "Your installed TensorFlow version ${TF_DETECTED_VERSION} is too old.")
  endif()

  if(TF_FIND_VERSION_EXACT)
    # User requested exact match of TensorFlow.
    # TensorFlow release cycles are currently just depending on (major, minor)
    # But we test against both.
    set(_TensorFlow_TEST_VERSIONS
        "${TF_FIND_VERSION_MAJOR}.${TF_FIND_VERSION_MINOR}.${TF_FIND_VERSION_PATCH}"
        "${TF_FIND_VERSION_MAJOR}.${TF_FIND_VERSION_MINOR}")
  else(TF_FIND_VERSION_EXACT)
    # User requested not an exact TensorFlow version.
    # However, only TensorFlow versions 1.9, 1.10 support all header files
    # for custom ops.
    set(_TensorFlow_KNOWN_VERSIONS ${TensorFlow_ADDITIONAL_VERSIONS}
        "1.9" "1.9.0" "1.10" "1.10.0" "1.11" "1.11.0")
    set(_TensorFlow_TEST_VERSIONS)

    if(TF_FIND_VERSION)
        set(_TF_FIND_VERSION_SHORT "${TF_FIND_VERSION_MAJOR}.${TF_FIND_VERSION_MINOR}")
        # Select acceptable versions.
        foreach(version ${_TensorFlow_KNOWN_VERSIONS})
          if(NOT "${version}" VERSION_LESS "${TF_FIND_VERSION}")
            # This version is high enough.
            list(APPEND _TensorFlow_TEST_VERSIONS "${version}")
          endif()
        endforeach(version)
      else(TF_FIND_VERSION)
        # Any version is acceptable.
        set(_TensorFlow_TEST_VERSIONS "${_TensorFlow_KNOWN_VERSIONS}")
      endif(TF_FIND_VERSION)
  endif()

  # test all given versions
  set(TensorFlow_FOUND FALSE)
  FOREACH(_TensorFlow_VER ${_TensorFlow_TEST_VERSIONS})
    if("${TF_DETECTED_VERSION_MAJOR}.${TF_DETECTED_VERSION_MINOR}" STREQUAL "${_TensorFlow_VER}")
      # found appropriate version
      set(TensorFlow_VERSION ${TF_DETECTED_VERSION})
      set(TensorFlow_ABI ${TF_DETECTED_ABI})
      set(TensorFlow_INCLUDE_DIR ${TF_DETECTED_INCLUDE_DIR})
      set(TensorFlow_LIBRARY ${TF_DETECTED_LIBRARY})
      set(TensorFlow_FOUND TRUE)
      message(STATUS "Found TensorFlow: (found appropriate version \"${TensorFlow_VERSION}\")")
      message(STATUS "TensorFlow-ABI is ${TensorFlow_ABI}")
      message(STATUS "TensorFlow-INCLUDE_DIR is ${TensorFlow_INCLUDE_DIR}")
      message(STATUS "TensorFlow-LIBRARY is ${TensorFlow_LIBRARY}")

      add_definitions("-DTENSORFLOW_ABI=${TensorFlow_ABI}")
      add_definitions("-DTENSORFLOW_VERSION=${TensorFlow_VERSION}")
      break()
    endif()
  ENDFOREACH(_TensorFlow_VER)

  if(NOT TensorFlow_FOUND)
  message(FATAL_ERROR "Your installed TensorFlow version ${TF_DETECTED_VERSION_MAJOR}.${TF_DETECTED_VERSION_MINOR} is not supported\n"
                      "We tested against ${_TensorFlow_TEST_VERSIONS}")
  endif(NOT TensorFlow_FOUND)

endif()

find_library(TensorFlow_C_LIBRARY
  NAMES libtensorflow_cc.so
  PATHS $ENV{TENSORFLOW_BUILD_DIR}
  DOC "TensorFlow CC library." )

if(TensorFlow_C_LIBRARY)
  message(STATUS "TensorFlow-CC-LIBRARY is ${TensorFlow_C_LIBRARY}")
else()
  message(STATUS "No TensorFlow-CC-LIBRARY detected")
endif()

find_path(TensorFlow_SOURCE_DIR
        NAMES
        tensorflow/c
        tensorflow/cc
        tensorflow/core
        tensorflow/core/framework
        tensorflow/core/platform
        tensorflow/python
        third_party
        PATHS $ENV{TENSORFLOW_SOURCE_DIR})

if(TensorFlow_SOURCE_DIR)
  message(STATUS "TensorFlow-SOURCE-DIRECTORY is ${TensorFlow_SOURCE_DIR}")
else()
  message(STATUS "No TensorFlow source repository detected")
endif()

macro(TensorFlow_REQUIRE_C_LIBRARY)
  if(TensorFlow_C_LIBRARY)
  else()
    message(FATAL_ERROR "Project requires libtensorflow_cc.so, please specify the path in ENV-VAR 'TENSORFLOW_BUILD_DIR'")
  endif()
endmacro()

macro(TensorFlow_REQUIRE_SOURCE)
  if(TensorFlow_SOURCE_DIR)
  else()
    message(FATAL_ERROR "Project requires TensorFlow source directory, please specify the path in ENV-VAR 'TENSORFLOW_SOURCE_DIR'")
  endif()
endmacro()

macro(add_tensorflow_cpu_operation op_name)
  # Compiles a CPU-only operation without invoking NVCC
  message(STATUS "will build custom TensorFlow operation \"${op_name}\" (CPU only)")

  add_library(${op_name}_op SHARED kernels/${op_name}_op.cc kernels/${op_name}_kernel.cc ops/${op_name}.cc )

  set_target_properties(${op_name}_op PROPERTIES PREFIX "")
  target_link_libraries(${op_name}_op LINK_PUBLIC ${TensorFlow_LIBRARY})
endmacro()


macro(add_tensorflow_gpu_operation op_name)
# Compiles a CPU + GPU operation with invoking NVCC
  message(STATUS "will build custom TensorFlow operation \"${op_name}\" (CPU+GPU)")

  set(kernel_file "")
  if(EXISTS "kernels/${op_name}_kernel.cu")
     message(WARNING "you should rename your file ${op_name}_kernel.cu to ${op_name}_kernel_gpu.cu.cc")
     set(kernel_file kernels/${op_name}_kernel.cu)
  else()
    set_source_files_properties(kernels/${op_name}_kernel_gpu.cu.cc PROPERTIES CUDA_SOURCE_PROPERTY_FORMAT OBJ)
     set(kernel_file kernels/${op_name}_kernel_gpu.cu.cc)
  endif()

  cuda_add_library(${op_name}_op_cu SHARED ${kernel_file})
  set_target_properties(${op_name}_op_cu PROPERTIES PREFIX "")

  add_library(${op_name}_op SHARED kernels/${op_name}_op.cc kernels/${op_name}_kernel.cc ops/${op_name}.cc )

  set_target_properties(${op_name}_op PROPERTIES PREFIX "")
  set_target_properties(${op_name}_op PROPERTIES COMPILE_FLAGS "-DGOOGLE_CUDA")
  target_link_libraries(${op_name}_op LINK_PUBLIC ${op_name}_op_cu ${TensorFlow_LIBRARY})
endmacro()

# simplify TensorFlow dependencies
add_library(TensorFlow_DEP INTERFACE)
TARGET_INCLUDE_DIRECTORIES(TensorFlow_DEP INTERFACE ${TensorFlow_SOURCE_DIR})
TARGET_INCLUDE_DIRECTORIES(TensorFlow_DEP INTERFACE ${TensorFlow_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(TensorFlow_DEP INTERFACE -Wl,--allow-multiple-definition -Wl,--whole-archive ${TensorFlow_C_LIBRARY} -Wl,--no-whole-archive)
TARGET_LINK_LIBRARIES(TensorFlow_DEP INTERFACE -Wl,--allow-multiple-definition -Wl,--whole-archive ${TensorFlow_LIBRARY} -Wl,--no-whole-archive)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  TENSORFLOW
  FOUND_VAR TENSORFLOW_FOUND
  REQUIRED_VARS
    TensorFlow_LIBRARY
    TensorFlow_INCLUDE_DIR
  VERSION_VAR
    TensorFlow_VERSION
  )

mark_as_advanced(TF_INFORMATION_STRING TF_DETECTED_VERSION TF_DETECTED_VERSION_MAJOR TF_DETECTED_VERSION_MINOR TF_DETECTED_VERSION TF_DETECTED_ABI
                 TF_DETECTED_INCLUDE_DIR TF_DETECTED_LIBRARY
                 TensorFlow_C_LIBRARY TensorFlow_LIBRARY TensorFlow_SOURCE_DIR TensorFlow_INCLUDE_DIR TensorFlow_ABI)

SET(TensorFlow_INCLUDE_DIR ${TensorFlow_INCLUDE_DIR} CACHE PATH "path to tensorflow header files")
SET(TensorFlow_VERSION ${TensorFlow_VERSION} CACHE INTERNAL "The Python executable Version")
SET(TensorFlow_ABI ${TensorFlow_ABI} CACHE STRING "The Python executable Version")
SET(TensorFlow_LIBRARY ${TensorFlow_LIBRARY} CACHE PATH "The Python executable Version")
SET(TensorFlow_FOUND ${TensorFlow_FOUND} CACHE BOOL "The Python executable Version")
