ADD_SUBDIRECTORY(glossary)

# Set of available languages.
SET (TP_LINGUAS az ca cs da eu fa ja nl rw sk sr sv tr uk zh_CN)
# already marked as external at TP:
SET (GC_LINGUAS ar bg de el en_GB es fi fr gu he hi hu it kn ko lt lv mr nb ne pl pt pt_BR ro ru ta te ur vi zh_TW)
# not marked or no TP team:
SET (NEW_LINGUAS as brx doi es_NI kok kok@latin ks mai mni mni@bengali)

SET (ALL_LINGUAS ${TP_LINGUAS} ${GC_LINGUAS} ${NEW_LINGUAS})

SET (CATALOGS "")
SET (BUILD_CATALOGS "")

SET(CMAKE_COMMAND_TMP "")
IF (${CMAKE_VERSION} VERSION_GREATER 3.1)
  SET(CMAKE_COMMAND_TMP ${CMAKE_COMMAND} -E env)
ENDIF()

SET(po_SOURCES "")
FOREACH(lingua ${ALL_LINGUAS})
  LIST(APPEND po_SOURCES ${lingua}.po)
ENDFOREACH()

SET_LOCAL_DIST(po_DIST_local ${po_SOURCES} CMakeLists.txt ChangeLog Makevars
  POTFILES.ignore POTFILES.in POTFILES.skip README check-po.cmake)
SET(po_DIST ${po_DIST_local} ${po_glossary_DIST} PARENT_SCOPE)

FOREACH(lingua ${ALL_LINGUAS})
  SET(_OUTPUT_FILE ${lingua}.mo)
  LIST(APPEND CATALOGS ${_OUTPUT_FILE})
  ADD_CUSTOM_COMMAND(
      OUTPUT ${_OUTPUT_FILE}
      COMMAND ${CMAKE_COMMAND_TMP} ${MSGFMT} -o ${_OUTPUT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${lingua}.po
  )
  SET(_BUILD_FILE_DIR ${DATADIR_BUILD}/locale/${lingua}/LC_MESSAGES)
  MAKE_DIRECTORY(${_BUILD_FILE_DIR})
  SET(_BUILD_FILE ${_BUILD_FILE_DIR}/gnucash.mo)
  LIST(APPEND BUILD_CATALOGS ${_BUILD_FILE})
  ADD_CUSTOM_COMMAND(
    OUTPUT ${_BUILD_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${lingua}.mo ${_BUILD_FILE}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_OUTPUT_FILE}
    #APPEND
    )
ENDFOREACH(lingua)

ADD_CUSTOM_TARGET(po-gmo ALL DEPENDS ${CATALOGS})
ADD_CUSTOM_TARGET(po-gmo-build ALL DEPENDS ${BUILD_CATALOGS})


FOREACH(lingua ${ALL_LINGUAS})
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${lingua}.mo RENAME gnucash.mo DESTINATION ${CMAKE_INSTALL_DATADIR}/locale/${lingua}/LC_MESSAGES)
ENDFOREACH(lingua)

FUNCTION(READ_FILE_REMOVING_COMMENTS OUTPUT PATH)
  SET(RESULT "")
  FILE(STRINGS "${PATH}" LINES_IN)
  FOREACH(line ${LINES_IN})
    STRING(REGEX REPLACE "#.*" "" line2 ${line})
    LIST(APPEND RESULT ${line2})
  ENDFOREACH()
  SET(${OUTPUT} ${RESULT} PARENT_SCOPE)
ENDFUNCTION()


FUNCTION(MAKE_GNUCASH_POTFILES)

  SET(IGNORE_PATTERNS "gw-" "test" "experimental" "python-bindings" "swig-.*\\.c")

  # Create a list of candidate translation files
  FILE(GLOB_RECURSE FILES_IN RELATIVE ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/*.c ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.glade ${CMAKE_SOURCE_DIR}/*.desktop.in
    ${CMAKE_SOURCE_DIR}/*.keys.in  ${CMAKE_SOURCE_DIR}/*.gschema.xml.in.in ${CMAKE_SOURCE_DIR}/*.scm)

  # Only consider files in the common, libgnucash, and gnucash/ directories. Also check against list of ignore patterns
  SET(FILES "")
  FOREACH(path ${FILES_IN})
    STRING(REGEX MATCH "^(bindings/|common/|borrowed/|libgnucash/|gnucash/)" IS_SRC ${path})
    IF (IS_SRC)
      SET(IS_IGNORED FALSE)
      FOREACH(pattern ${IGNORE_PATTERNS})
        STRING(REGEX MATCH ${pattern} YES_IGNORE ${path})
        IF (YES_IGNORE)
	  SET(IS_IGNORED TRUE)
        ENDIF()
      ENDFOREACH(pattern)
      IF (NOT IS_IGNORED)
        LIST(APPEND FILES ${path})
      ENDIF(NOT IS_IGNORED)
    ENDIF(IS_SRC)
  ENDFOREACH(path)

  # Read in list of paths to skip from POTFILES.{skip,ignore}
  READ_FILE_REMOVING_COMMENTS(SKIP_LINES POTFILES.skip)
  READ_FILE_REMOVING_COMMENTS(IGNORE_LINES POTFILES.ignore)

  # Remove the paths that are skipped or ignored
  FOREACH(path ${SKIP_LINES} ${IGNORE_LINES})
    list(REMOVE_ITEM FILES ${path})
  ENDFOREACH()


  # CMake sorting different from UNIX sorting. Sort the files
  # here so it is easier for me to compare to the autotools
  # generated POTFILES.in
  STRING(REPLACE ";" "\n" SORT_IN "${FILES}")
  SET(SORT_IN "${SORT_IN}\n")
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in.in "${SORT_IN}")

  EXECUTE_PROCESS(COMMAND "${PERL_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/util/elegant-sort.pl"
    INPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in.in
    OUTPUT_VARIABLE POTFILES_IN
  )
  STRING(REPLACE "\n" ";" POTFILES "${POTFILES_IN}")

  # Write out the final list.
  # intltool-update insists that this file be in the source directory. :-(
  SET(POTFILES_IN_PATH ${CMAKE_CURRENT_BINARY_DIR}/POTFILES.in)
  FILE(WRITE ${POTFILES_IN_PATH} "# This is a list of files which contain translatable strings.
# This file was generated by ../make-gnucash-potfiles.
")

  FOREACH(path ${POTFILES})
    STRING(REGEX MATCH gschema.xml.in.in IS_GSCHEMA ${path})
    IF(IS_GSCHEMA)
      # Force parse type for gsettings files
      SET(path "[type: gettext/gsettings]${path}")
    ENDIF()
    FILE(APPEND ${POTFILES_IN_PATH} "${path}\n")
  ENDFOREACH()

  # These are manually added, because they're not picked up by the generation script
  FILE(APPEND ${POTFILES_IN_PATH} "gnucash/gnome/gnucash.appdata.xml.in
gnucash/gnome/gnucash.desktop.in.in
libgnucash/engine/qofbookslots.h
doc/tip_of_the_day.list.in
")
  CONFIGURE_FILE(${POTFILES_IN_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in NEWLINE_STYLE LF)
ENDFUNCTION()

find_program(INTLTOOL_UPDATE NAMES intltool-update)
IF (${INTLTOOL_UPDATE} STREQUAL "INTLTOOL_UPDATE-NOTFOUND")
  MESSAGE(FATAL_ERROR "Can't find the 'intltool-update' program.")
ENDIF ()

IF(BUILDING_FROM_VCS)

  MAKE_GNUCASH_POTFILES()

  find_program(INTLTOOL_EXTRACT NAMES intltool-extract)
  find_program(XGETTEXT xgettext)

  IF (${INTLTOOL_EXTRACT} STREQUAL "INTLTOOL_EXTRACT-NOTFOUND")
    MESSAGE(FATAL_ERROR "Can't find the 'intltool-extract' program.")
  ENDIF ()

  IF (${XGETTEXT} STREQUAL "XGETTEXT-NOTFOUND")
    MESSAGE(FATAL_ERROR "Can't find the 'xgettext' program.")
  ENDIF ()

  IF (${CMAKE_VERSION} VERSION_GREATER 3.1 AND NOT WIN32)
    EXECUTE_PROCESS(
            COMMAND ${CMAKE_COMMAND_TMP} INTLTOOL_EXTRACT=${INTLTOOL_EXTRACT} XGETTEXT=${XGETTEXT} srcdir=${CMAKE_CURRENT_SOURCE_DIR} ${INTLTOOL_UPDATE} --gettext-package ${PACKAGE} --pot
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE GNUCASH_POT_RESULT
    )
    IF(NOT ${GNUCASH_POT_RESULT} STREQUAL "0")
      MESSAGE(FATAL_ERROR "Error when creating gnucash.pot: ${GNUCASH_POT_RESULT}")
    ENDIF()
  ELSE()
    CONFIGURE_FILE(make-gnucash-pot.sh.in make-gnucash-pot.sh)
    EXECUTE_PROCESS(
            COMMAND ${SHELL} ${CMAKE_CURRENT_BINARY_DIR}/make-gnucash-pot.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            RESULT_VARIABLE GNUCASH_POT_RESULT_SHELL
    )
    IF(NOT ${GNUCASH_POT_RESULT_SHELL} STREQUAL "0")
      MESSAGE(FATAL_ERROR "Error when creating gnucash.pot from shell script: ${GNUCASH_POT_RESULT_SHELL}")
    ENDIF()
  ENDIF()

  IF (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/gnucash.pot)
    MESSAGE(FATAL_ERROR "POT file '${CMAKE_CURRENT_SOURCE_DIR}/gnucash.pot' was not successfully created.")
  ENDIF()

ENDIF()

ADD_CUSTOM_TARGET(check-po
        COMMAND ${CMAKE_COMMAND}
           -D INTLTOOL_UPDATE=${INTLTOOL_UPDATE}
           -D PO_DIR=${CMAKE_CURRENT_SOURCE_DIR}
           -P check-po.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
