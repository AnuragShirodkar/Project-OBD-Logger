#include "../TeensyOBDSDLogger/LogRotation.h"
#include <assert.h>
#include <stdio.h>
int main(){
  using namespace automatic;
  assert(!needsLogRotation(0,8192));
  assert(!needsLogRotation(LogFileLimit-8192,8192));
  assert(needsLogRotation(LogFileLimit-8191,8192));
  assert(needsLogRotation(LogFileLimit,1));
  assert(needsLogRotation(LogFileLimit+1,1));
  uint64_t size=60;unsigned files=1;
  for(unsigned i=0;i<100000;++i){
    unsigned batch=4096+(i%320);
    if(needsLogRotation(size,batch)){assert(size<=LogFileLimit);size=60;++files;}
    size+=batch;assert(size<=LogFileLimit);
  }
  assert(files==2);puts("Log rotation size-boundary tests passed");
}
