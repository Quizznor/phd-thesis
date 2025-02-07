#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "muonAcqFmt.h"
#include "muonfill.h"

void print_hist(struct muon_histo *h)
{
  int i;
  printf("h_type=%d\n",h->h_type);
  printf("pk shift: %d %d %d %d\n",
         h->Pk_bit_shift[0],
         h->Pk_bit_shift[1],
         h->Pk_bit_shift[2],
         h->Pk_bit_shift[3]);

  printf("Ch shift: %d %d %d %d\n",
         h->Ch_bit_shift[0],
         h->Ch_bit_shift[1],
         h->Ch_bit_shift[2],
         h->Ch_bit_shift[3]);

  printf("pk start/end: %d-%d %d-%d %d-%d %d-%d\n",
         h->Pk_bin[0][0],h->Pk_bin[0][1],
         h->Pk_bin[1][0],h->Pk_bin[1][1],
         h->Pk_bin[2][0],h->Pk_bin[2][1],
         h->Pk_bin[3][0],h->Pk_bin[3][1] );

  printf("pk start/end: %d-%d %d-%d %d-%d %d-%d\n",
         h->Ch_bin[0][0],h->Ch_bin[0][1],
         h->Ch_bin[1][0],h->Ch_bin[1][1],
         h->Ch_bin[2][0],h->Ch_bin[2][1],
         h->Ch_bin[3][0],h->Ch_bin[3][1] );

  printf("Base Avg: %d %d %d %d\n",
         h->BaseAvg[0],
         h->BaseAvg[1],
         h->BaseAvg[2],
         h->BaseAvg[3]);
  printf("Offset: %d %d %d %d\n",
         h->Offset[0],
         h->Offset[1],
         h->Offset[2],
         h->Offset[3]);

  printf("Base line histo\n");
  for(i=0;i<20;i++){
    printf("%d  %d %d %d %d\n",i,
           h->Base[0][i],
           h->Base[1][i],
           h->Base[2][i],
           h->Base[3][i]);
  }

  printf("Peak\n");
  for(i=0;i<150;i++){
    printf("%d   %d %d %d %d\n",i,
           h->Peak[0][i],
           h->Peak[1][i],
           h->Peak[2][i],
           h->Peak[3][i]);
  }

  printf("Charge\n");
  for(i=0;i<600;i++){
    printf("%d   %d %d %d %d\n",i,
           h->Charge[0][i],
           h->Charge[1][i],
           h->Charge[2][i],
           h->Charge[3][i]);
  }
  printf("Shape\n");
  for(i=0;i<NBBIN;i++){
    printf("%d  %d %d %d %d\n",i,
           h->Shape[0][i],
           h->Shape[1][i],
           h->Shape[2][i],
           h->Shape[3][i]);
  }
}

void print_trace(char *buff,int n)
{
  int ev,t;
  uint16_t *pt,*pt1;
  uint16_t sig[4][NBBIN];
  pt=(uint16_t *)buff;
  for(ev=0;ev<n;ev++){
    printf("Event: %d %p\n",ev,pt);
    memcpy(sig[0],pt,sizeof(uint16_t)*NBBIN);
    pt+=NBBIN;
    memcpy(sig[1],pt,sizeof(uint16_t)*NBBIN);
    pt+=NBBIN;
    memcpy(sig[2],pt,sizeof(uint16_t)*NBBIN);
    pt+=NBBIN;
    memcpy(sig[3],pt,sizeof(uint16_t)*NBBIN);
    pt+=NBBIN;
    
    for(t=0;t<NBBIN;t++){
      printf("%2d  %4d %4d %4d %4d\n",t,
             sig[0][t],sig[1][t],sig[2][t],sig[3][t]);
    }
  }
}
void print_ssdHisto(char *buff)
{
  uint32_t *pt;
  int i;
  pt=(uint32_t *)buff;
  for(i=0;i<1024;i++){
    printf("%4d %d\n",i,*(pt+i));
  }
}

void print_param(char *buff,int n)
{
  struct dat_param{
    uint16_t p,ch,b,max;
  } *dat;
  
  int i,ev;
  uint16_t *pt;

  pt=(uint16_t *)buff;
  
  for(ev=0;ev<n;ev++){
    printf("ev: %d  -- trig: %d\n",ev,*pt );
    dat=(struct dat_param *)(pt+1);
    for(i=0;i<4;i++){
      printf("   pmt%d - %d %d %d %d\n",
             i,dat->p,dat->ch,dat->b,dat->max);
      dat++;
    }
    pt=(uint16_t *)(dat);
  }
}


int main(int argc,char *argv[])
{
  FILE *f;
  char preamble[16];
  struct muonAcqFmt_H h;
  struct muonAcqFmt_tHist *histh;
  struct muonAcqFmt_tTrace *trace;
  struct muonAcqFmt_tSsdHisto *ssdhist;
  struct muonAcqFmt_tParam *param;
  struct muon_histo muonH;
  int flag;
  char buff[65536];
  int Ok=0;
  if(argc<2){
    printf("Use as: %s <stored file>\n",argv[0]);
    return(1);
  }
  f=fopen(argv[1],"r");
  if(f!=NULL){
    Ok=1;
    while(fread(preamble,1,8,f)==8 && Ok){
      if(memcmp(preamble,"muonAcq!",8)==0){
        fread(&h,sizeof(struct muonAcqFmt_H),1,f);
        printf("header: \n");
        printf("%d %d %d %d\n",h.Tstart,h.timeInterval,h.type,h.size);
        if(h.size<=65536){
          flag=fread(buff,1,h.size,f);
          if(flag!=h.size){
            printf("Error while reading buff.\n");
            exit(1);
          }
        }
        switch (h.type){
        case emuonAcqFmt_hist:
          //flag=fread(&muonH,sizeof(struct muon_histo),1,f);
          //memcpy(muonH,buff,sizeof(struct muon_histo));
          print_hist((struct muon_histo*)buff);
          break;
        case emuonAcqFmt_trace:
          trace=(struct muonAcqFmt_tTrace *)(buff);
          print_trace(buff+sizeof(struct muonAcqFmt_tTrace),trace->nentries);
          printf("Traces: entries: %d\n",trace->nentries);
          break;
        case emuonAcqFmt_ssdHisto:
          ssdhist=(struct muonAcqFmt_tSsdHisto *)buff;
          printf("======ssd: \n  entries: %d; peakmin/max: %d %d; count under/over=%d/%d\n",
                 ssdhist->nentries,
                 ssdhist->pkmin,ssdhist->pkmax,
                 ssdhist->nPkUnder,ssdhist->nPkOver);
          print_ssdHisto(buff+sizeof(struct muonAcqFmt_tSsdHisto));
          break;
        case emuonAcqFmt_param:
          param=(struct muonAcqFmt_tParam *)buff;
          printf("params: entries: %d; base=%d,%d,%d,%d\n",
                 param->nentries,
                 param->chbase[0],param->chbase[1],
                 param->chbase[2],param->chbase[3]);
          print_param(buff+sizeof(struct muonAcqFmt_tParam),param->nentries);
          break;
        default:
          printf("error\n");
        }
      } else {
        Ok=0;
      }
    }
  }
}

