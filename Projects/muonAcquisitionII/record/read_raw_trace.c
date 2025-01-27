#include <stdio.h>
#include "muon_read.h"

#define max_bin_base 10
#define NBBIN 69

struct global
{
  int ch_bin_min[4],ch_bin_max[4];
  
  
} gl;
struct muoncalib_traceflags
{
  uint32_t ttag;
  uint32_t trig_type;
  uint32_t is_calib_channel;
};


int muoncalib_calc_parameters(int peak[],int charge[],int base[],
                              int bin_max[],int trace[][NBBIN],
                              struct muoncalib_traceflags *flags,
                              uint32_t buff1[],uint32_t buff2[],int index,
                              int *trig)
{
  /*
   * calculate the
   *    peak as maximum of the value of the trace
   *    charge as a sum of the trace between gl.ch_bin_min and gl.ch_bin_max
   *    base as the trace[bin=0]
   *
   * return 0: on success
   *        1: no data
   *        2: on error (data format look wrong)
   */
  int pmt;
  int bin;
  int nn,up[4],th[4];
  th[0]=0x120;
  th[1]=0x105;
  th[2]=0x106;

  *trig=0;

  nn=0;
  up[0]=0;
  up[1]=0;
  up[2]=0;
  up[3]=0;
  if(8096-NBBIN-1 < index){
    return(1);
  }
  flags->ttag     = buff1[index];
  flags->trig_type= buff2[index];

  if((((flags->ttag      >> 31) & 0x1) !=1) ||
     (((flags->trig_type >> 31) & 0x1) !=1)    ){
    return(2);
  }
  flags->ttag      &= 0x7FFFFFFF;
  flags->is_calib_channel =  ( flags->trig_type >> 5 ) & 0x1;
  flags->trig_type &= 0xF;

  index++; /* from this point, index should be the first time bin of trace */
  if((buff1[index ] & 0x80000000) ||
     (buff2[index ] & 0x80000000) ){
    return(2);
  }

  base[0] =  buff1[index]        & 0xFFF;
  base[1] = (buff1[index] >> 16) & 0xFFF;
  base[2] =  buff2[index]        & 0xFFF;
  base[3] = (buff2[index] >> 16) & 0xFFF;

  for(pmt=0; pmt<4; pmt++){
    if(gl.ch_bin_min[pmt] <= 0){
      charge[pmt] = base[pmt];
    } else {
      charge[pmt] = 0;
    }
    peak[pmt] = base[pmt];
    bin_max[pmt] = 0;
    trace[pmt][0] = base[pmt];
  }

  for(bin=1; bin<NBBIN; bin++ ){
    if((buff1[index + bin ] & 0x80000000) ||
       (buff2[index + bin ] & 0x80000000) ){
      return(2);
    }
    
    trace[0][bin]= buff1[index + bin]        & 0xFFF;
    trace[1][bin]=(buff1[index + bin] >> 16) & 0xFFF;
    trace[2][bin]= buff2[index + bin]        & 0xFFF;
    trace[3][bin]=(buff2[index + bin] >> 16) & 0xFFF;
    nn=0;
    for(pmt=0; pmt<4; pmt++){
      if(gl.ch_bin_min[ pmt ] <= bin &&
         bin < gl.ch_bin_max[ pmt ]    ){
        charge[pmt] += trace[pmt][bin];
      }

      if( peak[pmt] < trace[pmt][bin] ){
        peak[pmt] = trace[pmt][bin];
        bin_max[pmt] = bin;
      }
      if (pmt<3) {
        if(th[pmt]<trace[pmt][bin]){
          up[pmt]=3;
          nn++;
        } else if(0<up[pmt]) {
          up[pmt]--;
          nn++;
        }
      }
    }
    if(2<nn){
      *trig=1;
    }
  }
  return 0;
}


int main(int argc,char *argv[])
{
  struct muon_read_info buff;
  FILE *arq;
  int bin,i,j;
  uint32_t v1,v2;
  int p[4],n[4],nn,th[4],trigger;
  int sig[4][NBBIN ];

  int flag,prev_flag,b_avg;
  int nevt;

  int pk[4],ch[4],b[4],pk_bin[4];
  int hpk[2][4][4096],hch[2][4][65536];
  struct muoncalib_traceflags stflags;

  trigger=0;
  nevt=0;

  gl.ch_bin_min[0]=0;
  gl.ch_bin_min[1]=0;
  gl.ch_bin_min[2]=0;
  gl.ch_bin_min[3]=15;

  gl.ch_bin_max[0]=70;
  gl.ch_bin_max[1]=70;
  gl.ch_bin_max[2]=70;
  gl.ch_bin_max[3]=35;
  
  if(argc>1){
    arq=fopen(argv[1],"r");
    if(arq!=NULL){
      do{
	flag=fread(&buff,sizeof(buff),1,arq);
	if(flag==1){
	  bin=0;
	  for(i=0;i<MUON_MEM_WORDS;i+=NBBIN+1){
            muoncalib_calc_parameters(pk,ch,b,pk_bin,sig,&stflags,
                                      buff.buff[0],buff.buff[1],i,&trigger);
            //printf("%d %d %d %d -- %d\n",
            //       ch[0],ch[1],ch[2],ch[3],trigger);

            printf("%d %d %d %d -- %d\n",
                   pk[0],pk[1],pk[2],pk[3],trigger);

            for(j=0;j<4;j++){
              if(trigger){
                hpk[0][j][pk[j]]++;
                hch[0][j][ch[j]]++;
              } else {
                hpk[1][j][pk[j]]++;
                hch[1][j][ch[j]]++;
              }
            }
            nevt++;
          }
        }
      } while(flag==1);
    }
  }

  //for(i=0;i<65000;i++){
  //  printf("%d %d %d %d %d   %d %d %d %d\n",
  //         i,
  //         hch[0][0][i],hch[0][1][i],hch[0][2][i],hch[0][3][i],
  //         hch[1][0][i],hch[1][1][i],hch[1][2][i],hch[1][3][i]);
  //}
}
/*
root@Auger-uub:~/daq# reg 140                                                                                                    
reg: 140 -- 00000120 ... 00000120
root@Auger-uub:~/daq# reg 141
reg: 141 -- 00000105 ... 00000105
root@Auger-uub:~/daq# reg 142
reg: 142 -- 00000106 ... 00000106
root@Auger-uub:~/daq# reg 143
reg: 143 -- 00000108 ... 00000108
*/
