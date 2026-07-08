#ifndef CONFIG_H
#define CONFIG_H
#include "backend_pool.h"
#include <stdbool.h>
struct config {
    struct backend_pool p;
    bool valid;
};

struct config parse_config_file(char * file);
#endif