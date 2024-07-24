INSTALL (FILES
  ${srcdir}/AugerOfflineUser.cc
  ${srcdir}/ModuleSequence.xml.in
  ${srcdir}/bootstrap.xml.in
  ${srcdir}/bootstrap_ssd.xml.in
  ${srcdir}/EventFileReader.xml.in
  ${srcdir}/EventFileExporter.xml.in
  ${srcdir}/EventGenerator.xml.in
  ${srcdir}/fix-ci-seeds.sh
  
  ${srcdir}/Makefile
  DESTINATION ${docdir}/StandardApplications/SdSimulationReconstruction
)
