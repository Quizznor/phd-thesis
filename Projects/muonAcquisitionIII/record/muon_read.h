#ifndef _MUON_READ_H
#define _MUON_READ_H
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "xparameters.h"
#include "sde_trigger_defs.h"

struct muon_read_info
{
  uint32_t tag_a,tag_b,mask,st,nw;
  uint32_t ttag_sec,ttag_ticks,ttag_pps_sec,ttag_pps_ticks;
  uint32_t buff[2][MUON_MEM_WORDS];
};

struct muon_read_uio_str
{
  int use_uio;
  int fd;
  uint32_t *mem;
  int size;

  fd_set fds;
};
struct muon_read_str
{
  uint32_t *regs;
  int reg_size;
  uint32_t *ttag;
  int ttag_size;
  uint32_t *muon_buff[2];
  int muon_buff_size[2];

  struct muon_read_uio_str uio;
};


struct muon_read_str *muon_read_init();
void muon_read_finish(struct muon_read_str *str);

int muon_read_search_baseline(struct muon_read_str *str,struct timespec *talive); /*it would be used only at the begining of process */

void muon_read_get_config(struct muon_read_str *str,uint32_t *params);


void muon_read_set_threshold(struct muon_read_str *str,int *th);
void muon_read_set_enable(struct muon_read_str *str,int mask,int ncoinc);
int muon_read_get(struct muon_read_str *str,struct muon_read_info *buff);
void muon_read_set_enable_calib_ch(struct muon_read_str *str,int enable);
void muon_read_set2(struct muon_read_str *str,int *th,uint32_t enableFlags,int enable);
#endif
