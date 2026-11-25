#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
namespace obd {
using TraceWriter=void (*)(const char *);
inline void traceCan(TraceWriter write,const char *direction,bool extended,uint32_t id,uint32_t ms,const uint8_t *data,size_t n,bool remote=false){
  char text[160];int pos=snprintf(text,sizeof text,"%s-CAN:  %s: %0*lX   Time: %6lu   Len: %2u  ",direction,extended?"Ext":"Std",extended?8:3,(unsigned long)id,(unsigned long)ms,unsigned(n));
  if(remote)pos+=snprintf(text+pos,sizeof(text)-pos," RTR");
  else for(size_t i=0;i<n&&i<8;++i)pos+=snprintf(text+pos,sizeof(text)-pos," %02X",data[i]);
  snprintf(text+pos,sizeof(text)-pos,"\n");write(text);
}
// Emit bounded fragments of ONE line; no truncation of a 4095-byte ISO-TP payload.
inline void tracePayload(TraceWriter write,const char *direction,uint32_t id,uint32_t ms,const uint8_t *p,size_t n){
  char text[128];snprintf(text,sizeof text,"%7lu  %s:  %03lX   Len: %3u  ",(unsigned long)ms,direction,(unsigned long)id,unsigned(n));write(text);
  for(size_t offset=0;offset<n;offset+=32){int pos=0;
    for(size_t i=offset;i<n&&i<offset+32;++i)pos+=snprintf(text+pos,sizeof(text)-pos," %02X",p[i]);
    write(text);
  }write("\n");
}
inline void traceRequest(TraceWriter write,uint32_t ms,const uint8_t *p,size_t n){
  char text[120];int pos=snprintf(text,sizeof text,"%7lu  Mode %X",(unsigned long)ms,p[0]);
  if(n>1)pos+=snprintf(text+pos,sizeof(text)-pos,", %s %02X",p[0]==6?"MID":"PID",p[1]);
  if(p[0]==2&&n>2)pos+=snprintf(text+pos,sizeof(text)-pos,", Frame %u",p[2]);
  snprintf(text+pos,sizeof(text)-pos,"\n");write(text);
}
}
