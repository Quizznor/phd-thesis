#include <stdio.h>
#include "muonfill.h"

int main(int argc,char *argv[])
{
  FILE *arq;
  struct muon_histo_complete hc;
  struct muon_histo *h;
  struct muon_extra_histo *h2;
  int i,j;
  char *fname;
  if(argc>1){
    fname=argv[1];
  } else {
    fname="/home/root/daq/histos";
  }
  arq=fopen(fname,"r");

  if(arq!=NULL){
    if(fread(&hc,sizeof(struct muon_histo_complete),1,arq)==1){
      h=&(hc.histo);
      h2=&(hc.extra.h2);
      printf("#(Start,end,N)=(%d,%d,%d)\n",h->StartSecond,h->EndSecond,h->NEntries);
      printf("#offset: %d %d %d %d\n",
	     h->Offset[0],h->Offset[1],h->Offset[2],h->Offset[3]);
      //printf("#BaseAvg: %d %d %d %d\n",
      //	     h->BaseAvg[0],h->BaseAvg[1],h->BaseAvg[2],h->BaseAvg[3]);

      printf("# Base\n");
      for(i=0;i<20;i++){
	printf("%2d  %6d  %6d  %6d  %6d \n",
	       i,h->Base[0][i],h->Base[1][i],h->Base[2][i],h->Base[3][i]);

      }
      printf("# Peak\n");
      for(i=0;i<150;i++){
	printf("%2d  %6d  %6d  %6d  %6d    %6d  %6d  %6d\n",
	       i,h->Peak[0][i],h->Peak[1][i],h->Peak[2][i],h->Peak[3][i],
	       h2->Peak[0][i],h2->Peak[1][i],h2->Peak[2][i]);
      }
      printf("# Charge\n");
      for(i=0;i<600;i++){
	printf("%2d  %6d  %6d  %6d  %6d    %6d  %6d  %6d\n",
	       i,h->Charge[0][i],h->Charge[1][i],h->Charge[2][i],h->Charge[3][i],
	       h2->Charge[0][i],h2->Charge[1][i],h2->Charge[2][i]);
      }
      printf("# Shape\n");
      for(i=0;i<NBBIN;i++){
	printf("%2d  %6d  %6d  %6d  %6d\n",
	       i,h->Shape[0][i],h->Shape[1][i],h->Shape[2][i],h->Shape[3][i]);
      }
      
      
    }
    
    fclose(arq);
  } else {
    printf("Not possible to open the file %s\n",fname);
  }
  return(0);
}
