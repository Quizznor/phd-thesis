#ifndef _MUONACQSTORE_H_
#define _MUONACQSTORE_H_

#include "muonAcqFmt.h"
#include "muon_read.h" 
#include "muonfill.h"
#include <stdio.h>

#define MUONACQSTORE_BUFFSIZE 65536
struct muonAcqStore_str
{
  int maxSize;
  int maxBuffSize;
  int maxBlockSize;
  int nextBlockSize;
  
  int ssdTh;
  int mode;
  FILE *file;
  uint8_t *buff;

  int headSize;
  struct muonAcqFmt_H *head;
  uint8_t *dataPt; /* its adress should be just after the header 
                    * ( |preamble|muonAcqFmt_H|muonAcqFmt_xxxx|....| 
                    *                          ^
                    */

  int histStored; //if the histogram has already been stored
  int storedSize;

  /* look the stop condition 
   * It should consider the stop condition, when the number
   *  of events stored is bigger than nEvts or the acquired time
   *  interval is bigger than deltaTime
   *  or when the amount of data is bigger than maxSize
   * in whatever these cases finish is set to 0.
   */

  int deltaTime;
  int nEvts;
  int finish;
};
void muonAcqStore_init(struct muonAcqStore_str *str,int ssdTh,int maxSize,
                       int dtStop,int nEvtsStop);
int muonAcqStore_addHist(struct muonAcqStore_str *str,
			 struct muon_histo_complete *h,
                         struct muon_read_str *read_buff);
int muonAcqStore_entry(struct muonAcqStore_str *str ,
		       int peak[],int charge[],int base[],
		       int charge_bins[],
		       int bin_max[],uint16_t Offset[],
                       int trace[][NBBIN],
		       int trigType,
                       int currentTime //the time
		       );
#endif
