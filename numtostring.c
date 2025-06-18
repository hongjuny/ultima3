#include "numtostring.h"
#include <stdio.h>
#include <string.h>

void num_to_string(long number, unsigned char *outStr) {
    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%ld", number);
    if (len < 0) len = 0;
    if (len > 255) len = 255;
    outStr[0] = (unsigned char)len;
    memcpy(outStr + 1, buffer, len);
}
