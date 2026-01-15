#ifndef __MOC32JSON_DEBUG_H__
#define __MOC32JSON_DEBUG_H__

#include <cstdio>

static void log_dbg(const char *msg) {
#ifdef NDEBUG
  (void)msg;
#else
  fprintf(stderr, "DEBUG: %s\n", msg);
#endif
}

#endif  // __MOC32JSON_DEBUG_H__
