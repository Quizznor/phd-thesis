#ifndef IOSDMETEO_H
#define IOSDMETEO_H

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>
#include <iostream>

#include "TObject.h"

using namespace std;

// Brian Fick 22/01/2007
//    The format is :
//        1) format code  - something from the weather station that indicates
//        the nature of the extracted data. It should be ignored.
//        2) Day of the year
//        3) Hour of the day
//        4) minute
//        5) average temp (C) over past 5 minutes
//        6) Relative humidity , instantaneous
//        7) average wind speed over past 5 minutes , km/hr
//        8) instantaneous wind speed
//        9) maximum wind speed over past 5 minutes
//        10) wind direction (counter clockwise from North)
//        11) Barometric pressure
//


class IoSdMeteoData {
 public:
  typedef enum {eTemperature, 
                eHumidity, 
                eWindSpeed, 
                eWindSpeedInst, 
                eWindSpeedMax, 
                eWindDirection, 
                ePressure, 
                eLastMeteoData} IoSdMeteoDataEnum;
  bool valid;
  double d[eLastMeteoData];
  IoSdMeteoData() {
    valid=false;
    for (unsigned int i = eTemperature; i < eLastMeteoData; ++i) 
      d[i] = -1.0;
  }
  ClassDef(IoSdMeteoData, 1);
};


#endif
