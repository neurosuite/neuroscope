project(neuroscope-doc)
macro (CREATE_HTML_DOC _docbook _dir )
   get_filename_component(_input ${_docbook} ABSOLUTE)
   get_filename_component(_base ${_input} NAME)

   set(_htmlDoc ${CMAKE_CURRENT_BINARY_DIR}/index.html)
   add_custom_command(OUTPUT ${_htmlDoc}
      COMMAND ${DOCBOOK2HTML_EXECUTABLE} --output ${CMAKE_CURRENT_BINARY_DIR} ${_input}
      DEPENDS ${_input} ${DOCBOOK2HTML_EXECUTABLE}
   )

   get_filename_component(_targ ${CMAKE_CURRENT_SOURCE_DIR} NAME)
   add_custom_target(${_targ} ALL DEPENDS "${_htmlDoc}")

   install( DIRECTORY Images DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/neuroscope/ )
   file(GLOB _html ${CMAKE_CURRENT_BINARY_DIR}/*.html)
   install(FILES ${_html} ${_htmlDoc} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/neuroscope/)

endmacro (CREATE_HTML_DOC)

#find_program(DOCBOOK2HTML_EXECUTABLE NAMES docbook2html )
#if(DOCBOOK2HTML_EXECUTABLE)
#   add_subdirectory(en)
#else(DOCBOOK2HTML_EXECUTABLE)
#   MESSAGE(STATUS "docbook2html not found. Document will not generated")
#endif(DOCBOOK2HTML_EXECUTABLE)

file(GLOB _html ${CMAKE_CURRENT_SOURCE_DIR}/en/html/*.html)
install(FILES ${_html} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/neuroscope/HTML/en)


