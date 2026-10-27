#include <assert.h>
#include <initializer_list>
#include <string.h>
#include "../TeensyOBDSDLogger/src/mtp/VisibleObjects.h"
struct Storage {
  void GetObjectInfo(uint32_t h, char *name, uint64_t *, uint32_t *parent, uint16_t *) {
    *parent = 0;
    switch (h) {
      case 16: strcpy(name, "config.json"); break;
      case 17: strcpy(name, "OBD0000.TXT"); break;
      case 18: strcpy(name, "System Volume Information"); break;
      case 19: strcpy(name, "MTpIndex.DAT"); break;
      case 20: strcpy(name, "tracking.dat"); *parent = 18; break;
      case 21: strcpy(name, "mtpindex.dat.bak"); break;
      case 22: strcpy(name, "loop"); *parent = 22; break;
      case 23: strcpy(name, "logs"); break;
      case 24: strcpy(name, "OBD0001.TXT"); *parent = 23; break;
      default: name[0] = 0;
    }
  }
};
int main() {
  Storage s;
  assert(obd_mtp::sameName("SYSTEM VOLUME INFORMATION", "System Volume Information"));
  for (uint32_t h : {16u, 17u, 21u, 23u, 24u}) assert(obd_mtp::objectVisible<256>(s, h, 16));
  for (uint32_t h : {0u, 18u, 19u, 20u, 22u, 25u, static_cast<unsigned>(UINT32_MAX)}) assert(!obd_mtp::objectVisible<256>(s, h, 16));
}
