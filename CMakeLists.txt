cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

if(NOT TARGET score_lib_base)
  include(ScoreExternalAddon)
endif()

if(NOT TARGET score_plugin_avnd)
  return()
endif()

if(EMSCRIPTEN OR MSVC)
    return()
endif()

if (NOT has_std_2b_flag)
 message(" -- HDF5 plugin requires C++23")
 return()
endif()

project(score_addon_datareader LANGUAGES CXX)
score_common_setup()

block()
  if(NOT OSSIA_USE_SYSTEM_LIBRARIES)
    set(HDF5_IS_PARALLEL 0)
    set(HDF5_ENABLE_NONSTANDARD_FEATURES 0)
    set(HDF5_EXTERNALLY_CONFIGURED 1)
    set(HDF5_INSTALL_NO_DEVELOPMENT 1)
    set(HDF5_ENABLE_EMBEDDED_LIBINFO 0)
    set(HDF5_NO_PACKAGES 1)
    set(HDF5_BUILD_TOOLS 0)
    set(HDF5_BUILD_TESTS 0)
    set(HDF5_BUILD_UTILS 0)
    set(HDF5_BUILD_EXAMPLES 0)
    set(HDF5_BUILD_STATIC_TOOLS 0)
#    set(HDF5_USE_ZLIB_NG 1)
    set(HDF5_USE_ZLIB_STATIC 0)
    set(HDF5_BUILD_PARALLEL_TOOLS 0)
    set(HDF5_ENABLE_PARALLEL 0)
    set(HDF5_ENABLE_THREADSAFE 1)
    set(ZLIB_USE_LOCALCONTENT 1)
    set(HDF5_ALLOW_EXTERNAL_SUPPORT NO)
    set(HDF5_BUILD_HL_LIB NO)
    set(BUILD_SHARED_LIBS 0)
    set(BUILD_STATIC_LIBS 1)
    if(WIN32)
        set(H5_HAVE_VASPRINTF 0 CACHE "" INTERNAL)
    endif()
    add_subdirectory(3rdparty/hdf5)
  endif()

  block()
    set(HIGHFIVE_EXAMPLES 0)
    set(HIGHFIVE_UNIT_TESTS 0)
    set(HIGHFIVE_BUILD_DOCS 0)

    if( OSSIA_USE_SYSTEM_LIBRARIES)
      set(HIGHFIVE_FIND_HDF5 1)
    else()
      set(HDF5_IS_PARALLEL 0)
      set(HIGHFIVE_FIND_HDF5 0)
    endif()
    add_subdirectory(3rdparty/HighFive)
  endblock()
endblock()

if(TARGET hdf5-static)
  set(ossia_hdf5_lib hdf5-static)
elseif(TARGET hdf5::hdf5)
  set(ossia_hdf5_lib hdf5::hdf5)
endif()
add_library(score_addon_datareader
    DataReader/CommandFactory.hpp
    DataReader/DropHDF5.hpp
    DataReader/DropHDF5.cpp
    DataReader/DropCSV.hpp
    DataReader/DropCSV.cpp

    score_addon_datareader.cpp
    score_addon_datareader.hpp
)
target_compile_options(score_addon_datareader PRIVATE -std=c++2b)
target_compile_features(score_addon_datareader PRIVATE cxx_std_23)
set_target_properties(score_addon_datareader PROPERTIES SCORE_CUSTOM_PCH 1)

score_generate_command_list_file(
  score_addon_datareader
  "DataReader/CommandFactory.hpp"
)
target_link_libraries(score_addon_datareader PUBLIC
    score_plugin_avnd
    ${ossia_hdf5_lib}
    HighFive::HighFive
)

target_include_directories(score_addon_datareader PUBLIC
    "${3RDPARTY_FOLDER}/csv2/include"
)
setup_score_plugin(score_addon_datareader)

add_library(score_addon_hdf5
    DataReader/HDF5Ports.hpp
    DataReader/HDF5Ports.cpp
)
avnd_score_plugin_init(
  BASE_TARGET score_addon_hdf5
)

avnd_score_plugin_add(
  BASE_TARGET score_addon_hdf5
  SOURCES
    DataReader/HDF5DataReader.hpp
    DataReader/HDF5DataReader.cpp
  TARGET HDF5_Reader1D
  MAIN_CLASS HDF5_Reader1D
  NAMESPACE DataReader
)

avnd_score_plugin_add(
  BASE_TARGET score_addon_hdf5
  SOURCES
    DataReader/HDF5Texture.hpp
    DataReader/HDF5Texture.cpp
  TARGET HDF5_TextureReader
  MAIN_CLASS HDF5_TextureReader
  NAMESPACE DataReader
)


target_link_libraries(score_addon_hdf5 PUBLIC
    HighFive::HighFive
    ${ossia_hdf5_lib}
)
avnd_score_plugin_finalize(
  BASE_TARGET score_addon_hdf5
  PLUGIN_VERSION 1
  PLUGIN_UUID "81d2b26d-e225-4a22-ad49-57d65b47b124"
)

