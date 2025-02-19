#include "muonfill.h"

int main(int argc,char *argv[])
{
  struct muon_calib sipm;
  FILE *arq;

  arq=fopen(argv[1],"r");
  if(arq!=NULL){
    if(fread(&sipm,sizeof(sipm),1,arq)==1){
      printf("#(start, end, Offset): %d %d %d\n",
	     sipm.StartSecond,sipm.dtSecond,sipm.Offset);
      printf("#Charge ");
      for(i=0;i<600;i++){
	printf("%d %d\n",i,sipm.Charge[i]);
      }
      printf("#Base");
      for(i=0;i<20;i++){
	printf("%d %d\n",i,sipm.Base[i]);
      }
      printf("excess: %d %d\n",sipm.Base_excess[0],sipm.Base_excess[1]);
    }
    
  }
    
}
