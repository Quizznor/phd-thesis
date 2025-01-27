#ifndef _MUONACQSTORE_H_
#define _MUONACQSTORE_H_

#include "muonAcqFmt_str.h"
#include "muonfill.h"
struct muonAcqStore_str
{
  int maxSize;
  int maxBuffSize;
  int maxBlockSize;
  int ssdTh;
  uint8_t *buff;
  struct muonAcqFmt_str *head;
  uint8_t *dataPt;
  int histStored;
  int storedSize;
};
void muonAcqStore_init(struct muonAcqStore *str,int ssdTh,int maxSize);
void muonAcqStore_AddHist(struct muonAcqStore *str,
			  struct muon_histo_complete *h);
void muonAcqStore_entry(...);
#endif
