project(neuroscope-doc)
macro (CREATE_HTML_DOC _docbook _dir )
   get_filename_component(_input ${_docbook} ABSOLUTE)
   get_filename_component(_base ${_input} NAME)
   set(_ssheet "${KDE4_DATA_INSTALL_DIR}/ksgmltools2/customization/kde-chunk.xsl")

   set(_htmlDoc ${CMAKE_CURRENT_SOURCE_DIR}/index.html)
   add_custom_command(OUTPUT ${_htmlDoc}
      COMMAND ${DOCBOOK2HTML_EXECUTABLE} --output ${CMAKE_CURRENT_SOURCE_DIR}/html/ ${_input}
      DEPENDS ${_input} ${DOCBOOK2HTML_EXECUTABLE}
   )

   get_filename_component(_targ ${CMAKE_CURRENT_SOURCE_DIR} NAME)
   add_custom_target(${_targ} ALL DEPENDS "${_htmlDoc}")

   #install( DIRECTORY Images DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/klusters/ )
   #file(GLOB _html ${CMAKE_CURRENT_BINARY_DIR}/*.html)
   #install(FILES ${_html} ${_htmlDoc} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/klusters/)
   MESSAGE(STATUS "Need to add to git")

endmacro (CREATE_HTML_DOC)



if (KDE4_FOUND)
find_program(DOCBOOK2HTML_EXECUTABLE NAMES docbook2html )
if(DOCBOOK2HTML_EXECUTABLE)
   if (REGENERATE_DOC)
     add_subdirectory(en)
   endif()
endif(DOCBOOK2HTML_EXECUTABLE)
endif()

file(GLOB _html ${CMAKE_CURRENT_SOURCE_DIR}/en/html/*.html)
install(FILES ${_html} DESTINATION share/doc/neuroscope/HTML/en)

file(GLOB _image ${CMAKE_CURRENT_SOURCE_DIR}/en/Images/*.png)

install(FILES ${_image} DESTINATION share/doc/neuroscope/HTML/en/Images)

