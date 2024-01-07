include(FindGettext)

if (GETTEXT_FOUND)
  file(GLOB POTFILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.po")
  string(REPLACE ".po" " " LANGS ${POTFILES})
  message(STATUS "gettext found: ${LANGS}")
  string(REPLACE " " ";" LANGS ${LANGS})
  foreach(LANG ${LANGS})
    GETTEXT_PROCESS_PO_FILES(${LANG} ALL PO_FILES ${LANG}.po)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.gmo"
      DESTINATION "${CMAKE_INSTALL_FULL_LOCALEDIR}/${LANG}/LC_MESSAGES"
      RENAME "${PROJECT_NAME}.mo")
  endforeach()
else()
  message("-- gettext not found")
endif()
