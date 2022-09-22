#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define true 1
#define false 0

typedef struct {
    int x, y;
} point_t;

int main(void) {

    #define points_size 444



    // linearly interpolate between them
    const double point_count = 100; // number of points between each defined point
    point_t* buffer = malloc(sizeof(point_t) * point_count * points_size); // most likely bigger buffer than we need but whatever
    point_t* buffer_head = buffer; // this is the spot to write to, it always points to the next open slot

    for(int point_index = 0;  point_index < points_size - 1; point_index++) {
        point_t one = points[point_index];
        point_t two = points[point_index + 1];
        // 100 intersperced points
        for(int progression = 0; progression < point_count; progression++) {
            double x = (int) (one.x + (two.x - one.x) * (progression/point_count));
            double y = (int) (one.y + (two.y - one.y) * (progression/point_count));
            if(buffer_head != buffer && buffer_head[-1].x == x && buffer_head[-1].y == y) {
                // skip
            } else {
                // add it
                buffer_head->x = x; // HERE
                buffer_head->y = y;
                buffer_head++;
            }
        }
    }

    // lets write to the serial port directly!
    FILE* serialPort = fopen("/dev/ttyACM0", "a+");

    for(point_t* buf_iter = buffer; buf_iter < buffer_head; buf_iter++) {
        fprintf(serialPort, "r %i, %i\n", buf_iter->x, buf_iter->y);
        printf("Sending (%i, %i)\n", buf_iter->x, buf_iter->y);
        if(buf_iter < buffer + 10) {
            sleep(1);
        }
        usleep(1000 * 100);
    }
    printf("size: %i\n", buffer_head - buffer);
    free(buffer);
    buffer = buffer_head = 0;
}