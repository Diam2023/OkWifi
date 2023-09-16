/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <thread>

#include <nvs_flash.h>
#include <esp_wifi_default.h>
#include "gpio_cxx.hpp"

#include "wifi_provisioning/manager.h"
#include "ProvServer.h"
#include "OkWifi.h"

// ms s us
using namespace std::chrono_literals;

static const char *TAG = "Main";



/**
 * @brief Application main
 * This Function Will Run With MainTask And When Called app_main() MainTask Ending;
 */
extern "C" _Noreturn void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());
        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    esp_event_loop_create_default();

    /* Initialize Wi-Fi including netif with default config */
    //    esp_netif_create_default_wifi_sta();

    auto led = idf::GPIO_Output(idf::GPIONum(13));
    auto led_status = false;
    enum class Status {
        Waiting,
        Ending
    };
    auto now_status = Status::Waiting;
    auto res = std::thread([&led, &now_status, &led_status]() -> void {
        while (true) {
            if (led_status) {
                led.set_low();
            } else {
                led.set_high();
            }
            led_status = !led_status;

            if (now_status != Status::Waiting) {
                std::this_thread::sleep_for(800ms);
            }
            std::this_thread::sleep_for(200ms);
        }
    });

    ok_wifi::OkWifi::getInstance().init();

//    if ((*ok_wifi::OkWifi::getInstance()).joinable()) {
//        ESP_LOGI(TAG, "Wait Prov Complete!");
//        (*ok_wifi::OkWifi::getInstance()).join();
//    }
    ok_wifi::OkWifi::getInstance().join();
    ESP_LOGI(TAG, "Wait Prov Complete!");

    now_status = Status::Ending;

    while (true) {
        std::cout << "Tick Time" << std::endl;
        std::this_thread::sleep_for(10s);
    }
}
