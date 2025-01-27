#ifndef _MUONACQFMT_H_
#define _MUONACQFMT_H_

/* the output format should be as:
 *  |preamble|muonAcqFmt_H|muonAcqFmt_xxxx|data|
 *  |preamble|muonAcqFmt_H|muonAcqFmt_xxxx|data|
 *  ...
 * 
 *  in the _H there are a field type which define which
 *     header data type is selected ("xxxx")
 *  tHist, tTrace, tSsdHisto or tParam
 *    
 *  
 *
 */
#include <stdint.h>

enum muonAcqFmt_e
  {
    emuonAcqFmt_trace=1,
    emuonAcqFmt_ssdHisto,
    emuonAcqFmt_param,
    emuonAcqFmt_hist,
    emuonAcqFmt_last,
    emuonAcqFmt_start=10,
    emuonAcqFmt_end,
    emuonAcqFmt_err
  };

struct muonAcqFmt_H
{
  uint32_t Tstart;
  uint16_t timeInterval;
  uint16_t type;
  uint32_t size; //include the type header and the data itself
  uint32_t nentries;//whatever event which looks valid (used to stop condition).
};

struct muonAcqFmt_tHist
{
};
struct muonAcqFmt_tTrace
{
  uint32_t nentries; //number of entries specific for the trace
};
struct muonAcqFmt_tSsdHisto
{
  uint32_t nentries; //number of entries specific for this histogram
  int16_t pkmin,pkmax;
  uint16_t nPkUnder,nPkOver;
};
struct muonAcqFmt_tParam
{
  uint32_t nentries; //number of entries specific for the parameters.
  uint16_t chbase[4];
  uint32_t Thres; 
};
#endif
