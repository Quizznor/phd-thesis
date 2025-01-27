#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "muonfill.h"
#include "status.h"
#include "config.h"
#include "muonfill_sha1version.h"

#include "logfile.h"


#include "muoncalib.h"
#include "muon_read.h"

void write_histo(struct muon_histo_complete *histo)
{
  FILE *arq;
  arq=fopen("histos","w");
  if(arq!=NULL){
    fwrite(histo,sizeof(struct muon_histo_complete),1,arq);
    fclose(arq);
  }
}


int main()
{
  FILE *arq;
  struct muon_histo_complete *histo;
  struct muon_read_str *read_buff;
  struct muon_read_info muon_buff;
  int threshold[4],th;

  struct muoncalib_debug_str debug;

  STATUS *st;
  CONFIG *cf;

  int i,j;
  int secs_no_data;
  int runstatus, prev_runstatus;

  evbuffer *evb_muon;
  int err;

  unsigned char tag[16];


  int npmts,mask;

  muoncalib_debug_init(&debug);

  LogProgName = "MuonFill" ;

  memset(tag,0,16);

  st = statuslib_Initialize(); /*this is mostly to have information of
                                * GPS time offset
                                */
  if (st == NULL) {
    PrintLog(LOG_ERROR, "Can't initialize status module.\n");
    exit(2);
  }

  cf = configlib_Initialize();
  if (cf == NULL) {
    PrintLog(LOG_ERROR, "Can't initialize Config module.\n");
    exit(2);
  }
  if(st->muonfill.proc.pid!=0){
    printf("MuonFill: It looks to have another process running\n");
    exit(0);
  }
  st->muonfill.proc.pid = getpid();
  strcpy(st->muonfill.proc.version,muonfill_SHA1VERSION);
  st->muonfill.proc.pst = eproc_st_Start;
  clock_gettime(CLOCK_MONOTONIC_COARSE,&st->muonfill.proc.last_work_ok);
  read_buff=muon_read_init(cf->sys.UIO);
  if(read_buff==NULL){
    PrintLog(LOG_ERROR, "Problem to start read of muon buffer\n");
    exit(1);
  }

  while(( err= EvbInit(&evb_muon, MUON_BUFF_NAME, MUON_BUFF_SIZE) )
        ==  EVB_ERR_UNKNOWN){
    clock_gettime(CLOCK_MONOTONIC_COARSE,&st->muonfill.proc.last_work_ok);
    usleep(10000);
  }
  if( err) {
    PrintLog( LOG_ERROR, "EvbInit Err %d\n", err);
    exit(1);
  }
  /*get the first allocation before enter in the main loop */

  while( err = EvbAlloc(evb_muon,(void **)&histo,
                        sizeof(struct muon_histo_complete))){
    printf("Muon Fill not alloc!!\n");
    EvbFree(evb_muon, 3);
  }
  st->muonfill.proc.pst = eproc_st_DaqInit;
  clock_gettime(CLOCK_MONOTONIC_COARSE,&st->muonfill.proc.last_work_ok);

  muon_read_search_baseline(read_buff,&st->muonfill.proc.last_work_ok);
  if(cf->FeParams.V2Settings.MuCalibChannEnable){
    muon_read_set_enable_calib_ch(read_buff,1);
    PrintLog( LOG_INFO, "calib channel active\n");
  } else {
    muon_read_set_enable_calib_ch(read_buff,0);
    PrintLog( LOG_INFO, "calib channel Not active active\n");
  }
  secs_no_data=0;

  muoncalib_init(cf->FeParams.V2Settings.MuCalibSSDPkmin,
                 cf->FeParams.V2Settings.MuCalibSSDPkmax,
                 cf->FeParams.V2Settings.MuCalibSSDChmin,
                 cf->FeParams.V2Settings.MuCalibSSDChmax,
                 cf->FeParams.V2Settings.MuCalibWCDChmin,
                 cf->FeParams.V2Settings.MuCalibWCDChmax,
                 cf->FeParams.V2Settings.MuCalib2Mask,
                 cf->FeParams.V2Settings.MuCalib2ThVal,
                 cf->FeParams.V2Settings.MuCalib2BinMin,
                 cf->FeParams.V2Settings.MuCalib2BinMax,
                 cf->FeParams.V2Settings.MuCalibDeltaTime,
                 cf->FeParams.V2Settings.MuCalib2NAcc
                 );

  printf("Muonfill: ready\n");
  PrintLog( LOG_INFO, "Ready to start\n");
  st->muonfill.proc.pst = eproc_st_Running;   //this process do not have
                                              // a way to stop ...

  if(cf->FeParams.V2Settings.MuCalibChannEnable){
    muon_read_set_enable_calib_ch(read_buff,1);
    PrintLog( LOG_INFO, "calib channel active\n");
  } else {
    muon_read_set_enable_calib_ch(read_buff,0);
    PrintLog( LOG_INFO, "calib channel Not active active\n");
  }

  prev_runstatus = -1;
  while(1){
    clock_gettime(CLOCK_MONOTONIC_COARSE,&st->muonfill.proc.last_work_ok);
    runstatus = st->control.RunStatus; /* it use one additional variable,
                                        *  because it may happen to
                                        *  have modifition in
                                        *  st->control.RunStatus between
                                        *  one cicle
                                        */
    if(runstatus != RUN_STARTED){
      /* make muonfill only go ahead only if runstatus is RUN_STARTED */
      prev_runstatus = runstatus;
      sleep(1);
    } else {
      /* The runstatus is RUN_STARTED */
      if(prev_runstatus != runstatus){
        /* run this procedure everytime the runstatus become RUN_STARTED,
         * but it would be run only once each time runstatus
         * become RUN_STARTED
         */
        muon_read_search_baseline(read_buff,&st->muonfill.proc.last_work_ok);
        prev_runstatus = runstatus;

        muoncalib_force_baseline_calc();
        secs_no_data=0;
      }  else if(muon_read_get(read_buff,&muon_buff)==1){
        secs_no_data++;
        if(secs_no_data>60){
          PrintLog( LOG_INFO, "Resetting threshold\n");
          muoncalib_force_baseline_calc();
          muon_read_search_baseline(read_buff,&st->muonfill.proc.last_work_ok);
          secs_no_data=0;
        }
      } else {
        secs_no_data=0;
        if(muoncalib_TreatBuffer(histo,&muon_buff,&debug,0)==1){
          histo->histo.StartSecond +=st->GpsSecondOffset;
          histo->histo.EndSecond += st->GpsSecondOffset;
          if(histo->extra.h2.StartSecond>0){
            histo->extra.h2.StartSecond +=st->GpsSecondOffset;
            histo->extra.h2.EndSecond +=st->GpsSecondOffset;
          } else {
            histo->extra.h2.StartSecond = 0;
            histo->extra.h2.EndSecond   = 0;
          }
          if(histo->extra.calibch.StartSecond>0)
            histo->extra.calibch.StartSecond += st->GpsSecondOffset;
          muon_read_get_config(read_buff,histo->extra.trig_params);
          write_histo(histo);
          if(cf->FeParams.V2Settings.MuTrigMode==0) {
            for(i=0;i<4;i++){
              threshold[i]=histo->histo.Offset[i]+30; /* 10 + 20 */
            }
            mask=0x7; /* consider all PMTs working */
            npmts=1;  /* trigger multiplicity =1 */
          } else {
            /* consider the working PMTs according Trigger2 */
            npmts=0;
            mask = (st->trigger2.pmt_mask) & 0xF;
            for(i=0;i<4;i++){
              th = st->trigger2.VEM[i]/100; /* it is .1VEM */
              if(th<12)
                th=12;
              threshold[i] = histo->histo.Offset[i] + 10 + th;
              if(mask & (0x1<<i)){
                npmts++;
              }
            }
          }
          printf("Set Threshold: %d %d %d %d; pmt mask: %d ; ncoinc: %d t:%d %d %d %d\n",
                 threshold[0],threshold[1],threshold[2],threshold[3],
                 mask,npmts,
                 histo->histo.StartSecond,
                 histo->histo.EndSecond,
                 histo->extra.h2.StartSecond,
                 histo->extra.h2.EndSecond );

          PrintLog( LOG_ERROR, "Thres.: %d %d %d %d; offset=%d %d %d %d;mask: %d ; ncoin: %d\n",
                    threshold[0],
                    threshold[1],
                    threshold[2],
                    threshold[3],
                    histo->histo.Offset[0],histo->histo.Offset[1],
                    histo->histo.Offset[2],histo->histo.Offset[3],
                    mask,npmts
                    );
          muoncalib_debug_io(&debug,histo);
          muon_read_set_threshold(read_buff,threshold); /* set threshold */
          muon_read_set_enable(read_buff,mask,npmts);  /* set trig. condition */
          /*send the histograms to the shared buffer */
          if( err= EvbReady(evb_muon,(void *)histo,
                            sizeof(struct muon_histo_complete),tag)) {
            PrintLog( LOG_ERROR, "EvbReady Err %d\n", err);
          }
          /*get new allocation for next time */
          while( err = EvbAlloc(evb_muon,(void **)&histo,
                                sizeof(struct muon_histo_complete)))
            EvbFree(evb_muon, 3);
        } else {
          muoncalib_debug_io(&debug,NULL);
        }
      }
    } /* end of else of if(runstatus != RUN_STARTED) */
  }
  EvbFinish(&evb_muon);
  return(0);
}
