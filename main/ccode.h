#pragma once

#include "esp_http_server.h"



// if any of these == not, then they are not present.
// 'not' is adjusted to be a value that's never used
typedef struct {
    char* name;

    double x, y;
    long long a, b;
    int s;
    int r;

    long long not;
} args_t ;


int run_ccode(char* ccode, httpd_req_t* req);
int get_status_info(httpd_req_t *req);