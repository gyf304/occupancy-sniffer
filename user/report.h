#ifndef _REPORT_H
#define _REPORT_H

#include "osapi.h"
#include "wifi_device.h"

typedef struct report_header* report_t;

bool report_create(report_t* report_ptr, uint32_t capacity);
bool report_add_device(report_t report, wifi_device_t* device);
void report_destroy(report_t report);

#endif