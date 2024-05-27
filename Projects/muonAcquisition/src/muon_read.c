#include <stdlib.h>
#include <sys/select.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include <time_tagging.h>

#include "muon_read.h"
#include "linux_uio.h"

/*the following function might be one generical function
  to be used to open connection with front-end */
void *muon_read_map(char *device,uint32_t offset,int *size,int open_flag)
{
  int aux,fd;
  void *addr;
  printf("open device %s;  offset=%x \n",device,offset);
  fd=open(device,open_flag);
  if(fd<0){
    printf("open device %s  error!!! \n",device);
    *size=0;
    return(NULL);
  }
  aux=*size;
  if(aux%sysconf(_SC_PAGE_SIZE)){
    /*rounding the size up to be multiple of _SC_PAGE_SIZE */
    aux=((aux+sysconf(_SC_PAGE_SIZE)-1)/sysconf(_SC_PAGE_SIZE))*sysconf(_SC_PAGE_SIZE);
  }
  *size=aux;
  if(open_flag==O_RDWR){
    addr=(void *)mmap(NULL, aux,
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd,offset);
  } else {
    addr=(void *)mmap(NULL, aux,
                      PROT_READ , MAP_SHARED,
                      fd,offset);
  }
  if(addr==MAP_FAILED){
    printf("Error - while trying to map the Registers %08x %08x %d\n",
           offset,*size,fd);
    exit(1);
  }
  close(fd); /*it is not needed to keep opened */
  return addr;
}
void muon_read_devmen_init(struct muon_read_str *str)
{
  int size;
  size=256*sizeof(uint32_t);
  str->regs=muon_read_map("/dev/mem",SDE_TRIGGER_BASE,&size,O_RDWR);
  str->reg_size=size;
  str->ttag=muon_read_map("/dev/mem",TIME_TAGGING_BASE,&size,O_RDONLY);
  str->ttag_size=size;

  size=MUON_MEM_WORDS*sizeof(uint32_t)*MUON_MEM_NBUF;
  str->muon_buff[0]=muon_read_map("/dev/mem", TRIGGER_MEMORY_MUON0_BASE,
                                  &size,O_RDONLY);
  str->muon_buff_size[0]=size;
  size=MUON_MEM_WORDS*sizeof(uint32_t)*MUON_MEM_NBUF;
  str->muon_buff[1]=muon_read_map("/dev/mem",TRIGGER_MEMORY_MUON1_BASE,
                                  &size,O_RDONLY);
  str->muon_buff_size[1]=size;
}

void muon_read_uio_init(struct muon_read_str *str)
{
  int fd,size;
  void *addr;
  uint32_t trig_aux;

  size=256*sizeof(uint32_t);
  str->regs=muon_read_map(UIO_CTRL_SDE,0,&size,O_RDWR);
  str->reg_size=size;
  str->ttag=muon_read_map(UIO_CTRL_TTAG,0,&size,O_RDONLY);
  str->ttag_size=size;

  size=MUON_MEM_WORDS*sizeof(uint32_t)*MUON_MEM_NBUF;
  str->muon_buff[0]=muon_read_map(UIO_BUFF_MUON_0,0,&size,O_RDONLY);
  str->muon_buff_size[0]=size;

  size=MUON_MEM_WORDS*sizeof(uint32_t)*MUON_MEM_NBUF;
  str->muon_buff[1]=muon_read_map(UIO_BUFF_MUON_1,0,&size,O_RDONLY);
  str->muon_buff_size[1]=size;

  str->uio.fd=open(UIO_INTERRUPT_MUON,O_RDWR);
  if(str->uio.fd>0){
    size=sysconf(_SC_PAGE_SIZE);
    addr=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,str->uio.fd,0);
    if(addr!=MAP_FAILED){
      str->uio.mem=(uint32_t *)addr;
      str->uio.size=size;
    } else {
      close(str->uio.fd);
      str->uio.mem=NULL;
      str->uio.size=0;
    }
    FD_ZERO(&str->uio.fds);
    FD_SET(str->uio.fd, &str->uio.fds);

    str->uio.mem[INTR_GLOBAL_EN_ADDR]=1;
    str->uio.mem[INTR_EN_ADDR]=1;
    trig_aux=1;
    write(str->uio.fd,&trig_aux,sizeof(uint32_t));
  } else {
    printf("MUONFILL ---- problem to open %s device\n",UIO_INTERRUPT_MUON);
  }
  str->uio.use_uio=1;
}

struct muon_read_str *muon_read_init(int with_uio)
{
  int size;
  struct muon_read_str *str;

  str=(struct muon_read_str *)malloc(sizeof(struct muon_read_str));
  if(str==NULL)
    return NULL;
  if(with_uio){
    muon_read_uio_init(str);
  } else {
    muon_read_devmen_init(str);
  }

  /*set muon buffer trigger */
  /*set threshold */
}

void muon_read_finish(struct muon_read_str *str)
{
  int i;
  if(str!=NULL){
    if(str->regs !=NULL){
      munmap(str->regs,str->reg_size);
    }
    if(str->ttag !=NULL){
      munmap(str->ttag,str->ttag_size);
    }
    for(i=0;i<2;i++){
      if(str->muon_buff[i]!=NULL)
        munmap(str->muon_buff[i],str->muon_buff_size[i]);
    }
    if(str->uio.use_uio){
      munmap(str->uio.mem,str->uio.size);
      close(str->uio.fd);
    }
    free(str);
  }
}
void muon_read_get_config(struct muon_read_str *str,uint32_t *params)
{
  params[0]=str->regs[MUON_TRIG1_THR0_ADDR];
  params[1]=str->regs[MUON_TRIG1_THR1_ADDR];
  params[2]=str->regs[MUON_TRIG1_THR2_ADDR];
  params[3]=str->regs[MUON_TRIG1_SSD_ADDR];
  params[4]=str->regs[MUON_TRIG1_ENAB_ADDR];
  params[5]=str->regs[MUON_TRIG2_THR0_ADDR];
  params[6]=str->regs[MUON_TRIG2_THR1_ADDR];
  params[7]=str->regs[MUON_TRIG2_THR2_ADDR];
  params[8]=str->regs[MUON_TRIG2_SSD_ADDR];
  params[9]=str->regs[MUON_TRIG2_ENAB_ADDR];
  params[10]=str->regs[MUON_BUF_TRIG_MASK_ADDR];
}
void muon_read_set_threshold(struct muon_read_str *str,int *th)
{
  str->regs[MUON_TRIG1_THR0_ADDR]=th[0] & 0xFFF;
  str->regs[MUON_TRIG1_THR1_ADDR]=th[1] & 0xFFF;
  str->regs[MUON_TRIG1_THR2_ADDR]=th[2] & 0xFFF;
  str->regs[MUON_TRIG1_SSD_ADDR]=th[3] & 0xFFF;
}

void muon_read_set_enable(struct muon_read_str *str,int pmt_enable,
                          int ncoinc)
{
  int npmt;
  str->regs[MUON_TRIG1_ENAB_ADDR]=
    0x4000 | /* no sign. overlap; 2 bins above threshold. */
    //0x4C00 | /* 4 bins  overlap; 2 bins above threshold. */
    //0x4400 | /* 4 bins  overlap; 2 bins above threshold. */
    (pmt_enable & 0xF) | /* which PMTs should consider in trig.
                          * bit 0 - WCD PMT1;
                          * bit 1 - WCD PMT2;
                          * bit 2 - WCD PMT3;
                          * bit 3 - SSD PMT
                          */
    ((ncoinc & 0x7)<<4); /* number of PMTs should be in coincidence */

  if(pmt_enable & 0xF){ /*there are, at least, one PMT in the list */
    str->regs[MUON_BUF_TRIG_MASK_ADDR] |= 1;
  } else {
    str->regs[MUON_BUF_TRIG_MASK_ADDR] &= ~1;
  }
}

void muon_read_set_enable_calib_ch(struct muon_read_str *str,int enable)
{
  if(enable){
    //str->regs[MUON_BUF_TRIG_MASK_ADDR] |= 1<<MUON_BUF_SIPM_CAL_SHIFT;
    str->regs[MUON_BUF_TRIG_MASK_ADDR] |= 1<<5;
  } else {
    //str->regs[MUON_BUF_TRIG_MASK_ADDR] &= ~(1<<MUON_BUF_SIPM_CAL_SHIFT);
    str->regs[MUON_BUF_TRIG_MASK_ADDR] &= ~(1<<5);
  }
}

void muon_read_set2(struct muon_read_str *str,int *th,uint32_t enableFlags,int enable)
{
  str->regs[MUON_TRIG2_THR0_ADDR]=th[0] & 0xFFF;
  str->regs[MUON_TRIG2_THR1_ADDR]=th[1] & 0xFFF;
  str->regs[MUON_TRIG2_THR2_ADDR]=th[2] & 0xFFF;
  str->regs[MUON_TRIG2_SSD_ADDR]=th[3] & 0xFFF;
  str->regs[MUON_TRIG2_ENAB_ADDR]=enableFlags;
  if(enable){
    str->regs[MUON_BUF_TRIG_MASK_ADDR ] |= 1<<MUON_BUF_TRIG_SB2_SHIFT;
  } else {
    str->regs[MUON_BUF_TRIG_MASK_ADDR ] &= ~(1<<MUON_BUF_TRIG_SB2_SHIFT);
  }
}


int muon_read_search_baseline(struct muon_read_str *str,struct timespec *talive)
{
  /* search the baseline and set the initial threshold for the
   *  muon buffer.
   *
   *  talive - is to says the process is still alive. Sometimes, the
   *    function spent more than the process need to say it is alive.
   *    Therefore It need to update the parameter often ...
   */
  struct muon_read_info buff;
  uint32_t v1,v2;
  int th[4];
  int base[4],nev;
  int i,j,k;
  th[0]=0;
  th[1]=0;
  th[2]=0;
  th[3]=0;

  muon_read_set_threshold(str,th);
  muon_read_set_enable(str,0x7,1); /* set three WCD PMTs in trigger;
                                    * multiplicity = 1. It is Ok to
                                    * look baseline
                                    */
  for(i=0;i<64 && (j=muon_read_get(str,&buff))==1; i++){
    clock_gettime(CLOCK_MONOTONIC_COARSE,talive);
    for(k=0;k<4;k++){
      th[k]=(th[k]+64) & 0xFFF;
    }
    printf("muon_read - search baseline. Threshold: %d, %d, %d\n",
           th[2],th[1],th[0]);
    muon_read_set_threshold(str,th);
  }
  clock_gettime(CLOCK_MONOTONIC_COARSE,talive);
  muon_read_set_enable(str,0,0); /*disable the trigger*/

  if(j!=0){
    return(1);
  }
  base[0]=0;
  base[1]=0;
  base[2]=0;
  base[3]=0;
  nev=0;
  j=-1;
  for(i=0;i<MUON_MEM_WORDS;i++){
    v1=buff.buff[0][i];
    v2=buff.buff[1][i];
    if(v1>>31){
      j=0;
    }
    if(j==1){
      base[0]+= v1      & 0xFFF;
      base[1]+=(v1>>16) & 0xFFF;
      base[2]+= v2      & 0xFFF;
      base[3]+=(v2>>16) & 0xFFF;
      nev++;
    }
    if(j>=0){
      j++;
    }
  }
  if(nev==0){
    return(1);
  }
  for(i=0;i<4;i++){
    th[i]=(base[i]/nev)+30;
  }
  /*Set the threshold in a better way and enable the trigger */
  muon_read_set_threshold(str,th);
  muon_read_set_enable(str,0x7,1);
  return(0);
}


int muon_read_get(struct muon_read_str *str,struct muon_read_info *buff)
{
  int ntimes;
  volatile uint32_t *st;
  uint32_t mask_nfull,uio_trig;
  int rd,offset;

  fd_set fds_aux;
  struct timeval timeout;

  st=&(str->regs[MUON_BUF_STATUS_ADDR]);
  mask_nfull=MUON_BUF_NFULL_MASK<<MUON_BUF_NFULL_SHIFT;
  //mask_nfull=7<<8;
  /*this is done in this way because it has a bug in
    MUON_BUF_NFULL_MASK parameter. It would be
    MUON_BUF_NFULL_MASK<<MUON_BUF_NFULL_SHIFT
  */
  ntimes=0;

  if(str->uio.use_uio){
    timeout.tv_sec=1;
    timeout.tv_usec=0;
    fds_aux=str->uio.fds;
    if(select(str->uio.fd+1, &fds_aux, NULL,NULL, &timeout)>0){
      if(FD_ISSET(str->uio.fd, &fds_aux)){
        read(str->uio.fd,&uio_trig,sizeof(uint32_t));
      }
    }

  } else {
    while(((*st) & mask_nfull)==0 && ntimes<100 ){
      usleep(10000); /*it is not really critical, therefore, it may be done
                       with a simple usleep */
      ntimes++;
    }
  }
  if (((*st)&mask_nfull)==0){
    //printf("returning no data ...\n");
    return(1);
  }

  rd=(*st & MUON_BUF_RNUM_MASK) >> MUON_BUF_RNUM_SHIFT;

  buff->tag_a=str->regs[MUON_BUF_TIME_TAG_A_ADDR];
  buff->tag_b=str->regs[MUON_BUF_TIME_TAG_B_ADDR];
  buff->mask=str->regs[MUON_BUF_TRIG_MASK_ADDR];
  buff->st=str->regs[MUON_BUF_STATUS_ADDR];
  buff->nw=str->regs[MUON_BUF_WORD_COUNT_ADDR];

  buff->ttag_sec      =str->ttag[TTAG_MUON_SECONDS_ADDR];
  buff->ttag_ticks    =str->ttag[TTAG_MUON_NANOSEC_ADDR];
  buff->ttag_pps_ticks=str->ttag[TTAG_MUON_PPS_NANOSEC_ADDR];

  //buff->pps_sec = buff->ttag_sec;/*it was using the pps sec instead,
  //                               because the TTAG for muon was not
  //                               working. Now it looks fine. */
  // in principle it is not needed ...


  offset=rd*MUON_MEM_WORDS;
  memcpy(buff->buff[0],(str->muon_buff[0]+offset),
         MUON_MEM_WORDS*sizeof(uint32_t));
  memcpy(buff->buff[1],(str->muon_buff[1]+offset),
         MUON_MEM_WORDS*sizeof(uint32_t));

  /*reset the muon buffer of FPGA */
  str->regs[MUON_BUF_CONTROL_ADDR]=rd;

  if(str->uio.use_uio){
    str->uio.mem[INTR_ACK_ADDR]=1;
    uio_trig=1;
    write(str->uio.fd,&uio_trig,sizeof(uint32_t));
  }

  return(0);
}
