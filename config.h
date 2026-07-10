#ifndef CONFIG_H
#define CONFIG_H
#include "backend_pool.h"
#include <stdbool.h>
#include <strategy.h>


struct config {
    struct backend_pool p;
    enum strategy s;
};

struct config parse_config_file(char * file);
#endif