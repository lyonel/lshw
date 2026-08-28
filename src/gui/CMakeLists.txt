if(NOT GUI OR STATIC)
  message("-- gtk-${PROJECT_NAME} disabled")
  return()
endif()

find_package(PkgConfig)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0 gmodule-2.0)

set(GTK_SOURCES
  callbacks.c
  gtk-lshw.c
  stock.c
  engine.cc
  print-gui.cc
)

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src/core")

include_directories("${GTK3_INCLUDE_DIRS}")
link_directories("${GTK3_LIBRARY_DIRS}")

add_executable(gtk-lshw ${GTK_SOURCES})
target_link_libraries(gtk-lshw ${SQLITE3_LIBRARIES} ${Z_LIBRARIES} ${GTK3_LIBRARIES} core resolv)
install(TARGETS gtk-lshw DESTINATION sbin)

if(POLICYKIT)
  set(desktop_exec ${CMAKE_INSTALL_FULL_BINDIR}/lshw-gui)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/org.ezix.lshw.gui.policy.in"
    "${CMAKE_CURRENT_BINARY_DIR}/org.ezix.lshw.gui.policy")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/lshw-gui.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lshw-gui")
else()
  set(desktop_exec ${CMAKE_INSTALL_FULL_SBINDIR}/gtk-lshw)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/integration/gtk-lshw.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gtk-lshw.desktop")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/artwork/logo.svg"
  "${CMAKE_CURRENT_BINARY_DIR}/gtk-lshw.svg")

file(GLOB LOGOS "artwork/*.svg")
file(GLOB NOLOGOS "artwork/nologo/*.svg")

if(NOLOGO)
  install(FILES ${NOLOGOS} DESTINATION ${PROJECT_DATADIR}/artwork)
  foreach(LOGO ${LOGOS})
    get_filename_component(BASENAME ${LOGO} NAME)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/artwork/nologo/${BASENAME}")
      list(REMOVE_ITEM LOGOS ${LOGO})
    endif()
  endforeach()
endif()

install(FILES ${LOGOS} DESTINATION ${PROJECT_DATADIR}/artwork)
install(FILES gtk-lshw.ui DESTINATION ${PROJECT_DATADIR}/ui)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gtk-lshw.desktop
  DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/applications)
install(FILES integration/gtk-lshw.appdata.xml DESTINATION
  ${CMAKE_INSTALL_FULL_DATADIR}/appdata)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gtk-lshw.svg DESTINATION
  ${CMAKE_INSTALL_FULL_DATADIR}/pixmaps)


if(POLICYKIT)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.ezix.lshw.gui.policy
    DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/polkit-1/actions)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/lshw-gui
    DESTINATION bin)
endif()
