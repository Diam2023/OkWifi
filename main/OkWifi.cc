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
        pthread_attr_t attribute;
        pthread_attr_init(&attribute);
        pthread_attr_setstacksize(&attribute, 1024 * 10);
        pthread_create(&thread, &attribute, [](void *arg) -> void * {
            auto obj = reinterpret_cast<OkWifi *>(arg);
            obj->run();
            return reinterpret_cast<void *>(NULL);
        }, reinterpret_cast<void *>(this));
    }

    void OkWifi::run() {
        ProvServerScanner::getInstance().init();
//        ESP_LOGI(TAG, "Start first scan");
        std::this_thread::sleep_for(2s);

        bool firstScanResult = ProvServerScanner::getInstance().checkFounded();
        // stop scanner
//        ESP_LOGW(TAG, "Deinit Scanner");
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
//                        ESP_LOGI(TAG, "Init ble prov");
                        BleProv::getInstance().init();
                        nowMode = OkWifiStartMode::ModeWaitBleProvAndServer;
                    }
                    break;
                case OkWifiStartMode::ModeWaitBleProvAndServer:
//                    ESP_LOGI(TAG, "ModeWaitBleProvAndServer Start");
//                    ESP_LOGI(TAG, "Wait 1s for BleProv");
                    if (BleProv::getInstance().wait(1)) {
                        if (res_.getResult() == ok_wifi::ProvResultStatus::ResOk) {
//                            ESP_LOGI(TAG, "Completed Prov SSID: %s PWD: %s", res_.getSsid().c_str(),
//                                     res_.getPwd().c_str());
                            nowMode = OkWifiStartMode::ModeServer;
                            BleProv::getInstance().stop();
                            continue;
                        } else {
                            ESP_LOGE(TAG, "Error for Prov");
                            // Restart Prov
                            std::this_thread::sleep_for(3s);
                        }
                    }

                    // ERROR Handler
                    // Resolve conflict when run wifi prov and scan action
                    static wifi_prov_sta_fail_reason_t reason;
                    if (wifi_prov_mgr_get_wifi_disconnect_reason(&reason) == ESP_OK) {
                        // Restart
//                        ESP_LOGI(TAG, "Restart BleProv");
                        BleProv::getInstance().stop();
                        BleProv::getInstance().init();
                    }

                    std::this_thread::sleep_for(5s);
                    static wifi_prov_sta_state_t state;
                    if (wifi_prov_mgr_get_wifi_state(&state) == ESP_OK)
                    {
                        if (state == WIFI_PROV_STA_CONNECTING || state == WIFI_PROV_STA_CONNECTED)
                        {
                            // Accepted
                            ESP_LOGW(TAG, "Wifi PROV Accepted");
                            continue;
                        }
                    }
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

                    ProvServer::getInstance().setProvSsid(res_.getSsid());
                    ProvServer::getInstance().setProvPwd(res_.getPwd());

                    prov_ssid_res = res_.getSsid();
                    prov_pwd_res = res_.getPwd();

                    ProvServer::getInstance().setOutOfDate(provServerLifeTime);
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

                        prov_ssid_res = ProvClient::getInstance().getProvSsid();
                        prov_pwd_res = ProvClient::getInstance().getProvPwd();
                        nowMode = OkWifiStartMode::ModeCompleted;
                        ProvClient::getInstance().deinit();
                        break;
                    } else {
                        ESP_LOGE(TAG, "Request Failed!");
                        continue;
                    }
                    break;
                case OkWifiStartMode::ModeWaitServerCompleted:
                    ProvServer::getInstance().waitCompleted();
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
        join();
    }

    const std::string &OkWifi::getProvSsidRes() const {
        return prov_ssid_res;
    }

    const std::string &OkWifi::getProvPwdRes() const {
        return prov_pwd_res;
    }

}
