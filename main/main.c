#include <stdint.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wpa2.h"
#include "rom/ets_sys.h"
#include "esp_timer.h"

#include <inttypes.h>

#include "http.h"
#include "wifi.h"
#include "main.h"

typedef struct {
    gpio_num_t step;
    gpio_num_t direction;
    gpio_num_t enable;
    gpio_num_t ms1;
    gpio_num_t ms2;
	int32_t   position;
} stepper_t;

stepper_t stepper_a = {
    .step      = GPIO_NUM_15,
    .direction = GPIO_NUM_2,
    .enable    = GPIO_NUM_4,
    .ms1       = GPIO_NUM_16,
    .ms2       = GPIO_NUM_17,
	.position  = 0
};
stepper_t stepper_b = {
	.step      = GPIO_NUM_23,
	.direction = GPIO_NUM_22,
	.enable    = GPIO_NUM_21,
	.ms1       = GPIO_NUM_19,
	.ms2       = GPIO_NUM_18, 
	.position  = 0
};


static void setup_gpio_output(gpio_num_t pin) {
	// this is the new code, has not been tested yet
	//gpio_reset_pin(pin);
	//gpio_set_direction(pin, GPIO_MODE_OUTPUT);
	//return; // in theory the above is the same as the below. TODO verify
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1U << pin;
    io_conf.pull_down_en = 0; // down-pull mode
    io_conf.pull_up_en = 0; // pull-up mode
    esp_err_t err = gpio_config(&io_conf);
    if(err != ESP_OK) {
        printf("Error while stetting pin %d as output pin\n", pin);
    }
}

static void setup_stepper(stepper_t* stepper) {
    setup_gpio_output(stepper->step);
    setup_gpio_output(stepper->direction);
    setup_gpio_output(stepper->enable);
    setup_gpio_output(stepper->ms1);
    setup_gpio_output(stepper->ms2);
    gpio_set_level(stepper->direction, 0);
    gpio_set_level(stepper->ms1, 1);
    gpio_set_level(stepper->ms2, 1); // turn to 0 for big step
    gpio_set_level(stepper->enable, 0);
}

// speed is in ticks per second
void spin_stepper(int a_ticks, int b_ticks, int a_hz, int b_hz, long long* a_real, long long* b_real) {

	a_ticks = -1 * a_ticks; // reverse the direction of A

	gpio_set_level(stepper_a.direction, a_ticks < 0 ? 1 : 0);
	gpio_set_level(stepper_b.direction, b_ticks < 0 ? 1 : 0);
	a_ticks = a_ticks < 0 ? (0 - a_ticks) : a_ticks;
	b_ticks = b_ticks < 0 ? (0 - b_ticks) : b_ticks;

	// we want to run two motors at variable, different ferquency
	int period_a = 1000 * 1000  / a_hz / 2;
	int period_b = 1000 * 1000  / b_hz / 2;
	int64_t next_for_a = esp_timer_get_time() + period_a;
	bool a_state = true;
	int64_t next_for_b = esp_timer_get_time() + period_b;
	bool b_state = true;
	while(a_ticks > 0 || b_ticks > 0) {
		// just a for now
		int64_t current_time = esp_timer_get_time();

		// we're going to see if 'a' or 'b' is closer to toggling	
		// and then do that
		if(next_for_a == next_for_b) {
			next_for_a++; // if they overlap (likely if we keep our timings tight)
			continue;     // then just do 'b', and do 'a' next microsecond
						  // we don't need to be precice on timings here
		} else if(next_for_a < next_for_b) {
			int64_t delta = next_for_a - current_time;
			if(delta >= 0) {
				ets_delay_us(next_for_a - current_time);
			}
			// TODO do a more real time way of getting next_for_a 
			// so we don't drift over time??
			// might be better for lines and shit
			next_for_a = esp_timer_get_time() + period_a;

			if(a_ticks == 0) continue;
			// we do this after all the other stuff
			// so that it doesn't impact timings when a finishes
			// I think that makes sense

			a_state = !a_state;
			gpio_set_level(stepper_a.step, a_state); 
			if(a_state == false) {
				// when the falling edge of the pulse, basically
				// that's what we count as the end of the step
				a_ticks--;
				(*a_real)--;
			}
		} else {
			// b is closer to toggling
			int64_t delta = next_for_b - current_time;
			if(delta >= 0) {
				ets_delay_us(next_for_b - current_time);
			}
			next_for_b = esp_timer_get_time() + period_b;
		
			if(b_ticks == 0) continue;

			b_state = !b_state;
			gpio_set_level(stepper_b.step, b_state);
			if(b_state == false) {
				b_ticks--;
				(*b_real)--;
			}
		}
	}
	printf("done spinning\n");
}








void app_main(void) { 
    setup_stepper(&stepper_a);
	setup_stepper(&stepper_b);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    
    initalize_wifi();
    start_webserver();
}





