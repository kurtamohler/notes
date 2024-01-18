#include <stdio.h>
#include "zlog.h"

int main(int argc, char** argv) {

  int rc = zlog_init("test0.conf");

  if (rc) {
    printf("init failed\n");
    return -1;
  }

  zlog_category_t* c = zlog_get_category("my_category");

  if (!c) {
    printf("get cat fail\n");
    zlog_fini();
    return -2;
  }

  zlog_debug(c, "a debug message");
  zlog_info(c, "an info message");
  zlog_notice(c, "a notice message");
  zlog_warn(c, "a warning message");
  zlog_error(c, "an error message");
  zlog_fatal(c, "a fatal message");

  zlog_fini();
  return 0;
}
