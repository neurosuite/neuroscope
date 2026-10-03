# The handbook is kept pre-rendered in doc/en/html (generated from the DocBook
# sources in doc/en with docbook2html).
file(GLOB _html ${CMAKE_CURRENT_SOURCE_DIR}/en/html/*.html)
install(FILES ${_html} DESTINATION ${NEUROSCOPE_DOC_INSTALL_DIR})

file(GLOB _image ${CMAKE_CURRENT_SOURCE_DIR}/en/Images/*.png)
install(FILES ${_image} DESTINATION ${NEUROSCOPE_DOC_INSTALL_DIR}/Images)
