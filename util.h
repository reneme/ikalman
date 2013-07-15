#pragma once

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <iomanip>

#include "platform/platform.h"

bool FileExists(const std::string &path) {
  platform_stat64 info;
  return ((platform_lstat(path.c_str(), &info) == 0) &&
          S_ISREG(info.st_mode));
}

size_t FileSize(const std::string &path) {
  platform_stat64 info;
  const int retval = platform_lstat(path.c_str(), &info);
  if (retval != 0) {
    return 0;
  }

  return info.st_size;
}

void Die(const std::string message) {
  std::cerr << message << std::endl;
  exit (1);
}

float ToFloat(const std::string& s) {
  std::istringstream i(s);
  float x;
  if (!(i >> x))
    return 0;
  return x;
}

struct tm ConvertTimestamp(const std::string &timestamp) {
  struct tm tm;
  constexpr auto format = "%Y-%m-%dT%T";

  std::istringstream iss (timestamp);
  iss >> std::get_time(&tm, format);
  return tm;
}
