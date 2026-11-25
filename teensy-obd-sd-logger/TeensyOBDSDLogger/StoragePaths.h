#pragma once

namespace automatic {
constexpr const char *CONFIG_DIRECTORY = "/CONFIG";
constexpr const char *LOG_DIRECTORY = "/LOGS";
constexpr const char *CONFIG_PATH = "/CONFIG/config.json";
constexpr const char *LEGACY_CONFIG_PATH = "/config.json";

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
