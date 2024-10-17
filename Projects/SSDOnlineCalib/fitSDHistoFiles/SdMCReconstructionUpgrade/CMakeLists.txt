INSTALL (FILES
  ${srcdir}/AugerOfflineUser.cc
  ${srcdir}/ModuleSequence.xml.in
  ${srcdir}/bootstrap.xml.in
  ${srcdir}/EventFileReader.xml.in
  ${builddir}/Makefile
  DESTINATION ${docdir}/StandardApplications/SdMCReconstructionUpgrade
)

AUGER_CREATE_CONFIGURE_FILES (
  ${srcdir}/Makefile.in
)
