#pragma once

#include "esp_http_server.h"


int run_ccode(char* ccode, httpd_req_t* req);
int get_status_info(httpd_req_t *req);