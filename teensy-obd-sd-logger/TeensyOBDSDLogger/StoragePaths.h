#pragma once
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace automatic {
constexpr const char *CONFIG_DIRECTORY = "/CONFIG";
constexpr const char *LOG_DIRECTORY = "/LOGS";
constexpr const char *CONFIG_PATH = "/CONFIG/config.json";
constexpr const char *LEGACY_CONFIG_PATH = "/config.json";

inline bool formatLogPath(char *out,size_t capacity,unsigned index,const char *logger,bool passiveCan){
  if(!logger||strncmp(logger,"OBD_",4)||!logger[4]||index<1||index>=1000000)return false;
  const char *id=logger+4;
  for(const char *p=id;*p;++p)if(*p<'0'||*p>'9')return false;
  const int length=snprintf(out,capacity,"/LOGS/%02u_%s_%s.%s",index,
    passiveCan?"CAN":"OBD",id,passiveCan?"ASC":"TXT");
  return length>=0&&size_t(length)<capacity;
}

// An existing canonical config wins even if invalid: do not silently use stale settings.
template<class Storage>
const char *configPath(Storage &storage) {
  return storage.exists(CONFIG_PATH) ? CONFIG_PATH : LEGACY_CONFIG_PATH;
}

// A regular file named CONFIG or LOGS is an error, never something to overwrite.
template<class Storage>
bool ensureDirectory(Storage &storage, const char *path) {
  if (!storage.exists(path) && !storage.mkdir(path)) return false;
  auto directory = storage.open(path);
  const bool valid = directory && directory.isDirectory();
  directory.close();
  return valid;
}
}
