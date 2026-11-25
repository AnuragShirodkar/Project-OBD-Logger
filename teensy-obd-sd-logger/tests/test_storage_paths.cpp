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
