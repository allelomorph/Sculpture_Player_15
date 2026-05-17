cmake_minimum_required(VERSION 3.14)

if(NOT COMMAND set_strict_compile_options)
  include(SetStrictCompileOptions)
endif()

# TBD fmtlib is only needed when using compilers that don't support C++26
#   std::println
include(Getfmt)

# GTK4 seems to only support cmake via pkg-config, see:
#   - https://docs.gtk.org/gtk4/getting_started.html
# required packages, see:
#   - https://pkgs.org/search/?q=gtk4.pc
#   - https://pkgs.org/search/?q=gtkmm.h
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM4 gtkmm-4.0)
if(NOT GTKMM4_FOUND)
  message(FATAL_ERROR [[required package gtkmm-4.0 not found, look for package names:
   - Debian/Ubuntu: libgtkmm-4.0-dev
   - Fedora:        gtkmm4.0-devel
   - Arch:          gtkmm-4.0
]])
endif()
# gtk4 should be installed by installation of gtkmm4, verify anyway
pkg_check_modules(GTK4 gtk4)
if(NOT GTK4_FOUND)
  message(FATAL_ERROR [[required package gtk4 not found, look for package names:
    - Debian/Ubuntu: libgtk-4-dev
    - Fedora:        gtk4-devel
    - Arch:          gtk4
]])
endif()


add_library(src OBJECT
  ControlPanelWindow.cpp
  main.cpp
)
set_strict_compile_options(src)
# `--std=c++20` (not gnu++20)
set_target_properties(src PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_include_directories(src PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${GTKMM4_INCLUDE_DIRS}
)
target_link_libraries(src PRIVATE
  ${GTKMM4_LINK_LIBRARIES}
  fmt
)
