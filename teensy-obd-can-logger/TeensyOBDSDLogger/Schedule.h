#pragma once
#include <stdint.h>
#include <stddef.h>
namespace automatic {
struct Job {
  bool enabled=true;
  uint8_t bytes[3]={}; size_t length=0;
  uint32_t interval=10, next=0, sent=0, skipped=0, late=0, maxLate=0;
};
class Schedule {
  size_t cursor=0;
public:
  void begin(Job *jobs,size_t count,uint32_t now) {
    cursor=0;for(size_t i=0;i<count;++i)jobs[i].next=now;
  }
  // One outstanding opportunity per job; missed periods never become a backlog.
  int select(Job *jobs,size_t count,uint32_t now) {
    for(size_t n=0;n<count;++n){
      size_t i=(cursor+n)%count; Job &j=jobs[i];
      if(!j.enabled||int32_t(now-j.next)<0)continue;
      uint32_t lateness=now-j.next, skipped=lateness/j.interval;
      j.skipped+=skipped; if(lateness)++j.late;
      if(lateness>j.maxLate)j.maxLate=lateness;
      j.next+=(skipped+1)*j.interval;cursor=(i+1)%count;return int(i);
    }
    return -1;
  }
  void finish(Job *jobs,size_t count,uint32_t now) {
    for(size_t i=0;i<count;++i){Job &j=jobs[i];
      if(j.enabled&&int32_t(now-j.next)>=0){uint32_t n=(now-j.next)/j.interval+1;j.skipped+=n;j.next+=n*j.interval;}
    }
  }
};
}
