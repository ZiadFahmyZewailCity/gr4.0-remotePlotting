# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-src"
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-build"
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix"
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix/tmp"
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix/src/packageproject.cmake-populate-stamp"
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix/src"
  "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix/src/packageproject.cmake-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix/src/packageproject.cmake-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/ziad/GNURADIO_DEVELOPMENT/flowgraph_blocks/dashboard_blocks/build/_deps/packageproject.cmake-subbuild/packageproject.cmake-populate-prefix/src/packageproject.cmake-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
