#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int x, y;
} point_t;


int main(void) {
    #define points_size 46
    // point_t points[points_size] = {
	// { -100, -70 }, // w
	// { -85, 0 },
	// { -60, -70 },
	// { -45, 0 },
	// { -30, -70 },

	// { -20, -70 }, // p
	// { 0, -50 },
	// { -20, -30 },
	// { -20, -70 },
	// { -20, 0 },

	// { 10, 0 }, // i
	// { 35, 0 },
	// { 60, 0 },
	// { 35, 0 },
	// { 35, -70 },
	// { 10, -70 },
	// { 35, -70 },
	// { 60, -70 }
    // };
    point_t points[points_size] = {
{ -49, 0},
{ -49, 0},
{ -35, -48},
{ -35, -48},
{ -31, -25},
{ -31, -25},
{ 36, -25},
{ 36, -25},
{ 41, -48},
{ 41, -48},
{ 48, 1},
{ 48, 1},
{ 27, 0},
{ 27, 0},
{ 28, -16},
{ 28, -16},
{ 25, -15},
{ 25, -15},
{ 25, -11},
{ 25, -11},
{ 31, -12},
{ 31, -12},
{ 31, -16},
{ 31, -16},
{ 28, -16},
{ 28, -16},
{ 28, 1},
{ 28, 1},
{ -23, 0},
{ -23, 0},
{ -22, -18},
{ -22, -18},
{ -20, -17},
{ -20, -17},
{ -19, -13},
{ -19, -13},
{ -24, -13},
{ -24, -13},
{ -24, -17},
{ -24, -17},
{ -22, -17},
{ -22, -17},
{ -23, 0},
{ -23, 0},
{ 0, 0},
{ 0, 0},
    };


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
        fprintf(stderr, "r %i, %i\n", buf_iter->x, buf_iter->y);
        printf("Sending (%i, %i)\n", buf_iter->x, buf_iter->y);
        if(((unsigned long)buf_iter) % 3 == 0 || buf_iter < buffer + 10) {
            sleep(1);
        }
    }
    printf("size: %i\n", buffer_head - buffer);
    free(buffer);
    buffer = buffer_head = NULL;
}
