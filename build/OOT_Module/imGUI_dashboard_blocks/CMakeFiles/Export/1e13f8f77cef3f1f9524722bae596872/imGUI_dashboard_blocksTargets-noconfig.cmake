#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "imGUI_dashboard_blocks::imGUI_dashboard_blocks" for configuration ""
set_property(TARGET imGUI_dashboard_blocks::imGUI_dashboard_blocks APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(imGUI_dashboard_blocks::imGUI_dashboard_blocks PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libimGUI_dashboard_blocks.so"
  IMPORTED_SONAME_NOCONFIG "libimGUI_dashboard_blocks.so"
  )

list(APPEND _cmake_import_check_targets imGUI_dashboard_blocks::imGUI_dashboard_blocks )
list(APPEND _cmake_import_check_files_for_imGUI_dashboard_blocks::imGUI_dashboard_blocks "${_IMPORT_PREFIX}/lib/libimGUI_dashboard_blocks.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
