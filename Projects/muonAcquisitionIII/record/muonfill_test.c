
#include <stdio.h>
#include <stdlib.h>

#include <muonfill.h>
#include <status.h>
#include <run_config.h>


#include <logfile.h>

#include <string.h>

#include "muoncalib.h"
#include "muon_read.h"


void use_as(char *cmd)
{
  printf("use as: %s <raw muon traces>\n",cmd);
  exit(0);
}

void write_histo(struct muon_histo *histo)
{
  FILE *arq;
  arq=fopen("histos","w");
  if(arq!=NULL){
    fwrite(histo,sizeof(struct muon_histo),1,arq);
    fclose(arq);
  }
}


int main(int argc,char *argv[])
{
  FILE *arq;
  struct muon_histo *histo;
  struct muon_read_info muon_buff;
  int threshold[4],nbuffs;

  if(argc<2){
    use_as(argv[0]);
  }    
  arq=fopen(argv[1],"r");
  if(arq==NULL){
    printf("Not possible to open the file %s\n",argv[1]);
    return(1);
  }
  muoncalib_init();
  histo=(struct muon_histo *)malloc(sizeof(struct muon_histo));

  printf("start read ...\n");
  fflush(stdout);
  nbuffs=0;
  while(fread(&muon_buff,sizeof(muon_buff),1,arq)==1){
    if((nbuffs+1)%2==0) printf("nbuffs=%d\n",nbuffs);
    nbuffs++;
    if(muoncalib_TreatBuffer(histo,&muon_buff,0)==1){
      printf("going to write histogram ...\n");
      fflush(stdout);
      write_histo(histo);
    }
  }
  printf("before write the histogram ...\n");
  fflush(stdout);
  if(muoncalib_TreatBuffer(histo,&muon_buff,1)==1){
    write_histo(histo);
  }
}


