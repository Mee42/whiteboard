#include "ccode.h"
#include "esp_log.h"
#include <errno.h>
#include "main.h"

char* next_token(char **ref) {
    char *str = *ref;
    if(*str == 0) return NULL; // if str is already pointing at the zero byte, we've reached the end
    // else
    while(*str == ' ') str++; // seek to the next nonwhitespace character
    char *buf = malloc(25);
    size_t size = 0;
    while(*str != ' ' && *str != 0) {
        buf[size++] = *str++;
    }
    buf[size] = 0; // null terminating byte
    *ref = str;
    return buf;
}




void m0(int a, int b);
void m1(int a, int b);
void m10(char c);
void m11(int hz);




int run_ccode(char *ccode, httpd_req_t *req) {

    const char *error = "Generic Error";


    if(ccode[0] == 0) {
        error = "No content";
        goto run_ccode_err;
    }
    if(ccode[0] != 'c' && ccode[0] != 'm') {
        error = "Must start with c/m";
        goto run_ccode_err;
    }

    char* toks[7] = { next_token(&ccode), 
                       next_token(&ccode), 
                       next_token(&ccode), 
                       next_token(&ccode), 
                       next_token(&ccode), 
                       next_token(&ccode), 
                       next_token(&ccode) };
    
    #define IF_CASE(NAME) if(strcmp(toks[0], #NAME) == 0) 

    #define PARSE_INT(TOK_NUM, VARIABLE) if(toks[TOK_NUM] == NULL) {\
            error = "Expecting token at token " #TOK_NUM;\
            goto run_ccode_err;\
        }\
        for(int i = 0; i < strlen(toks[TOK_NUM]); i++){\
            char c = toks[TOK_NUM][i];\
            if((c < '0' || c > '9') && c != '&' && c != '-'){\
                error = "Expecting number at token " #TOK_NUM;\
                goto run_ccode_err;\
            }\
        }\
        errno = 0;\
        int VARIABLE = strtol(toks[TOK_NUM], NULL, 0);\
        if(errno != 0) {\
            error = "Failed to parse int at token " #TOK_NUM;\
            goto run_ccode_err;\
        }



    IF_CASE(m0) {
        PARSE_INT(1, a);
        PARSE_INT(2, b);
        httpd_resp_sendstr(req, "Running");
        m0(a, b);
    } else IF_CASE(m1) {
        PARSE_INT(1, a_target);
        PARSE_INT(2, b_target);
        httpd_resp_sendstr(req, "Running");
        m1(a_target, b_target);
    } else IF_CASE(m10) {
        if(toks[1] == NULL) {
            error = "Expecting token at token 1";
            goto run_ccode_err;
        }
        char c = toks[1][0];
        if(c != 'a' && c != 'A' && c != 'b' && c != 'B') {
            error = "Token 1 must be either 'A' or 'B'";
            goto run_ccode_err;
        }
        m10(c);
        httpd_resp_sendstr(req, c == 'a' || c == 'A' ? "Homed A" : "Homed B");
    } else IF_CASE(m11) {
        PARSE_INT(1, hz);
        m11(hz);
        httpd_resp_sendstr(req, "Set feed rate");
    } else IF_CASE(c0) {
        goto run_ccode_unimpl;
    } else IF_CASE(c1) {
        goto run_ccode_unimpl;
    } else IF_CASE(c10) {
        goto run_ccode_unimpl;
    } else IF_CASE(c11) {
        goto run_ccode_unimpl;
    } else {
        error = "Not a valid ccode command";
        goto run_ccode_err;
    }
    return ESP_OK; // we hope we're sent back smth by now...



    run_ccode_err:
   	httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_sendstr(req, error);
    return ESP_OK;

    run_ccode_unimpl:
   	httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_sendstr(req, "Unimplemented :(");
    return ESP_OK;
    
}

long long a_home = 0;
long long b_home = 0;

 // 0 means nothing for a_pos, when a_pos = a_home it is at the top corner
long long a_pos = 0;
long long b_pos = 0;

int max_feedrate = 1000;

// THESE ARE ALL UNTESTED
void m0(int a, int b){
    spin_stepper(a, b, max_feedrate, max_feedrate);
    a_pos += a;
    b_pos += b;
}

void m1(int a_target, int b_target) {
    int a_ticks = (a_home + a_target) - a_pos;
    int b_ticks = (b_home + b_target) - b_pos;
    spin_stepper(a_ticks, b_ticks, max_feedrate, max_feedrate);
    a_pos = a_home + a_target; // a_pos = a_pos + (a_target - a_pos)
    b_pos = b_home + b_target;
}
void m10(char mode) {
    if(mode == 'a' || mode == 'A') {
        a_home = a_pos;
    } else {
        b_home = b_pos;
    }
}
void m11(int hz) {
    max_feedrate = hz;
}