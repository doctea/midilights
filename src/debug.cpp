#include "debug.h"

void messages_log_add(String msg) {
    if (Serial)
        Serial.println(msg);
}