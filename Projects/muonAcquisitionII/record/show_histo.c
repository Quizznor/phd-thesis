#include <muonfill.h>

main()
{
  FILE *arq;
  struct muon_histo h;
  int i,j;
  arq=fopen("h","r");
  if(arq!=NULL){
    if(fread(&histo,sizeof(struct muon_histo),1,arq)==1){
      printf("#(Start,end,N)=(%d,%d,%d)\n",h.StartSecond,h.EndSecond,h.NEntries);
      printf("#offset: %d %d %d %d\n",
	     h.Offset[0],h.Offset[1],h.Offset[2],h.Offset[3]);
      printf("# Base\n");
      for(i=0;i<20;i++){
	printf("%2d  %d  %d  %d  %d\n",
	       i,h.Base[0][i],h.Base[1][i],h.Base[2][i],h.Base[3][i]);
      }
      printf("# Peak\n");
      for(i=0;i<150;i++){
	printf("%2d  %d  %d  %d  %d\n",
	       i,h.Peak[0][i],h.Peak[1][i],h.Peak[2][i],h.Peak[3][i]);
      }
      printf("# Charge\n");
      for(i=0;i<600;i++){
	printf("%2d  %d  %d  %d  %d\n",
	       i,h.Charge[0][i],h.Charge[1][i],h.Charge[2][i],h.Charge[3][i]);
      }
      printf("# Shape\n");
      for(i=0;i<NBBIN;i++){
	printf("%2d  %d  %d  %d  %d\n",
	       i,h.Shape[0][i],h.Shape[1][i],h.Shape[2][i],h.Shape[3][i]);
      }
      
      
    }
    
    fclose(arq);
    
  }
}
