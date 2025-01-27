#include <stdio.h>
#include <stdlib.h>

#include "config.h"
int main()
{
  CONFIG *cf;
  cf=NULL;
  cf = configlib_Initialize();
  if (cf == NULL) {
    printf( "Can't initialize Config module.\n");
    exit(2);
  }
  printf("cf=%d %d   %d %d\n",
         cf->FeParams.V2Settings.MuCalibSSDPkmin,
         cf->FeParams.V2Settings.MuCalibSSDPkmax,
         cf->FeParams.V2Settings.MuCalibSSDChmin,
         cf->FeParams.V2Settings.MuCalibSSDChmax);
  
  printf("DAC=%d %d %d   %d %d %d\n",
         cf->MonitParams.Dac[0],
         cf->MonitParams.Dac[1],
         cf->MonitParams.Dac[2],
         cf->MonitParams.Dac[3],
         cf->MonitParams.Dac[4],
         cf->MonitParams.Dac[5]);
  //cf->MonitParams.Dac[5]=0x40;
  return(0);

}

