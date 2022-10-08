/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

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
#include "wifi.h"
#include "rom/ets_sys.h"




typedef struct {
    gpio_num_t step;
    gpio_num_t direction;
    gpio_num_t enable;
    gpio_num_t ms1;
    gpio_num_t ms2;
} stepper_t;

stepper_t stepper_a = {
    .step      = GPIO_NUM_15,
    .direction = GPIO_NUM_2,
    .enable    = GPIO_NUM_4,
    .ms1       = GPIO_NUM_16,
    .ms2       = GPIO_NUM_17
};


// expects GPIO_NUM_X to be passed as 'pin'
static void setup_gpio_output(gpio_num_t pin) {
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
    gpio_set_level(stepper->ms2, 0);
    gpio_set_level(stepper->enable, 0);
}


extern uint8_t index_html_start[] asm("_binary_index_html_start"); 
extern uint8_t index_html_end[]   asm("_binary_index_html_end"); 

esp_err_t get_handler(httpd_req_t* req) {
    httpd_resp_send(req, (char*)index_html_start, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t spin_handler(httpd_req_t* req) {
    const char resp[] = "";
    //int header_length = httpd_req_get_hdr_value_len(req, "_wb_ticks");
    //char* ticks_str_buf = alloca(header_length + 3);
    //for(int i = 0; i < header_length + 3; i++){
    //    ticks_str_buf[i] = 0;
    //}
    //httpd_req_get_hdr_value_str(req, "_wb_ticks", ticks_str_buf, header_length);
    printf("Spinning!\n"); 
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    for(int i = 0; i < 200 * 4 ; i++){
        gpio_set_level(stepper_a.step, i % 2); // turn on LED
        //vTaskDelay(1);
        ets_delay_us(500 + 150);
    }
    return ESP_OK;
} 

httpd_uri_t uri_get_root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post_spin = {
    .uri = "/spin",
    .method = HTTP_POST,
    .handler = spin_handler,
    .user_ctx = NULL
};

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_LOGI("http", "Starting server on port: %d", config.server_port); 
    httpd_handle_t server = NULL;
    if(httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get_root);
        httpd_register_uri_handler(server, &uri_post_spin);
    }
    return server; // can be null
}






static const char *TAG = "main.c";




void app_main(void) { 
    setup_stepper(&stepper_a);


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





