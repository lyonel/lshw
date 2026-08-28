cmake_minimum_required(VERSION 3.0)

project(lshw)
set(VERSION "B.02.20")

find_package(Git)

if(EXISTS "${PROJECT_SOURCE_DIR}/.git" AND "${MAKE_RELEASE}" STREQUAL "")
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags
      OUTPUT_VARIABLE DESCRIBE_TAG
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "B.[0-9]+.[0-9]+[-][0-9]+" VERSION ${DESCRIBE_TAG})
    string(REPLACE "-" "." VERSION ${VERSION})
  endif()
endif()

message("-- lshw: ${VERSION}")

option(GUI "Enable GUI application (${PROJECT_NAME}-gtk)" ON)
option(HWDATA "Install hwdata files" ON)
option(SQLITE "Enable SQLite support" OFF)
option(ZLIB "Enable zlib support" OFF)
option(NOLOGO "Don't install vendor logos" OFF)
option(STATIC "Do a static (will disable other features)" OFF)
option(POLICYKIT "Install PolicyKit file and pfexec wrapper" OFF)

include(GNUInstallDirs)
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE STRING "Install prefix")
set(DATADIR "${CMAKE_INSTALL_FULL_DATADIR}")
set(PROJECT_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
set(SBINDIR "${CMAKE_INSTALL_FULL_SBINDIR}")
set(MANDIR "${CMAKE_INSTALL_FULL_MANDIR}")
set(LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")

configure_file(
  "${PROJECT_SOURCE_DIR}/lshw.spec.in"
  "${PROJECT_BINARY_DIR}/lshw.spec")

add_subdirectory(src)
add_subdirectory(src/po)
add_subdirectory(src/gui)
