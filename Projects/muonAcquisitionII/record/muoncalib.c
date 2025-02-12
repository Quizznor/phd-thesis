#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "muoncalib.h"

/*definition of CALIBCH */
#define MUONCALIB_CALIB_NCHMAX 600


int muoncalib_debug_reset(struct muoncalib_debug_str *debug)
{
  int i;
  debug->state=0;
  debug->file_index=0;

  debug->h.nevts=0;
  debug->nevttot=0;
  return(0);
}

int muoncalib_debug_init(struct muoncalib_debug_str *debug)
{
  int i;
  debug->nevt_max=2200;
  debug->evts=malloc(debug->nevt_max*sizeof(struct muoncalib_debug_output_evt));
  for(i=0;i<12;i++){
    debug->h.preamble[i]='a'+i;
  }
  muoncalib_debug_reset(debug);
  debug->nevttot_max=300000; /* to limit the total amount of data to be stored*/
  return(0);
}

int muoncalib_debug_io(struct muoncalib_debug_str *debug,
                       struct muon_histo_complete *h)
{
  /* verify if it would start to acquire date (state=0 and
   *  the file "muonfill_debug_init" exist) or
   *  to store the data if is in the corresponding state
  *
   *  in case the total number of events is larger then
   *  nevttot_max, it will set state=0 and look for the
   *  muonfill_debug_init in the next time.
  *
   *  when the "h" is NULL and state!=0, it would store the
   *  data if the number of events is big enough.
   */
  char filename[100];
  FILE *arq;
  if(debug->state==0){
    if( h != NULL ){
      arq=fopen("muon_data_init","r");
      if(arq!=NULL){
        debug->state=1;
        fclose(arq);
        unlink("muon_data_init");
      }
    }
    return(0);
  }
  if(debug->state==1){
    return(0);
  }
  if(debug->state==2){
    debug->state=3;
    return(0);
  }
  if( (debug->h.nevts + 200 < debug->nevt_max) &&
      h==NULL){
    return(1);
  }

  sprintf(filename, "muon_data/%03d", debug->file_index);
  printf("Going to write to %s\n",filename);
  debug->file_index++;
  arq=fopen(filename, "w");
  if(arq==NULL){
    muoncalib_debug_reset(debug);
    return(1);
  }
  printf("Going to write event data\n");
  fflush(stdout);
  fwrite(&(debug->h),sizeof(debug->h),1,arq);
  fwrite(&(debug->evts),sizeof(struct muoncalib_debug_output_evt),
         debug->h.nevts,arq);
  debug->nevttot += debug->h.nevts;
  debug->h.nevts=0;
  if(h!=NULL){
    printf("Going to write histo\n");
    fflush(stdout);
    debug->h.evt_size = sizeof(struct muon_histo_complete);
    debug->h.nevts = 1;
    fwrite(&(debug->h),sizeof(debug->h),1,arq);
    fwrite(&(debug->evts),debug->h.evt_size,
           debug->h.nevts,arq);
    muoncalib_debug_reset(debug);
  } else if(debug->nevttot_max <=debug->nevttot){
    muoncalib_debug_reset(debug);
  }
  fclose(arq);
  return(0);
}

struct muoncalib_avg_sig_str
{
  int sum,sum2,n;
};

struct muoncalib_calib_channel
{
  /* for the readout the calibration channel.
   *it is mostly for the SiPM.
   * this histogram is not transmited with the normal event and
   * is a temporary solution for this particular channel.
   */
  uint16_t count;
  uint16_t offset;
  int full_flag; /* it would indicate if the base and fullhist got
                  * already a maximum possible value (2^16). bit 0:
                  * main histogram; bit 1: base histogram.
                  */
  int s1; /*this variable will be used only at the begining at
           * muoncalib_ssd_entry.
           */
  struct muon_calib histos[2]; /* 0 current, 1: previous */
};

struct muoncalib_h2_manage_str
{
  int Mask;
  int Th;
  int Pos[2];
  int naccumulate,iaccumulate; /* number of histograms which should be
                                *  accumulate
                                * naccumulate -> the number of hist. which
                                *     should accumulate
                                * iaccumulate -> the number which has already
                                *     accumulated
                                */
  struct muon_extra_histo *hprev,*hcurr;
  struct muoncalib_avg_sig_str tdiff[3];
  int16_t tdiff_res[3][2];
};

struct muoncalib_global{
  struct muoncalib_calib_channel SSD_calib_ch;
  int firsttime;

  /* ===== parameters related with peak and charge calculation. ===== */
  /* pk: Signal peak. The peak of the signal is considered only
   *    if the peak is between "min" (inclusive) and "max" (exclusive).
   * ch: Area or Charge of the signal. The charge will be computed
   *    only between "min" (inclusive) and "max" (exclusive).
   *    "bins" is the difference "max" - "min"
   * The "shift" parameter is the number of shift will be applied
   *   for peak and charge (ch/pk = ch/pk >> shift )
   */
  /*AREA calculation timebin. from min (inclusive) to max (exclusive).
   *  bins = "max" - "min"
   */
  int pk_bin_min[4],pk_bin_max[4];
  int ch_bin_min[4],ch_bin_max[4],ch_bins[4];
  int pk_bits_shift[4],ch_bits_shift[4];

  int integrationInterval; /* in seconds */
  struct muoncalib_h2_manage_str h2;
} gl;

struct muoncalib_traceflags
{
  uint32_t ttag;
  uint32_t trig_type;
  uint32_t is_calib_channel;
};

/*
 * the functions muoncalib_ssd_* are to take into account the
 *  calibration channel. It would be used to study
 *  the MIP in terms of single photo electron
 */


int muoncalib_avg_sig_reset(struct muoncalib_avg_sig_str *str)
{
  str->sum=0;
  str->sum2=0;
  str->n=0;
  return(0);
}

int muoncalib_avg_sig_entry(struct muoncalib_avg_sig_str *str,int val)
{
  str->sum += val;
  str->sum2 += val*val;
  str->n ++;
  return(0);
}
int muoncalib_avg_sig_calc(struct muoncalib_avg_sig_str *str,
                           double *avg,double *sig_avg)
{
  /* it is to calculate the average and the uncertainty of the data
   * entered with the function "muoncalib_avg_sig_entry.
   * Return:
   *   0 - On success and set the values avg and sig_avg
   *   1 -   fail - not enough data to evaluate the average and uncertainty.
   *   2 -   the \sum_i x_i^2 < \sum (*avg)^2 - formally it should never happen.
   */
  double aux,navg2;
  if(2<str->n){
    *avg = (double)(str->sum)/str->n;
    navg2 = *avg * str->sum;
    aux=(double)str->sum2;
    if(aux < navg2){
      return(2);
    }
    *sig_avg = sqrt( (aux- navg2)/ ((str->n)*(str->n - 1)) );
    return(0);
  } else {
    return(1);
  }
}


void muoncalib_ssd_init(struct muoncalib_calib_channel *ch)
{
  int i;

  memset(&(ch->histos[0]),0,sizeof(struct muon_calib));
  memset(&(ch->histos[1]),0,sizeof(struct muon_calib));
  ch->full_flag=0;
  ch->count=0;
  ch->offset=20000;
}

void muoncalib_ssd_newhisto(struct muoncalib_calib_channel *ch,int t_sec)
{
  /* write the histograms to a file and reset the existing histogram,
   *  if it looks razonable
   */
  int i,s1,s2;
  memcpy(&(ch->histos[1]),&(ch->histos[0]),sizeof(struct muon_calib));
  ch->histos[1].dtSecond=t_sec - ch->histos[1].StartSecond;
  /* need to recalculate the offset and reset the base it is
   *  basically calculate the average of the base distribution and
   *  subtract 10 to set the begining of the histogram.
   *
   *  It also reset the base histogram.
   */
  s1=0;s2=0;
  for(i=0;i<20;i++){
    s2+=ch->histos[0].Base[i];
    s1+=ch->histos[0].Base[i]*i;
  }

  s2+=ch->histos[0].Base_excess[0];
  s1-=ch->histos[0].Base_excess[0];

  s2+=ch->histos[0].Base_excess[1];
  s1+=ch->histos[0].Base_excess[1]*20;

  if(s2>0){
    ch->offset+=s1/s2-10;
  } else {
    ch->offset=0;
  }
  if(ch->offset < 0)
    ch->offset=0;
  if(4000 < ch->offset)
    ch->offset=4000;

  memset(&(ch->histos[0]),0,sizeof(struct muon_calib));

  ch->histos[0].Offset=ch->offset;
  /*start to count the GPS PPS. It
   * is not really the GPS second.
   */
  ch->full_flag=0;
}
void muoncalib_ssd_entry(struct muoncalib_calib_channel *ch,
                         int charge,int base,
                         int t_sec)
{
  int aux,ch_val;
  if(ch->offset>10000){
    /*this is the first time it is working, therefore, it need to
     * determine, somehow the offset
     */
    ch->s1+=base;
    ch->count++;
    if(ch->count>100){
      ch->offset=(ch->s1)/ch->count;
      if(ch->offset<10)
        ch->offset=0;
      else
        ch->offset-=10;

      ch->histos[0].StartSecond=t_sec;
      ch->full_flag=0;
    }
  } else {
    if((ch->full_flag & 0x2) ==0){
      /*the base histogram is not full. */
      aux=base - ch->offset;
      if(aux<0){
        ch_val=ch->histos[0].Base_excess[0]++;
      } else if(aux<20) {
        ch_val=ch->histos[0].Base[aux]++;
      } else {
        ch_val=ch->histos[0].Base_excess[1]++;
      }
      if(ch_val>65000)
        /* there are one base bin which is close to exceed the
         * possible value */
        ch->full_flag |= 0x2;
    }
    if((ch->full_flag & 0x1) ==0){
      /* the charge histogram is not full */
      aux=(charge -  gl.ch_bins[3]*ch->offset)>>4 ; /*divide by 16 */
      if(0<=aux){
        ch_val=0;
        if(aux<300){
          ch->histos[0].Charge[aux]++;
          ch_val = ch->histos[0].Charge[aux];
        } else {
          aux=((aux-300)>>2)+300;
          if(aux < MUONCALIB_CALIB_NCHMAX ){
            ch->histos[0].Charge[aux]++;
            ch_val=ch->histos[0].Charge[aux];
          }
        }
        if(ch_val>65000){
          /*there are one charge bin which is close to
           *exceed the possible value.
           */
          ch->full_flag |= 0x1;
        }
      }
    }
    ch->histos[0].dtSecond=t_sec-ch->histos[0].StartSecond;
    if(  ch->full_flag == 0x3  ||  900 <= ch->histos[0].dtSecond  ){
      /*it is already two histogram get maximum value in one particular bin or
       *it already filling the histogram for 15 minutes
       */
      muoncalib_ssd_newhisto(ch,t_sec);/*copy of the hist and start new one */
      /*to a second histogram and restart another histogram. */
      ch->histos[0].StartSecond=t_sec;
    }
  } /*end if(ch->offset==-1) {} else { ...} */
}

int muoncalib_add_histo_entry(uint16_t *h,int val_inter,int max,int val)
{
  /*
   *  add the value "val" to the histogram "h"
   *  h - array of the histogram to be filled.
   *  val_inter: the intermediary value
   *  max: array size.
   *  val: input value.
   *
   *  for
   *     0< val <val_inter: insert directly to "h"
   *       val_inter <= val: insert into the histogram, but "compressing"
   *                      the bins by 4. See the diagram below
   *
   *
   *   |---x----------------+---.---.-y-.---.---.---| (normal histogram)
   *   |---x----------------+--y---|                  (included)
   *
   *   x and y are the two entry values (in different calls).
   *   "+" is the value of val_inter
   *  return 0 - on success (the corresponding val value has considered in
   *        the histogram
   *         -1: value val is under the allowed value
   *          1: value val is over the allowed value.
   */
  int x;
  if(0<=val){
    if(val<val_inter){
      h[val]++;
      return(0);
    } else {
      x= val_inter + ( (val - val_inter)>>2 );
      if( x<max ){
        h[x]++;
        return(0);
      }
    }
  }else {
    return(-1);
  }
  return(1);
}



/* ======================================================================
 * the "h2" are the histogram generated in coincidence between SSD and SSD
 * the following functions are mostly to manage these histograms.
 * ======================================================================
 */
void muoncalib_h2_init(struct muoncalib_h2_manage_str *h2,
                       int mask,int thres,int pos0,int pos1,int nacc)
{
  /* start parameters related with "h2"
   */
  int i;
  h2->Pos[0]=pos0;
  h2->Pos[1]=pos1;
  h2->Th=thres;
  h2->Mask=mask;
  h2->naccumulate=nacc;
  h2->iaccumulate=0;
  h2->hprev=(struct muon_extra_histo *)
    malloc(sizeof(struct muon_extra_histo));
  h2->hcurr=(struct muon_extra_histo *)
    malloc(sizeof(struct muon_extra_histo));
  h2->hprev->has_histo=0;
  memset(h2->hcurr,0,sizeof(struct muon_extra_histo));
  muoncalib_avg_sig_reset(&h2->tdiff[0]);
  muoncalib_avg_sig_reset(&h2->tdiff[1]);
  muoncalib_avg_sig_reset(&h2->tdiff[2]);
  memset(h2->tdiff_res,0,3*2*sizeof(int16_t));
  for(i=0;i<3;i++){
    h2->tdiff_res[i][0]=-32001;
    h2->tdiff_res[i][1]=-1;
  }
}
int muoncalib_h2_new_histo(struct muoncalib_h2_manage_str *h2,int sec)
{
  /* make the copy of current histo to previous histo
   * and reset the current histo to start a new one
   */
  double avg,sig;
  int i;
  printf("H2 - new histo - start\n");
  //  return(0);
  h2->iaccumulate++;
  if(  h2->iaccumulate < h2->naccumulate){
    h2->hcurr->EndSecond = sec;
  } else {
    memcpy(h2->hprev,h2->hcurr,sizeof(struct muon_extra_histo));
    h2->hprev->EndSecond = sec;

    memset(h2->hcurr,0,sizeof(struct muon_extra_histo));
    h2->hcurr->mask = h2->Mask;
    h2->hcurr->ssdTh = h2->Th; /* the applied threshold */
    h2->hcurr->peakPos[0] = h2->Pos[0];
    h2->hcurr->peakPos[1] = h2->Pos[1];
    for(i=0;i<3;i++){
      if(muoncalib_avg_sig_calc(&(h2->tdiff[i]),&avg,&sig)==0){
        if(avg<-32.){
          h2->tdiff_res[i][0]=-32000;
        } else if(avg<32.){
          h2->tdiff_res[i][0]=(int16_t )(avg*1000.);
        } else {
          h2->tdiff_res[i][0]=32000;
        }
        if(sig<=0){
          h2->tdiff_res[i][1]=0;
        } else if(sig<32.){
          h2->tdiff_res[i][1]=(int16_t )(sig*1000.);
        } else {
          h2->tdiff_res[i][1]=32000;
        }
      } else {
        h2->tdiff_res[i][0]=-32001;
        h2->tdiff_res[i][1]=-1;
      }
      muoncalib_avg_sig_reset(&h2->tdiff[i]);
    }
    h2->iaccumulate = 0;
    h2->hcurr->StartSecond = sec;
  }
  printf("H2 - new histo - end\n");
  return(0);
}

int muoncalib_h2_entry(struct muoncalib_h2_manage_str *h2,
                       int *peak,
                       int *charge,
                       int *pk_bin)
{
  /* add the signal entry or not
   *  peak[0],...,peak[3] -> peak value of the signal
   *      WCD PMT1, WCD PMT2, WCD PMT3, SSD PMT
   *  charge[0],...,charge[3] - the same for charge
   *  pk_bin[0],...,pk_bin[3] - the same for the bin which
   *     the signal get the maximum value.
   *  include also the time difference entry
   */


  /* condition to consider the coincidence Histogram.
   * Mask: bit 0 -> if the threshold condition should be applied.
   *       bit 1 -> if peak position (bin) should be in a particular
   *                    range
   * the parameters Mask, Th, and Pos are set at "_init".
   */
  //return(0);
  int add;

  if( (((h2->Mask & 0x1)==0) ||
       ((h2->Mask & 0x1) && (h2->Th < peak[3]))) &&
      (((h2->Mask & 0x2)==0) ||
       ((h2->Mask & 0x2) &&
        (h2->Pos[0]<=pk_bin[3] && pk_bin[3]<h2->Pos[1])))) {
    int bin_diff,i;
    h2->hcurr->has_histo=1;
    for(i=0;i<3;i++){
      if( gl.pk_bin_min[i] <= pk_bin[i] &&
          pk_bin[i] < gl.pk_bin_max[i] ){ /* position of peak */
        /*include signal in the histogram */
        muoncalib_add_histo_entry(h2->hcurr->Peak[i]  ,
                                  100,150,peak[i] >> gl.pk_bits_shift[i]);
        muoncalib_add_histo_entry(h2->hcurr->Charge[i],
                                  400,600,charge[i] >> gl.ch_bits_shift[i]);

        /* time difference: peak_bin[ssd] - peak_bin[wcd pmt _i] */
        bin_diff=pk_bin[i]-pk_bin[3];
        muoncalib_avg_sig_entry(&(h2->tdiff[i]),bin_diff);
      }
    }
  }
  return(0);
}
int muoncalib_h2_get_histo(struct muoncalib_h2_manage_str *h2,struct muon_extra *extr)
{
  int i;
  for( i=0; i<3; i++ ){
    extr->tdiff[i][0] = h2->tdiff_res[i][0];
    extr->tdiff[i][1] = h2->tdiff_res[i][1];
  }
  if(h2->hprev->has_histo){
    memcpy(&(extr->h2),h2->hprev,sizeof(struct muon_extra_histo));
  } else {
    memcpy(&(extr->h2),h2->hcurr,sizeof(struct muon_extra_histo));
  }
  return(0);
}

void muoncalib_init(int ssd_p_min,int ssd_p_max,
                    int ssd_a_min,int ssd_a_max,
                    int wcd_a_min,int wcd_a_max,
                    int h2Mask,
                    int h2Th,
                    int h2Pos0,int h2Pos1,
                    int integrationInterval,
                    int h2nacc)

{
  int i;
  gl.firsttime=0x3; /*bit 0: need to allocate the histograms;
                     *    1: need to calculate the offset */
  muoncalib_ssd_init(&(gl.SSD_calib_ch));

  /* the index 0,1,2 corresponds to WCD; 3: for SSD. */

  /* Peak: should be between "pk_bin_min" (inclusive) and
   *  "pk_bin_max" (exclusive)
   */
  gl.pk_bin_min[0] = 0 ;
  gl.pk_bin_min[1] = 0 ;
  gl.pk_bin_min[2] = 0 ;
  gl.pk_bin_min[3] = ssd_p_min ;

  gl.pk_bin_max[0] = 70 ;
  gl.pk_bin_max[1] = 70 ;
  gl.pk_bin_max[2] = 70 ;
  gl.pk_bin_max[3] = ssd_p_max ;

  /* charge (area): will be calculated between "ch_bin_min" (inclusive) and
   *   "ch_bin_max" (exclusive).
   *   "ch_bins" corresponds to the total number of bins between
   *       min and max.
   */

  gl.ch_bin_min[0] = wcd_a_min ;
  gl.ch_bin_min[1] = wcd_a_min ;
  gl.ch_bin_min[2] = wcd_a_min ;
  gl.ch_bin_min[3] = ssd_a_min ;

  gl.ch_bin_max[0] = wcd_a_max ;
  gl.ch_bin_max[1] = wcd_a_max ;
  gl.ch_bin_max[2] = wcd_a_max ;
  gl.ch_bin_max[3] = ssd_a_max ;

  for(i=0;i<4;i++){
    gl.ch_bins[i] = gl.ch_bin_max[i] - gl.ch_bin_min[i];
  }

  /* to reduce the amount of data to be transfered,
   *   it is applied a bit shift for peak and area.
   *     peak = (peak >> pk_bits_shift);
   *     charge = (charge >> pk_bits_shift);
   */

  gl.pk_bits_shift[0]=2;
  gl.pk_bits_shift[1]=2;
  gl.pk_bits_shift[2]=2;
  gl.pk_bits_shift[3]=1;

  gl.ch_bits_shift[0]=3;
  gl.ch_bits_shift[1]=3;
  gl.ch_bits_shift[2]=3;
  gl.ch_bits_shift[3]=1;

  gl.integrationInterval = integrationInterval;
  muoncalib_h2_init(&(gl.h2),h2Mask,h2Th,h2Pos0,h2Pos1,h2nacc);
}

void muoncalib_ClearHisto(struct muon_histo_complete *hist){
  int i,j;
  for (i=0;i<4;i++) {
    for (j=0;j<20;j++) hist->histo.Base[i][j]=0;
    for (j=0;j<150;j++) hist->histo.Peak[i][j]=0;
    for (j=0;j<600;j++) hist->histo.Charge[i][j]=0;
    for (j=0;j<NBBIN;j++) hist->histo.Shape[i][j]=0;
    hist->extra.excess_bins[i][0]=0;
    hist->extra.excess_bins[i][1]=0;
  }
  hist->histo.StartSecond=0;
  hist->histo.EndSecond=0;
  hist->histo.NEntries=0;

  /* -----hh2 init - clean the second histogram --------- */

  for(i=0;i<3;i++){
    for(j=0;j<600;j++) { hist->extra.h2.Charge[i][j]=0;}
    for(j=0;j<150;j++) { hist->extra.h2.Peak[i][j]=0;}
  }

  hist->extra.h2.has_histo=0;
  hist->extra.h2.mask=0;
  hist->extra.h2.ssdTh=0;
  hist->extra.h2.peakPos[0]=0;
  hist->extra.h2.peakPos[1]=0;
  /* -----hh2 end --------- */
}

void muoncalib_force_baseline_calc()
{
  gl.firsttime |= 0x2;
}

int muoncalib_calc_parameters(int peak[],int charge[],int base[],
                              int bin_max[],int trace[][NBBIN],
                              struct muoncalib_traceflags *flags,
                              uint32_t buff1[],uint32_t buff2[],int index)
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
  flags->trig_type &= 0x1F;

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

    for(pmt=0; pmt<4; pmt++){
      if(gl.ch_bin_min[ pmt ] <= bin &&
         bin < gl.ch_bin_max[ pmt ]    ){
        charge[pmt] += trace[pmt][bin];
      }

      if( peak[pmt] < trace[pmt][bin] ){
        peak[pmt] = trace[pmt][bin];
        bin_max[pmt] = bin;
      }
    }
  }
  return 0;
}


int muoncalib_finish(int offset_next[],
                     struct muon_histo_complete *curHisto,
                     int t_end)
{
  /* input: "curHisto" - histogram which would have been filled
   *
   * output: offset_next - the offset which should be used to
   *              contruct next histogram
   *
   * It will also some parts of curHisto.
   *
   * return 0.
   */
  int i,j;
  int offset_a,offset_b,offset;
  int base;
  curHisto->histo.EndSecond = t_end; /*muonbuffer->ttag_sec; */
  /* include the parameters used to construct the histograms */
  for(i=0;i<4;i++){
    curHisto->histo.Pk_bit_shift[i] = gl.pk_bits_shift[i];
    curHisto->histo.Ch_bit_shift[i] = gl.ch_bits_shift[i];

    curHisto->histo.Pk_bin[i][0]    = gl.pk_bin_min[i];
    curHisto->histo.Pk_bin[i][1]    = gl.pk_bin_max[i];

    curHisto->histo.Ch_bin[i][0]    = gl.ch_bin_min[i];
    curHisto->histo.Ch_bin[i][1]    = gl.ch_bin_max[i];
  }

  muoncalib_h2_get_histo(&gl.h2,&(curHisto->extra));

  /* calculate the base line (average) using the data of the baseline
   *    histogram
   * and also the offset to be used in the next histogram.
   */

  for (i=0;i<4;i++) {
    offset_a=0;
    offset_b=0;
    for (j=0;j<20;j++) {
      offset_a += curHisto->histo.Base[i][j]*j;
      offset_b += curHisto->histo.Base[i][j];
    }
    offset = curHisto->histo.Offset[i]; /* just to use later. */
    /*===== to be implemented in the next version ... ============== */
    if (offset_b>0){
      base=100*offset +
        100*offset_a/offset_b - 1000 ; /*1000 is because hist. of base
                                         has an offset of 10 ADC */
      if(base < 0xFFFF){
        curHisto->histo.BaseAvg[i]=base; /*this is to transmit the baseline */
      } else {
        curHisto->histo.BaseAvg[i]=0xFFFF;
      }
    }

    offset_a +=  -1*curHisto->extra.excess_bins[i][0];
    offset_a +=  20*curHisto->extra.excess_bins[i][1];
    offset_b +=  curHisto->extra.excess_bins[i][0] +
      curHisto->extra.excess_bins[i][1];

    if(offset_b!=0){
      offset_next[i] = offset + offset_a/offset_b - 10;
    } else {
      offset_next[i] = offset;
    }
  }

  /* copy the calibration histogram if it has been filled */
  if(gl.SSD_calib_ch.histos[1].StartSecond>0){
    memcpy(&(curHisto->extra.calibch),
           &(gl.SSD_calib_ch.histos[1]),
           sizeof(struct muon_calib));
  } else {
    memset(&(curHisto->extra.calibch),0,sizeof(struct muon_calib));
  }
  return(0);
}

int muoncalib_start_new_histo(struct muon_histo_complete *h,
                              int *offset,int t_start_next_histo)
{
  int i;
  muoncalib_ClearHisto(h);
  for(i=0;i<4;i++){
    h->histo.Offset[i]=offset[i];
  }
  h->histo.StartSecond = t_start_next_histo;

  /* -----hh2 init - start new histogram --------- */
  muoncalib_h2_new_histo(&gl.h2,t_start_next_histo);
  return(0);
}

int muoncalib_TreatBuffer(struct muon_histo_complete *histo_out,
                          struct muon_read_info *muonbuffer,
                          struct muonAcqStore_str *storeSsd,
                          int force_wr)
{
  /*return 1 in the case it has data to be added (the histo_out) has
   *          new information.
   *      0 the histo_out has nothing new.
   *   if force_wr==1: it will force to write the existing histogram
   *       and return 1.
   */

  unsigned int v;
  int nbbin=0;
  int baselinetimes64[4];
  int i,j,k;
  int nbbase;
  int retval;
  //struct histo *h;
  int peak[4],charge[4],b[4],vb[4],nbbin_max[4],shape[4][NBBIN];
  int offset_next[4];
  int to_include_h2;

  //static struct histo *CurrentHisto;
  static struct muon_histo_complete *CurrentHisto;
  struct muoncalib_debug_output_evt *debug_evt_pt;

  struct muoncalib_traceflags flags;
  retval=0;

  /*if it is the first time to run this part of the code, it need
   * to allocate the memory for this histograms
   *
   * and also to calculate the baseline offset.
   * This two things have been evaluated separatelly, because sometimes,
   * it get some problems while evaluating the baseline offset.
   */
  if (gl.firsttime & 0x1) {
    /* SSD special histogram for SiPM only XXX FIXME */

    for(i=0;i<4;i++){
      offset_next[i]=0;
    }
    CurrentHisto=malloc(sizeof(struct muon_histo_complete));
    muoncalib_ClearHisto(CurrentHisto);
    gl.firsttime^=0x1;
  }

  if(gl.firsttime & 0x2){
    CurrentHisto->histo.StartSecond = muonbuffer->ttag_sec;
    /* compute baselines: sum 64 times first bin */
    nbbase=0;
    for (i=0;i<4;i++)
      baselinetimes64[i]=0;
    for (j=0; j<8096 && nbbase<64;j += NBBIN + 1) {
      if(muoncalib_calc_parameters(peak,charge,b,nbbin_max,shape,&flags,
                                   muonbuffer->buff[0],muonbuffer->buff[1],
                                   j) ==0){
        if( (! flags.is_calib_channel ) && flags.trig_type==1 ){
          for(i=0; i<4; i++){
            baselinetimes64[i] += b[i];
          }
          nbbin++;
          nbbase++;
        }
      }
    }
    /* set offsets with that */
    if (nbbase>5){
      for (i=0;i<4;i++){
        baselinetimes64[i] /= nbbase;
      }
    } else {
      return(0); /*to get another data block to evaluate the baseline */
    }

    muoncalib_start_new_histo(CurrentHisto, baselinetimes64,
                              muonbuffer->ttag_sec);
    gl.firsttime^=0x2; /*the offset have been evaluated */
  }

  /* check for each new second if there is a full
   * (gl.integrationInterval + 1 ) second calibration period
   */
  if ( (muonbuffer->ttag_sec - CurrentHisto->histo.StartSecond >
        gl.integrationInterval) ||
       force_wr) {
    /* 61 seconds of calibration.
     * Go to Next Buffer: CurrentHisto->Next will be the new current.
     */
    printf("%d %d\n",muonbuffer->ttag_sec,CurrentHisto->histo.StartSecond);
    muoncalib_finish(offset_next, CurrentHisto,muonbuffer->ttag_sec);

    //printf("start debug\n");
    //if(debug->state==1){
    //  printf("MUONCALIB - treat signal. Start debug\n");
    //  debug->h.nevts=0;
    //  debug->h.evt_size=sizeof(struct muoncalib_debug_output_evt);
    //  debug->h.offset[0] = CurrentHisto->histo.Offset[0];
    //  debug->h.offset[1] = CurrentHisto->histo.Offset[1];
    //  debug->h.offset[2] = CurrentHisto->histo.Offset[2];
    //  debug->h.offset[3] = CurrentHisto->histo.Offset[3];
    //  debug->state=2;
    //}
    /*  Let's dump to disk the histograms */

    memcpy(histo_out,CurrentHisto,sizeof(struct muon_histo_complete));

    muoncalib_start_new_histo(CurrentHisto,offset_next,muonbuffer->ttag_sec);

    retval=1;
    if(force_wr){
      return(1);
    }
  }/*end of if (second-CurrentHisto->Next->StartSecond>60) */
  /* end of check for new calibration histogram period */

  /*=============================================================
   * really start to fill the calibrating histogram.
   *=============================================================
   */
  for (j=0;j<8096;j+=NBBIN+1) {
    if(muoncalib_calc_parameters(peak,charge,b,nbbin_max,shape,&flags,
                                 muonbuffer->buff[0],muonbuffer->buff[1],
                                 j)==0){
      /*
      printf("%d - %4d %4d %4d %4d; %4d %4d %4d %4d; %6d %6d %6d %6d\n",
             flags.is_calib_channel,
             b[0],b[1],b[2],b[3],
             peak[0],peak[1],peak[2],peak[3],
             charge[0],charge[1],charge[2],charge[3]);
      */
      /* new signal - process ...*/
      if(flags.is_calib_channel){
        /* The fourth signal is from the calibration channel
         * used by the SiPM to look the single photo-electron
         *
         * It is going to consider only the signal from the fourth
         * channel. It is may be not a really good idea, but
         * is much easier to implement.
         */
        muoncalib_ssd_entry(&(gl.SSD_calib_ch),charge[3],b[3],
                            muonbuffer->ttag_sec);
      } else {
        /*xxxxxxxxxx */
	if(flags.trig_type & 0xF){ /* consider single bin trig (the two def.)
				    * and the external trigger, but exclude
				    * calib channel
				    */
          muonAcqStore_entry(storeSsd,peak,charge,b,gl.ch_bins,
                             nbbin_max,CurrentHisto->histo.Offset,
                             shape,flags.trig_type,
                             muonbuffer->ttag_sec);
        }
	if(flags.trig_type & 0x1) {
          /* It is the usual muon trigger */
          /* The fourth signal is from the high gain channel */
          CurrentHisto->histo.NEntries++;

          //debug_evt_pt=NULL;
          //if( (debug->state == 3 || debug->state==2) &&
          //    debug->h.nevts < debug->nevt_max){
          //  debug_evt_pt=&(debug->evts[debug->h.nevts]);
          //  debug->h.nevts++;
          //}
          for (i=0;i<4;i++) {
            /* offsetting each value by baseline and bit shift it to
             * keep in range
             */
            b[i] = b[i] - CurrentHisto->histo.Offset[i];
            peak[i]=(peak[i] - CurrentHisto->histo.Offset[i]);
            charge[i]=(charge[i]- gl.ch_bins[i]*CurrentHisto->histo.Offset[i]);

            //if(debug_evt_pt!=NULL){
            //  debug_evt_pt->binmax[i] = nbbin_max[i];
            //  debug_evt_pt->b[i]      = b[i];
            //  debug_evt_pt->pk[i]     = peak[i];
            //  debug_evt_pt->ch[i]     = charge[i];
            //}
          }
          muoncalib_h2_entry(&gl.h2,peak,charge,nbbin_max);

          /* include the values into the histograms */
          for(i=0;i<4;i++){
            peak[i]   >>= gl.pk_bits_shift[i];
            charge[i] >>= gl.ch_bits_shift[i];

            /* saving where it corresponds
             * note: 60k entries enough for baseline */
            if(CurrentHisto->histo.NEntries<60000){
              if(b[i]< -10){
                CurrentHisto->extra.excess_bins[i][0]++;
              } else if (b[i]<10){ /* this is actually -10<=b[i]<10 */
                CurrentHisto->histo.Base[i][b[i]+10]++;
              } else {
                CurrentHisto->extra.excess_bins[i][1]++;
              }
            }
            if( gl.pk_bin_min[i] <= nbbin_max[i] &&
                nbbin_max[i] < gl.pk_bin_max[i] ){ /* position of peak */
              muoncalib_add_histo_entry(CurrentHisto->histo.Peak[i]  ,
                                        100,150,peak[i]  );
              muoncalib_add_histo_entry(CurrentHisto->histo.Charge[i],
                                        400,600,charge[i]);

              if ((100<=charge[i]) && (charge[i]<300)){
                for (k=0;k<NBBIN;k++){
                  CurrentHisto->histo.Shape[i][k] +=
                    shape[i][k] - CurrentHisto->histo.Offset[i];
                }
              }
            }
          }
	}
      }
    }
  }
  return retval;
}
