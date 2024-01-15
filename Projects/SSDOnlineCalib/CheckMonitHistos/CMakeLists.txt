SET (CDAS_SOURCES
  ${srcdir}/CDASEventFile.cc
  ${srcdir}/CDASToOfflineEventConverter.cc
  ${srcdir}/IoAugerEventFile.cc
)

LIST_REGEX_PLACE ("\\.cc$" ".h" CDAS_HEADERS ${CDAS_SOURCES})

LIST (APPEND AugerEventIO_SOURCES ${CDAS_SOURCES})
LIST (APPEND io_HEADERS ${CDAS_HEADERS})

AUGER_ADD_TEST (testCDASEventFile
  ${srcdir}/../../Utilities/Testing/testRunner.cc
  ${srcdir}/testCDASEventFile.cc
  LIBS ${AugerEventIO_LIBRARY}
  ${AugerFramework_LIBRARY}
  ${AugerUtilities_LIBRARY}
  ${XERCESC_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${CDAS_LIBRARIES}
  ${CLHEP_LIBRARIES}
  ${ROOT_LIBRARIES}
  #${MySQL_LIBRARIES}
  FLAGS -DBOOTSTRAPFILE=\"${builddir}/bootstrap.xml\" -DTESTFILE=\"${srcdir}/CDASInputTest_IoSd.root\"
)

AUGER_CREATE_CONFIG_RULES (
  ${srcdir}/bootstrap.xml.in
  TARGET testCDASEventFile
)
