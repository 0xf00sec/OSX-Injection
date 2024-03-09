#include <stdio.h>
#include <syslog.h>

__attribute__((constructor))
static void customConstructor(int argc, const char **argv)
 {
     printf("Foo!\n");
     syslog(LOG_ERR, "Dylib injection successful in %s\n", argv[0]);
}