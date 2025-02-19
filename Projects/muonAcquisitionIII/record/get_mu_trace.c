#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "muon_read.h"

#define NBUFFMAX 100
int main()
{
  struct muon_read_str *read_buff;
  struct muon_read_info *muon_buff;
  char *pt;
  int nbuff;
  int fd_wr,nw,aux;
  fd_wr=open("muon_buff.bin",O_RDWR|O_CREAT);
  if(fd_wr<0){
    printf("not possible to open the file muon_buff.bin to write\n");
    return(1);
  }
  read_buff=muon_read_init();
  //muon_read_search_baseline(read_buff);
  //muon_read_set_enable_calib_ch(read_buff,1);
  muon_buff=(struct muon_read_info *)
    malloc(NBUFFMAX * sizeof(struct muon_read_info));
  printf("Get traces\n");
  for(nbuff=0;nbuff<NBUFFMAX;nbuff++){
    while(muon_read_get(read_buff,&(muon_buff[nbuff]))!=0);
  }
  printf("Store traces\n");
  fflush(stdout);
  for(nbuff=0;nbuff<NBUFFMAX;nbuff++){
    pt=(char *) &(muon_buff[nbuff]);
    nw=0;
    while(nw<sizeof(struct muon_read_info)){
      aux=write(fd_wr,pt+nw,sizeof(struct muon_read_info)-nw);
      if(aux>=0){
        nw+=aux;
      } else {
        printf("some error while try to write ...\n");
        return(1);
      }
    }
    printf("%d\n",nbuff);
  }
  close(fd_wr);
  return(0);
}
