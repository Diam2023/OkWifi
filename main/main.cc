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


//    OkWifiStartMode mode = OkWifiStartMode::ModeUnInitialize;
//
//    auto &scanner = ok_wifi::ProvServerScanner::getInstance();
//    scanner.init();
//    ESP_LOGI(TAG, "Wait first scan");
//
//    std::this_thread::sleep_for(5s);
//    ok_wifi::BleProv &prov = ok_wifi::BleProv::getInstance();
//    auto &client = ok_wifi::ProvClient::getInstance();
//    auto &server = ok_wifi::ProvServer::getInstance();
//
//
//    if (!scanner.checkFounded()) {
//        mode = OkWifiStartMode::ModeWaitBleProvAndServer;
//
//        // not found
//        // deinit scanner
//
//        ESP_LOGI(TAG, "Deinit Scan");
//        scanner.deinit();
//
//        std::this_thread::sleep_for(1s);
//        // init prov
//        ESP_LOGI(TAG, "Init ble prov");
//        prov.init();
//
//        while (true) {
//            auto &res_ = prov.getProvResult();
//            if (prov.wait(1)) {
//                if (res_.getResult() == ok_wifi::ProvResultStatus::ResOk) {
//                    std::cout << "Completed Prov SSID: " << res_.getSsid() << " PWD: " << res_.getPwd() << std::endl;
//                    mode = OkWifiStartMode::ModeServer;
//                    prov.stop();
//                    break;
//                } else {
//                    std::cout << "Error for Prov" << std::endl;
//                    std::this_thread::sleep_for(5s);
//                    continue;
//                }
//            }
//
//            wifi_prov_sta_fail_reason_t reason;
//            // TODO Wait more issue resume
//            if (wifi_prov_mgr_get_wifi_disconnect_reason(&reason) == ESP_OK) {
//                // Restart
//                prov.stop();
//                prov.init();
//            }
//
//            std::this_thread::sleep_for(5s);
//
//            try {
//                if (scanner.scanOnce(1s)) {
//
//                    ESP_LOGI(TAG, "Stopping Ble Prov!");
//                    prov.stop();
//
//                    ESP_LOGI(TAG, "Stopped Ble Prov!");
//
//                    mode = OkWifiStartMode::ModeClient;
//
//                    ESP_LOGI(TAG, "Client Mode!");
//                    client.setServerSsid(scanner.getServerSsid());
//                    client.init();
//                    if (client.waitWifiConnect() == ok_wifi::ClientProvResult::ProvConnected) {
//                        ESP_LOGI(TAG, "Connect Successful!");
//                    } else {
//                        ESP_LOGE(TAG, "Connect Failed!");
//                        continue;
//                    }
//
//                    if (client.sendRequest()) {
//                        ESP_LOGI(TAG, "Request Successful!");
//                        ESP_LOGI(TAG, "Result: ssid: [%s] pwd: [%s]", client.getProvSsid().c_str(),
//                                 client.getProvPwd().c_str());
//                        mode = OkWifiStartMode::ModeCompleted;
//                        break;
//                    } else {
//                        ESP_LOGE(TAG, "Request Failed!");
//                        continue;
//                    }
//                }
//            } catch (idf::event::EventException &exception) {
//                if (exception.error == ESP_ERR_WIFI_STATE) {
//                    ESP_LOGW(TAG, "Stop Scan for 20s!");
//                    esp_wifi_scan_stop();
//
//                    esp_wifi_disconnect();
//                    esp_wifi_connect();
//
//                    // Stop Scan When Ble Received Prov Message
//                    std::this_thread::sleep_for(20s);
//                }
//            }
//            std::this_thread::sleep_for(5s);
//        }
//    } else {
//        scanner.deinit();
//        esp_event_loop_delete_default();
//        esp_event_loop_create_default();
//
//        do {
//            // Found prover
//            client.setServerSsid(scanner.getServerSsid());
//
//            client.init();
//            if (client.waitWifiConnect() == ok_wifi::ClientProvResult::ProvConnected) {
//                ESP_LOGI(TAG, "Connect Successful!");
//            } else {
//                ESP_LOGE(TAG, "Connect Failed!");
//                continue;
//            }
//
//            if (client.sendRequest()) {
//                ESP_LOGI(TAG, "Request Successful!");
//                ESP_LOGI(TAG, "Result: ssid: [%s] pwd: [%s]", client.getProvSsid().c_str(),
//                         client.getProvPwd().c_str());
//                mode = OkWifiStartMode::ModeCompleted;
//                break;
//            } else {
//                ESP_LOGE(TAG, "Request Failed!");
//                continue;
//            }
//        } while (true);
//    }
//
//    if (mode == OkWifiStartMode::ModeServer) {
//        std::this_thread::sleep_for(5s);
//
//        // Start Prov Server
//        auto &res_ = prov.getProvResult();
//        std::cout << "Prepare Sending Prov Data SSID: " << res_.getSsid() << " PWD: " << res_.getPwd() << std::endl;
//        mode = OkWifiStartMode::ModeWaitServerCompleted;
//        esp_event_loop_delete_default();
//        esp_event_loop_create_default();
//
//        server.setProvPwd(res_.getSsid());
//        server.setProvSsid(res_.getPwd());
//        server.init();
//        ESP_LOGI(TAG, "Start ProvServer Mode!");
//    }
//
//    if (mode == OkWifiStartMode::ModeWaitServerCompleted) {
//        server.waitCompleted();
//        ESP_LOGI(TAG, "Wait ProvServer Completed!");
//    }

    ok_wifi::OkWifi::getInstance().init();

//    if ((*ok_wifi::OkWifi::getInstance()).joinable()) {
//        ESP_LOGI(TAG, "Wait Prov Complete!");
//        (*ok_wifi::OkWifi::getInstance()).join();
//    }
    ok_wifi::OkWifi::getInstance().join();
    ESP_LOGI(TAG, "Wait Prov Complete!");

    ESP_LOGI(TAG, "Got ssid and pwd: %s %s", ok_wifi::OkWifi::getInstance().getProvSsidRes().c_str(),
             ok_wifi::OkWifi::getInstance().getProvPwdRes().c_str());

    now_status = Status::Ending;

//    auto &client = ok_wifi::ProvClient::getInstance();
//    do {
//        client.init();
//
//        if (client.waitWifiConnect() == ok_wifi::ClientProvResult::ProvConnected) {
//            ESP_LOGI(TAG, "Connect Successful!");
//        } else {
//            ESP_LOGE(TAG, "Connect Failed!");
//            break;
//        }
//
//        if (client.sendRequest()) {
//            ESP_LOGI(TAG, "Request Successful!");
//            ESP_LOGI(TAG, "Result: ssid: [%s] pwd: [%s]", client.getProvSsid().c_str(), client.getProvPwd().c_str());
//        } else {
//            ESP_LOGE(TAG, "Request Failed!");
//            break;
//        }
//
//    } while (false);
//    client.deinit();

//    auto &server = ok_wifi::ProvServer::getInstance();
//    server.setProvPwd("12111");
//    server.setProvSsid("sssaa");
//    server.init();
//    server.waitCompleted();


//    scanner.init();
//    ESP_LOGI(TAG, "Start Scanner");
//    std::this_thread::sleep_for(5s);

//    if (scanner.checkFounded()) {
//        ESP_LOGI(TAG, "First Check founded");
//    } else {
//        // Wait 20s
//        while (true) {
//            try {
//                if (scanner.wait(20)) {
//                    ESP_LOGI(TAG, "Founded %s", scanner.getServerSsid().c_str());
//                    break;
//                }
//                // Founded
//            } catch (idf::event::EventException &exception) {
//                ESP_LOGE(TAG, "Error %s", exception.what());
//            }
//        }
//    }
//    scanner.deinit();
//

    while (true) {
        std::cout << "Tick Time" << std::endl;
        std::this_thread::sleep_for(10s);
    }
}
