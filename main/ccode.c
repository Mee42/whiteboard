#include "ccode.h"
#include "esp_log.h"
#include <errno.h>
#include "main.h"
#include "math.h"
#include "parsing_ccode.h"

#define TAG "ccode"

// CONFIGURABLE
const double width = 822.325; // mm
const double startingHeight = 5 * 25.4; // mm, 5 inches down

// constants
const double startingWidth = width / 2;
const int ticks_per_rotation = 3200 * 2; // * 2 for two "steps" per real step, I think. It works irl so.
const double spool_dimm = 70; // mm
const double spool_circum = 70 * 2 * 3.1415; // mm

// helpers
// we need this one to use commas inside of macro values (cursed, yes, but uh)
#define COMMA ,
#define REPEAT_9X(A)  A A A A A A A A A
#define REPEAT_10X(A) A A A A A A A A A A


typedef struct {
    double x, y;
    long long a, b;
    bool is_xy_valid;
} specific_position_t;

typedef struct { double x, y; } xy_pos_t;



void m0 (args_t);
void m1 (args_t);
void m10(args_t);
void m11(args_t);
void c0 (args_t);
void c10(args_t);
void c11(args_t);



// are we processing a command on a separate thread
bool running = false;

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

    error = NULL;
    args_t args = parse_args(ccode, &error);
    if(error != NULL) {
        goto run_ccode_err;
    }
    // args is all good!
    
    #define IF_CASE(NAME) if(strcmp(args.name, #NAME) == 0) 
    
    // satisfy dependencies before entering the function
    #define WANT( VAL ) if(args.VAL == args.not) { error = "Can't find parameter: " #VAL; goto run_ccode_err; }
    #define WANT2( A, B ) WANT(A) WANT(B)
    #define WANT3( A, B, C ) WANT2(A, B) WANT(C)
    #define WANT4( A, B, C, D ) WANT3(A, B, C) WANT(D)
    #define WANT5( A, B, C, D, E ) WANT4(A, B, C, D) WANT(E)

    IF_CASE(m0) {
        WANT2(a, b)
        httpd_resp_sendstr(req, "Running");
        m0(args);
    } else IF_CASE(m1) {
        WANT2(a, b)
        httpd_resp_sendstr(req, "Running");
        m1(args);
    } else IF_CASE(m10) {
        m10(args);
        httpd_resp_sendstr(req, "Homed");
    } else IF_CASE(m11) {
        WANT(s)
        m11(args);
        httpd_resp_sendstr(req, "Set feed rate");
    } else IF_CASE(c0) {
        WANT2(x, y)
        httpd_resp_sendstr(req, "Running");
        c0(args);
    } else IF_CASE(c1) {
        goto run_ccode_unimpl;
    } else IF_CASE(c10) {
        WANT(r)
        c10(args);
        httpd_resp_sendstr(req, "Set default register");
    } else IF_CASE(c11) {
        WANT(r)
        c11(args);
        httpd_resp_sendstr(req, "Set default register");
    } else {
        error = "Not a valid ccode command";
        goto run_ccode_err;
    }
    return ESP_OK;
    
    run_ccode_err:
   	httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_sendstr(req, error);
    return ESP_OK;

    run_ccode_unimpl:
   	httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_sendstr(req, "Unimplemented :(");
    return ESP_OK;
    
}

// ===== current state =====
specific_position_t current_position = {
    .a = 0,
    .b = 0,
    .x = 0,
    .y = 0,
    .is_xy_valid = false,
};

// these are updated in real-time, and are ONLY for debugging purposes (and to look cool)
long long real_a = 0, real_b = 0;


int max_feedrate = 1000;

int default_reg = 0;

// regs[0] needs to stay { 0, 0 }
// regs[9] is fake, should NEVER be read, users should instead look at current_position
xy_pos_t regs[10] = { REPEAT_10X({ .x = 0 COMMA .y = 0 } COMMA) }; 


// =========================

void task_fn() {
    int a_ticks = -1000;
    int b_ticks = -1000;
    running = true;
    ESP_LOGI("task_fn", "starting spin");
    spin_stepper(a_ticks, b_ticks, max_feedrate, max_feedrate, &real_a, &real_b);
    ESP_LOGI("task_fn", "done with spin");
    running = false;
}

void spin_step_ez(int a_ticks, int b_ticks) {
    task_fn();
    if(false) return;
      xTaskCreate(
                    task_fn,          /* Task function. */
                    "spintask",        /* String with name of task. */
                    1000,            /* Stack size in bytes. */
                    NULL,             /* Parameter passed as input of the task */
                    1,                /* Priority of the task. */
                    NULL);            /* Task handle. */

}

void m0(args_t args){
    spin_step_ez(args.a, args.b);
    current_position.a += args.a;
    current_position.b += args.b;
    current_position.is_xy_valid = false;
}

void m1(args_t args) {
    int a_ticks = args.a - current_position.a;
    int b_ticks = args.b - current_position.b;
    spin_step_ez(a_ticks, b_ticks);
    current_position.a = args.a;
    current_position.b = args.b;
    current_position.is_xy_valid = false;
}

void m11(args_t args) {
    max_feedrate = args.s;
}


xy_pos_t get_reg_value(int reg) {
    xy_pos_t offset = regs[reg];
    if(reg == 9) {
        offset.x = current_position.x;
        offset.y = current_position.y;
    }
    return offset;
}

int get_status_info(httpd_req_t *req) {
    const char *format = 
        "{"
            "\"a_pos\": %lld,"
            "\"b_pos\": %lld,"
            "\"a_real\": %lld," 
            "\"b_real\": %lld," 
            "\"x_pos\": %f,"
            "\"y_pos\": %f,"
            "\"is_xy_valid\": %s,"
            "\"max_feedrate\": %d,"
            "\"default_reg\": %d,"
            "\"regs\": ["
                "[ %d, %d ]"
                REPEAT_9X(", [ %d, %d ]")
            "],"
            "\"running\": %s"
        "}";
        
    char buf[255] = { 0 };
    
    #define REG(N) ((int)get_reg_value(N).x), ((int)get_reg_value(N).y)

    sprintf(buf, format, 
        current_position.a, current_position.b,
        real_a, real_b,
        current_position.x, current_position.y,
        current_position.is_xy_valid ? "true" : "false",
        max_feedrate, default_reg,
        REG(0), 
        REG(1), REG(2), REG(3),
        REG(4), REG(5), REG(6),
        REG(7), REG(8), REG(9),
        running ? "true" : "false");
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

long long mm_to_ticks(double mm) {
    double num_of_rotations = mm / spool_circum;
    double ticks = num_of_rotations * ticks_per_rotation;
    return (long long)(ticks);
}

specific_position_t compute_string_lengths(double x, double y) {
    double a_mm = sqrt(x*x + y*y);
    double b_mm = sqrt((width - x) * (width - x) + y*y);
    specific_position_t ret = {
        .x = x, .y = y,
        .a = mm_to_ticks(a_mm),
        .b = mm_to_ticks(b_mm),
        .is_xy_valid = true
    };
    return ret;
}

void m10(args_t args) {
    current_position = compute_string_lengths(startingWidth, startingHeight);
}

specific_position_t get_specified_position(args_t args) {
    // if rX is included, respects that, otherwise uses the default register
    int reg = (args.r != args.not) ? args.r : default_reg;
    xy_pos_t offset = get_reg_value(reg);
    return compute_string_lengths(args.x + offset.x, args.y + offset.y);
}


void c0(args_t args) {
    specific_position_t setpoints = get_specified_position(args);
    spin_step_ez(setpoints.a - current_position.a, setpoints.b - current_position.b);
    current_position = setpoints;
}

void c10(args_t args) {
    int reg = args.r;
    double x = (args.x != args.not) ? args.x : current_position.x;
    double y = (args.y != args.not) ? args.y : current_position.y;
    if(reg <= 0 || reg >= 9) return;
    regs[reg].x = x;
    regs[reg].y = y;
}

void c11(args_t args) {
    default_reg = args.r;
}