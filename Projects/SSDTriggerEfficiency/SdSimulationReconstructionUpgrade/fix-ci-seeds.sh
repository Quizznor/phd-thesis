#!/bin/bash


DETECTORSEED=517695534
PHYSICSSEED=556232385

if [ ! grep -q RandomEngineRegistry ] ; then
  sed -i 's|</centralConfig>|  <configLink id="RandomEngineRegistry" type="XML" xlink:href="./RandomEngineRegistry.xml"/>\n\n  </centralConfig>|' bootstrap.xml.in
fi

cat >RandomEngineRegistry.xml.in <<EOF
<?xml version="1.0" encoding="iso-8859-1"?>

<RandomEngineRegistry
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:noNamespaceSchemaLocation='@SCHEMALOCATION@/RandomEngineRegistry.xsd'>

  <DetectorSeed> $DETECTORSEED </DetectorSeed>
  <PhysicsSeed> $PHYSICSSEED </PhysicsSeed>

</RandomEngineRegistry>
EOF
