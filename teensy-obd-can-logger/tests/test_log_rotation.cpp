#include "../TeensyOBDSDLogger/LogRotation.h"
#include "../TeensyOBDSDLogger/CanRecording.h"
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
  assert(files==2);
  assert(!needsLogRotation(LogFileLimit-CAN_ASC_END_SIZE-8192,8192+CAN_ASC_END_SIZE));
  assert(needsLogRotation(LogFileLimit-CAN_ASC_END_SIZE-8191,8192+CAN_ASC_END_SIZE));
  size=768;files=1;
  for(unsigned i=0;i<100000;++i){
    unsigned batch=4096+(i%320);
    if(needsLogRotation(size,batch+CAN_ASC_END_SIZE)){
      assert(size+CAN_ASC_END_SIZE<=LogFileLimit);size=768;++files;
    }
    size+=batch;assert(size+CAN_ASC_END_SIZE<=LogFileLimit);
  }
  assert(files==2);size+=CAN_ASC_END_SIZE;assert(size<=LogFileLimit);
  puts("Log rotation size-boundary and CAN footer-reservation tests passed");
}
