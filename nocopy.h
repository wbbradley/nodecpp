// REVIEWED
#pragma once

#define NOCOPY(Class) \
  Class(const Class &) = delete; \
  Class &operator =(const Class &) = delete; \
  Class &operator =(const Class &&) = delete;
