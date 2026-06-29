#ifndef RSOD_H
#define RSOD_H

#include "../lib/types.h"

void rsod_fatal(const char *message);
void rsod_fatal_code(const char *message, uint32_t code);

#endif
