/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <thread>

#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include "gpio_cxx.hpp"

#include "OkWifi.h"


#include <wifi_provisioning/wifi_config.h>
#include <wifi_provisioning/manager.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <wifi_provisioning/scheme_ble.h>
#include <esp_timer_cxx.hpp>


#include "WifiProv.h"
#include "ProvServerScanner.h"


// ms s us
using namespace std::chrono_literals;

static const char *TAG = "Main";



/**
 * @brief Application main
 * This Function Will Run With MainTask And When Called app_main() MainTask Ending;
 */
extern "C" _Noreturn void app_main() {
    // Initialize NVS
    std::cout << "S1" << std::endl;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());
    // esp_netif_init();

    std::cout << "S2" << std::endl;

    esp_event_loop_create_default();

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

    // ok_wifi::ProvServerScanner::getInstance().init();

    // std::this_thread::sleep_for(5s);
    ok_wifi::OkWifi::getInstance().waitExit();

    // ok_wifi::ProvServerScanner::getInstance().deinit();

    // std::this_thread::sleep_for(2s);

    // ok_wifi::WifiProv::getInstance().init();


//    auto s = idf::esp_timer::ESPTimer([]() {
//        if (!ok_wifi::OkWifi::getInstance().checkExit()) {
//            ok_wifi::OkWifi::getInstance().stop();
//        }
//    });
//    s.start(100s);

    now_status = Status::Ending;

    while (true) {
        std::cout << "Tick Time" << std::endl;
        std::this_thread::sleep_for(5s);
    }
}
