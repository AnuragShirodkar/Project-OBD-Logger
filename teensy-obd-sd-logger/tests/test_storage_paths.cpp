#include <assert.h>
#include <string.h>
#include "../TeensyOBDSDLogger/StoragePaths.h"

struct Handle {
  bool exists, directory;
  explicit operator bool() const { return exists; }
  bool isDirectory() const { return directory; }
  void close() {}
};
struct Storage {
  bool canonical=false, present=false, directory=false, writable=true;
  unsigned creates=0;
  bool exists(const char *path) const {
    return !strcmp(path,automatic::CONFIG_PATH)?canonical:present;
  }
  bool mkdir(const char *) {
    ++creates;
    if(!writable)return false;
    present=true;directory=true;return true;
  }
  Handle open(const char *) const {return {present,directory};}
};
int main() {
  char path[48];
  assert(automatic::formatLogPath(path,sizeof path,1,"OBD_1757338",false));
  assert(!strcmp(path,"/LOGS/01_OBD_1757338.TXT"));
  assert(automatic::formatLogPath(path,sizeof path,1,"OBD_1757338",true));
  assert(!strcmp(path,"/LOGS/01_CAN_1757338.ASC"));
  assert(automatic::formatLogPath(path,sizeof path,2,"OBD_1757338",true));
  assert(!strcmp(path,"/LOGS/02_CAN_1757338.ASC"));
  assert(automatic::formatLogPath(path,sizeof path,999999,"OBD_16777215",true));
  assert(!strcmp(path,"/LOGS/999999_CAN_16777215.ASC"));
  assert(automatic::formatLogPath(path,sizeof path,1,"OBD_0",true));
  assert(!strcmp(path,"/LOGS/01_CAN_0.ASC"));
  assert(!automatic::formatLogPath(path,8,1,"OBD_1757338",true));
  assert(!automatic::formatLogPath(path,sizeof path,0,"OBD_1757338",true));
  assert(!automatic::formatLogPath(path,sizeof path,1000000,"OBD_1757338",true));
  assert(!automatic::formatLogPath(path,sizeof path,1,"OBD_",true));
  assert(!automatic::formatLogPath(path,sizeof path,1,"OBD_../x",true));
  Storage storage;
  assert(!strcmp(automatic::configPath(storage),automatic::LEGACY_CONFIG_PATH));
  storage.canonical=true;
  assert(!strcmp(automatic::configPath(storage),automatic::CONFIG_PATH));
  assert(automatic::ensureDirectory(storage,automatic::LOG_DIRECTORY));
  assert(storage.creates==1);
  assert(automatic::ensureDirectory(storage,automatic::LOG_DIRECTORY));
  assert(storage.creates==1);
  storage.directory=false;
  assert(!automatic::ensureDirectory(storage,automatic::LOG_DIRECTORY));
  assert(storage.creates==1); // Never replace an existing file.
  storage.present=false;storage.writable=false;
  assert(!automatic::ensureDirectory(storage,automatic::LOG_DIRECTORY));
}
