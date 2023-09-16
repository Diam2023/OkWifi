//
// Created by 35691 on 9/14/2023.
//

#include "OkWifi.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include <wifi_provisioning/wifi_config.h>
#include <wifi_provisioning/manager.h>
#include "ProvServer.h"
#include "ProvClient.h"
#include "ProvServerScanner.h"
#include "esp_event_cxx.hpp"

namespace ok_wifi {
    using namespace std::chrono_literals;

    static const char *TAG = "OkWifi";

    OkWifi::OkWifi() : nowMode(OkWifiStartMode::ModeUnInitialize) {
    }

    void OkWifi::init() {
        thread = std::thread(&OkWifi::run, this);
    }

    void OkWifi::run() {
        ProvServerScanner::getInstance().init();
        ESP_LOGI(TAG, "Start first scan");
        std::this_thread::sleep_for(2s);

        bool firstScanResult = ProvServerScanner::getInstance().checkFounded();
        // stop scanner
        ESP_LOGW(TAG, "Deinit Scanner");
        ProvServerScanner::getInstance().deinit();
        auto &res_ = BleProv::getInstance().getProvResult();

        bool exitFlag = false;
        while (true) {
            if (exitFlag) {
                break;
            }
            switch (nowMode) {
                case OkWifiStartMode::ModeUnInitialize:
                    // init
                    if (firstScanResult) {
                        nowMode = OkWifiStartMode::ModeClient;
                    } else {
                        ESP_LOGI(TAG, "Init ble prov");
                        BleProv::getInstance().init();
                        nowMode = OkWifiStartMode::ModeWaitBleProvAndServer;
                    }
                    break;
                case OkWifiStartMode::ModeWaitBleProvAndServer:
                    ESP_LOGI(TAG, "ModeWaitBleProvAndServer Start");

                    ESP_LOGI(TAG, "Wait 1s for BleProv");
                    if (BleProv::getInstance().wait(1)) {
                        if (res_.getResult() == ok_wifi::ProvResultStatus::ResOk) {
                            ESP_LOGI(TAG, "Completed Prov SSID: %s PWD: %s", res_.getSsid().c_str(),
                                     res_.getPwd().c_str());
                            nowMode = OkWifiStartMode::ModeServer;
                            BleProv::getInstance().stop();
                            continue;
                        } else {
                            ESP_LOGE(TAG, "Error for Prov");
                            std::this_thread::sleep_for(3s);
                        }
                    }

                    // Resolve conflict when run wifi prov and scan action
                    wifi_prov_sta_fail_reason_t reason;
                    if (wifi_prov_mgr_get_wifi_disconnect_reason(&reason) == ESP_OK) {
                        // Restart
                        ESP_LOGI(TAG, "Restart BleProv");
                        BleProv::getInstance().stop();
                        BleProv::getInstance().init();
                    }

                    std::this_thread::sleep_for(5s);
                    try {
                        ESP_LOGI(TAG, "Scan once wait for 1s");
                        if (ProvServerScanner::getInstance().scanOnce(1s)) {

                            ESP_LOGW(TAG, "Stopping Ble Prov!");
                            BleProv::getInstance().stop();
                            nowMode = OkWifiStartMode::ModeClient;
                            ProvClient::getInstance().setServerSsid(ProvServerScanner::getInstance().getServerSsid());
                            ProvClient::getInstance().init();
                            continue;
                        }
                    } catch (idf::event::EventException &exception) {
                        if (exception.error == ESP_ERR_WIFI_STATE) {
                            ESP_LOGW(TAG, "Stop Scan for 20s!");
                            ESP_LOGI(TAG, "Reconnect Prov");
                            esp_wifi_scan_stop();
                            esp_wifi_disconnect();
                            esp_wifi_connect();
                            // Stop Scan When Ble Received Prov Message
                            std::this_thread::sleep_for(20s);
                        }
                    }
                    std::this_thread::sleep_for(5s);
                    break;
                case OkWifiStartMode::ModeServer:
                    ESP_LOGI(TAG, "Server Mode Init!");
                    esp_event_loop_delete_default();
                    esp_event_loop_create_default();

                    ProvServer::getInstance().setProvPwd(res_.getSsid());
                    ProvServer::getInstance().setProvSsid(res_.getPwd());
                    ProvServer::getInstance().init();

                    ESP_LOGI(TAG, "Prepare Sending Prov Data SSID: %s PWD: %s", res_.getSsid().c_str(),
                             res_.getPwd().c_str());
                    nowMode = OkWifiStartMode::ModeWaitServerCompleted;
                    break;
                case OkWifiStartMode::ModeClient:
                    ESP_LOGI(TAG, "Client Mode!");
                    if (ProvClient::getInstance().waitWifiConnect() ==
                        ok_wifi::ClientProvResult::ProvConnected) {
                        ESP_LOGI(TAG, "Connect Successful!");
                    } else {
                        ESP_LOGE(TAG, "Connect Failed!");
                        // TODO Check count out
                        nowMode = OkWifiStartMode::ModeWaitBleProvAndServer;
                        continue;
                    }

                    if (ProvClient::getInstance().sendRequest()) {
                        ESP_LOGI(TAG, "Request Successful!");
                        ESP_LOGI(TAG, "Result: ssid: [%s] pwd: [%s]",
                                 ProvClient::getInstance().getProvSsid().c_str(),
                                 ProvClient::getInstance().getProvPwd().c_str());
                        nowMode = OkWifiStartMode::ModeCompleted;
                        ProvClient::getInstance().deinit();
                        break;
                    } else {
                        ESP_LOGE(TAG, "Request Failed!");
                        continue;
                    }
                    break;
                case OkWifiStartMode::ModeWaitServerCompleted:

                    ESP_LOGI(TAG, "Waiting ProvServer!");
                    ProvServer::getInstance().waitCompleted();
                    ESP_LOGI(TAG, "Wait ProvServer Completed!");
                    ProvServer::getInstance().stop();
                    nowMode = OkWifiStartMode::ModeCompleted;
                    break;
                case OkWifiStartMode::ModeCompleted:
                    exitFlag = true;
                    ESP_LOGI(TAG, "OkWifi Prov Down!");
                    break;
            }

            ESP_LOGI(TAG, "Mode Circle");
            std::this_thread::sleep_for(2s);
        }
    }

    void OkWifi::deinit() {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
