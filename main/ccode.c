#include "ccode.h"
#include "esp_log.h"
#include <errno.h>
#include "main.h"
#include "math.h"

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
void m10();
void m11(int hz);
void c0(int a, int b);


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
        m10();
        httpd_resp_sendstr(req, "Homed");
    } else IF_CASE(m11) {
        PARSE_INT(1, hz);
        m11(hz);
        httpd_resp_sendstr(req, "Set feed rate");
    } else IF_CASE(c0) {
        PARSE_INT(1, a_target);
        PARSE_INT(2, b_target);
        httpd_resp_sendstr(req, "Running");
        c0(a_target, b_target);
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

// 0 is 0mm string length from the corner.
long long a_pos = 0;
long long b_pos = 0;

int max_feedrate = 1000;


void m0(int a, int b){
    spin_stepper(a, b, max_feedrate, max_feedrate);
    a_pos += a;
    b_pos += b;
}

void m1(int a_target, int b_target) {
    int a_ticks = a_target - a_pos;
    int b_ticks = b_target - b_pos;
    spin_stepper(a_ticks, b_ticks, max_feedrate, max_feedrate);
    a_pos = a_target;
    b_pos = b_target;
}

void m11(int hz) {
    max_feedrate = hz;
}


int get_status_info(httpd_req_t *req) {
    const char *format = 
        "{"
        "\"a_pos\": %lld,"
        "\"b_pos\": %lld,"
        "\"max_feedrate\": %d"
        "}";
    char buf[255] = { 0 };
    sprintf(buf, format, a_pos, b_pos, max_feedrate);
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}


// CONFIGURABLE
const double width = 822.325; // mm

const double startingHeight = 5 * 25.4; // mm, 5 inches down
const double startingWidth = width / 2;

const int ticks_per_rotation = 3200 * 2;
const double spool_dimm = 70; // mm
const double spool_circum = 70 * 2 * 3.1415; // mm

typedef struct {
    double x, y;
    long long a, b;
} setpoint_t;

long long mm_to_ticks(double mm) {
    double num_of_rotations = mm / spool_circum;
    double ticks = num_of_rotations * ticks_per_rotation;
    return (long long)(ticks);
}

setpoint_t compute_string_lengths(double x, double y) {
    double a_mm = sqrt(x*x + y*y);
    double b_mm = sqrt((width - x) * (width - x) + y*y);
    setpoint_t ret = {
        .x = x, .y = y,
        .a = mm_to_ticks(a_mm),
        .b = mm_to_ticks(b_mm),
    };
    return ret;
}



void m10() {
    setpoint_t startpoint = compute_string_lengths(startingWidth, startingHeight);
    a_pos = startpoint.a;
    b_pos = startpoint.b;
}

void c0(int x, int y) {
    setpoint_t setpoints = compute_string_lengths(x, y);
    spin_stepper(setpoints.a - a_pos, setpoints.b - b_pos, max_feedrate, max_feedrate);
    a_pos = setpoints.a;
    b_pos = setpoints.b;
}