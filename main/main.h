#pragma once

typedef enum {
	DIRECTION_EXTRUDE, 
	DIRECTION_INTAKE
} direction_t;

void spin_stepper(int a_ticks, int b_ticks, int a_hz, int b_hz, long long* a_real, long long* b_real);




