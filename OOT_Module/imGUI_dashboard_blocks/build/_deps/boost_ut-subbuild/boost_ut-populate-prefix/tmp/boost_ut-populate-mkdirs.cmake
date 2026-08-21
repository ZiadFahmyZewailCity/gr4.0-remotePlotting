# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-src"
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-build"
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix"
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix/tmp"
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix/src/boost_ut-populate-stamp"
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix/src"
  "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix/src/boost_ut-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix/src/boost_ut-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/ziad/GR4/gr4.0-remotePlotting/flowgraph_blocks/dashboard_blocks/build/_deps/boost_ut-subbuild/boost_ut-populate-prefix/src/boost_ut-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
