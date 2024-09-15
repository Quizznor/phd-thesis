#!/bin/bash

DETECTORSEED=394057508
PHYSICSSEED=1429782542

CONFIGLINK="<configLink id=\"RandomEngineRegistry\" type=\"XML\" xlink:href=\"./RandomEngineRegistry.xml\"/>"

cat << EOF >  RandomEngineRegistry.xml.in
<?xml version="1.0" encoding="iso-8859-1"?>

<RandomEngineRegistry xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation='@SCHEMALOCATION@/RandomEngineRegistry.xsd'>

	<DetectorSeed> $DETECTORSEED </DetectorSeed>
	<PhysicsSeed> $PHYSICSSEED </PhysicsSeed>

</RandomEngineRegistry>
EOF

sed -i "s!</centralConfig>!  $CONFIGLINK\n\n  </centralConfig>!g" bootstrap.xml.in

