
#include "parsing_ccode.h"

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




// returns 'ccode' parsed into an args struct. Sets *error to an error message if it's fucked somehow
args_t parse_args(char *ccode, const char **error) {

    int not = -4238; // just some random value, can be anything.
    args_t args = {
        .x = not, .y = not,
        .a = not, .b = not,
        .s = not, .not = not,
        .r = not,
        .name = NULL,
    };


    const char delim[2] = " ";
    // take the first character
    args.name = strtok(ccode, delim);
    // iterate through the rest of the characters
    char *token = strtok(NULL, delim);
    while(token != NULL) {
        if(token[0] == 0) {
            *error = "For some reason, got an empty token. 500.";
            return args;
        }
        char c = token[0];
        int i = atoi(token + 1);
        //ESP_LOGI(TAG, "token: %s, i: %i", token, i);
        if(i == not) {
            // increment 'not' until we find a value that isn't used by anyone, including the new value
            do {
                not++;
            } while(args.x != not && args.y != not && 
                    args.a != not && args.b != not && 
                    args.s != not && i != not);
            // copy the new value into the old unused values
            if(args.x == args.not) args.x = not;
            if(args.y == args.not) args.y = not;
            if(args.a == args.not) args.a = not;
            if(args.b == args.not) args.b = not;
            if(args.s == args.not) args.s = not;
            if(args.r == args.not) args.r = not;
            args.not = not;
        }
        switch(c) {
            case 'x': args.x = i; break;
            case 'y': args.y = i; break;
            case 'a': args.a = i; break;
            case 'b': args.b = i; break;
            case 's': args.s = i; break;
            case 'r': args.r = i; break;
            default:
                *error = "Not an acceptable parameter prefix character";
                return args;
        }

        // take the next token, and loop
        token = strtok(NULL, delim);
    }
    return args;
}
