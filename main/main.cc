/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <memory>
#include <thread>

#include <nvs_flash.h>
#include <wifi_provisioning/scheme_ble.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>

#include "esp_timer_cxx.hpp"
#include "esp_event_cxx.hpp"
#include "gpio_cxx.hpp"

#include "wifi_provisioning/manager.h"
#include "BleProv.h"
#include "ProvServer.h"
#include "ProvClient.h"
#include "OkWifi.h"
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

//    auto& server = ok_wifi::ProvServer::getInstance();
//    server.setProvPwd("123");
//    server.setProvSsid("abccccc");
//    server.init();
//    server.waitCompleted();


    auto &scanner = ok_wifi::ProvServerScanner::getInstance();
    scanner.init();
    ESP_LOGI(TAG, "Start Scanner");
    std::this_thread::sleep_for(5s);

    if (scanner.checkFounded()) {
        ESP_LOGI(TAG, "First Check founded");
    } else {
        // Wait 20s
        while (true) {
            try {
                if (scanner.wait(20)) {
                    ESP_LOGI(TAG, "Founded %s", scanner.getServerSsid().c_str());
                    break;
                }
                // Founded
            } catch (idf::event::EventException &exception) {
                ESP_LOGE(TAG, "Error %s", exception.what());
            }
        }
    }
    scanner.deinit();

    auto &client = ok_wifi::ProvClient::getInstance();
    do {
        client.init();

        if (client.waitWifiConnect() == ok_wifi::ClientProvResult::ProvConnected) {
            ESP_LOGI(TAG, "Connect Successful!");
        } else {
            ESP_LOGE(TAG, "Connect Failed!");
            break;
        }

        if (client.sendRequest()) {
            ESP_LOGI(TAG, "Request Successful!");
            ESP_LOGI(TAG, "Result: ssid: [%s] pwd: [%s]", client.getProvSsid().c_str(), client.getProvPwd().c_str());
        } else {
            ESP_LOGE(TAG, "Request Failed!");
            break;
        }

    } while (false);

    client.deinit();


//    auto okWifi = std::make_unique<ok_wifi::BleProv>();
//    try {
//        okWifi->wait(50);
//    } catch (idf::event::EventException &exception) {
//        ESP_LOGE(TAG, "Error %s", exception.what());
//    }
//
//    auto &res_ = okWifi->getProvResult();
//    if (res_.getResult() == ok_wifi::ProvResultStatus::ResOk) {
//        std::cout << "Completed Prov SSID: " << res_.getSsid() << " PWD: " << res_.getPwd() << std::endl;
//    } else {
//        std::cout << "Error for Prov" << std::endl;
//    }
//
    while (true) {
        std::cout << "Tick Time" << std::endl;
        std::this_thread::sleep_for(10s);
    }
}
