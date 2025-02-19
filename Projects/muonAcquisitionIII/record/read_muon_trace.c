#include <stdio.h>
#include "muon_read.h"

#define max_bin_base 10

int main(int argc,char *argv[])
{
  struct muon_read_info buff;
  FILE *arq;
  int bin,i,j;
  uint32_t v1,v2;
  int aux[4],flag,b[4],p[4],p_bin[4],A[4],na[4];
  int sig,prev_flag,b_avg;
  if(argc>1){
    arq=fopen(argv[1],"r");
    if(arq!=NULL){
      sig=0;
      do{
	flag=fread(&buff,sizeof(buff),1,arq);
	if(flag==1){
	  bin=0;
	  for(i=0;i<MUON_MEM_WORDS;i++){
	    v1=buff.buff[0][i];
	    v2=buff.buff[1][i];
	    if(v1&0x80000000){
	      bin=0;
	      //printf("=== %08x %08x \n",v1,v2);
	      if(sig>0){
		printf("%5d  ",sig);
		for(j=0;j<4;j++){
		  b_avg=b[j]/max_bin_base;
		  printf("%4d %4d %4d  %2d %5d  ",b_avg,p[j],p[j]-b_avg,p_bin[j],
			 A[j]-b[j]*na[j]/max_bin_base);
		  b[j]=0;
		  p[j]=0;
		  A[j]=0;
		  na[j]=0;
		}
		printf("%08x\n",prev_flag);
	      }
	      prev_flag=v2;
	      sig++;
	    } else {
	      aux[0]=v1&0xFFF;
	      aux[1]=(v1>>16)&0xFFF;
	      aux[2]=(v2)&0xFFF;
	      aux[3]=(v2>>16)&0xFFF;
	      if(bin<max_bin_base){
		for(j=0;j<4;j++){
		  b[j]+=aux[j];
		}
	      }
	      if(bin>=max_bin_base){
		for(j=0;j<4;j++){
		  if(p[j]<aux[j]){
		    p[j]=aux[j];
		    p_bin[j]=bin;
		  }
		}
		if(15<bin && bin<69){
		  for(j=0;j<3;j++){
		    A[j]+=aux[j];
		    na[j]++;
		  }
		}
		if(10<=bin && bin<30){
		  A[3]+=aux[3];
		  na[3]++;
		}
	      }
	      
	      //printf("%2d  %4d %4d %4d   %4d \n",bin,aux[0],aux[1],aux[2],aux[3]);
	      bin++;
	    }
	  }
	}
      } while(flag==1);
    }
  }
}
