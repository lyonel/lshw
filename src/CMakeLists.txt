if(STATIC)
 set(ZLIB OFF)
 set(SQLITE OFF)
endif()

find_package(PkgConfig)

# SQLite support
if(SQLITE)
  pkg_check_modules(SQLITE3 sqlite3)
  if(SQLITE3_FOUND)
    message("-- Enabling SQLite support")
  else()
    message(FATAL_ERROR "SQLite not found, install lib or disable feature: -DSQLITE=OFF")
  endif()
else()
  message("-- SQLite support disabled")
endif()

# zlib support
if(ZLIB)
  find_program(GZIP gzip "Path to gzip application")
  if(NOT GZIP)
    message(FATAL_ERROR "gzip program not found, install gzip or disable zlib support: -DZLIB=OFF")
  endif()
  pkg_check_modules(Z zlib)
  if(Z_FOUND)
    message("-- Enabling zlib support")
  else()
    message(FATAL_ERROR "zlib not found, install lib or disable feature: -DZLIB=OFF")
  endif()
else()
  message("-- zlib support disabled")
endif()

if(ERROR)
  message(FATAL_ERROR "Configuration failed")
endif()

# Some special targets, compress, refresh_hwdata and release
add_custom_target(compressed
  COMMAND upx -9 -o lshw-compress lshw
  COMMENT "Creating upx compressed binary")
add_dependencies(compressed lshw)

add_custom_target(refresh_hwdata
  COMMAND wget -N http://pciids.sourceforge.net/pci.ids
  COMMAND wget -N http://www.linux-usb.org/usb.ids
  COMMAND wget -N http://standards-oui.ieee.org/oui/oui.txt
  COMMAND wget -O manuf.txt http://anonsvn.wireshark.org/wireshark/trunk/manuf
  COMMAND wget -N https://git.fedorahosted.org/cgit/hwdata.git/plain/pnp.ids
  COMMAND wget -N http://www-pc.uni-regensburg.de/hardware/TECHNIK/PCI_PNP/pnpid.txt
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
  COMMENT "Updating hwdata files from upstream location")

set(TARNAME ${PROJECT_NAME}-${VERSION})
add_custom_target(release
  COMMAND ${GIT_EXECUTABLE} archive --prefix=${TARNAME}/
    -o ${PROJECT_BINARY_DIR}/${TARNAME}.tar HEAD
  COMMAND mv ${PROJECT_BINARY_DIR}/lshw.spec ${PROJECT_SOURCE_DIR}/lshw.spec
  COMMAND tar --owner=0 --group=0
    --transform s,lshw.spec,${TARNAME}/lshw.spec,
    -rf ${PROJECT_BINARY_DIR}/${TARNAME}.tar lshw.spec
  COMMAND gzip ${PROJECT_BINARY_DIR}/${TARNAME}.tar
  COMMAND rm ${PROJECT_SOURCE_DIR}/lshw.spec
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMENT "Creating release tarball")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/core/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/core")

file(GLOB DATAFILES "pci.ids" "pnp.ids" "usb.ids" "manuf.txt" "oui.txt" "pnpid.txt")

file(GLOB SOURCES "core/*.cc")
add_library(core ${SOURCES})
add_executable(lshw lshw.cc)

if(STATIC)
  set_target_properties(lshw PROPERTIES LINK_FLAGS "-static" )
endif()

target_link_libraries(lshw ${SQLITE3_LIBRARIES} ${Z_LIBRARIES} core resolv)

if(NOT ZLIB)
  if(HWDATA)
    install(FILES ${DATAFILES} DESTINATION ${PROJECT_DATADIR})
  endif()
else()
  foreach(DATAFILE ${DATAFILES})
    get_filename_component(FILE ${DATAFILE} NAME)
    add_custom_command(
      OUTPUT ${FILE}.gz
      COMMAND ${GZIP} -c ${DATAFILE} > ${FILE}.gz)
    add_custom_target(${FILE} ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.gz)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILE}.gz DESTINATION ${PROJECT_DATADIR})
  endforeach()
endif()

install(FILES lshw.1 DESTINATION ${MANDIR}/man1 COMPONENT doc)
install(TARGETS lshw DESTINATION sbin)
