//
// Created by 35691 on 9/14/2023.
//

#include "BleProv.h"

#include <wifi_provisioning/scheme_ble.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_cxx.hpp>

namespace ok_wifi {
    using namespace std::chrono_literals;

    static const char *TAG = "BleProv";

    BleProv::BleProv()
            : thread(), result(ProvStatus::ProvPrepare, ""), serviceName("WaterBox_Device"), net(nullptr),
              provTimeout(200s) {
        result.setResult(ProvResultStatus::ResUnknown);
    }

    void BleProv::run() {
        exitSignal = false;
        ESP_LOGI(TAG, "Run");
        net = esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

        wifi_prov_mgr_config_t config{};

        config.scheme = wifi_prov_scheme_ble;
        config.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;
        wifi_prov_mgr_reset_provisioning();

        wifi_prov_mgr_init(config);

        const char *service_name = "WaterBox_BLU";
        wifi_prov_security_t security = WIFI_PROV_SECURITY_0;

        // 设定不自动停止
        wifi_prov_mgr_disable_auto_stop(10000);

        // 开始配网
        wifi_prov_mgr_start_provisioning(security, nullptr, service_name, nullptr);
        result.setStatus(ProvStatus::ProvWaiting);

        // 指示变量
        bool provisioned = false;
        static wifi_prov_sta_fail_reason_t reason;
        static wifi_prov_sta_state_t state;
        static wifi_config_t wifi_cfg;

        // 设置 超时时间
        auto to = provTimeout;

        while (true) {

            // 识别退出信号 由stop发出
            if (exitSignal)
            {
                break;
            }
            wifi_prov_mgr_is_provisioned(&provisioned);
            wifi_prov_mgr_get_wifi_state(&state);

            // 实验记录：
            // 当连接BLE时wifi_prov_mgr_is_provisioned为真 此时State为1 Reason获取失败
            // 当尝试连接WIFI失败时 State为2 Reason为0
            // 当WIFI连接成功时    State为1 Reason无法获取
            // 当调用wifi_prov_mgr_reset_sm_state_for_reprovision时provisioned将会被重置

            if ((provisioned) && (state == WIFI_PROV_STA_DISCONNECTED) &&
                (wifi_prov_mgr_get_wifi_disconnect_reason(&reason) == ESP_OK)) {
                // 连接失败，重新启动配网
                ESP_LOGW(TAG, "Restore prov");
                // 重置配网回到初始状态
                wifi_prov_mgr_reset_sm_state_for_reprovision();

                // 阻塞停止
                wifi_prov_mgr_stop_provisioning_block();
                std::this_thread::sleep_for(2s);
                // 检测是否完全停止
                while (!wifi_prov_mgr_is_stopped()) {
                    wifi_prov_mgr_stop_provisioning_block();
                    std::this_thread::sleep_for(100ms);
                }
                // 重新启动
                ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, nullptr, service_name, nullptr));
                continue;
            }
            if ((state == WIFI_PROV_STA_CONNECTED) && (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) == ESP_OK)) {
                // 连接成功
                std::string ssid = reinterpret_cast<char *>(wifi_cfg.sta.ssid);
                std::string pwd = reinterpret_cast<char *>(wifi_cfg.sta.password);
                result.setSsid(ssid);
                result.setPwd(pwd);
                result.setResult(ProvResultStatus::ResOk);
                ESP_LOGI(TAG, "Connected");
                return;
            }

            std::this_thread::sleep_for(1s);
            to -= 1s;
            if (to <= 0s) {
                result.setResult(ProvResultStatus::ResError);
                break;
            }
        }
    }

    ProvResult &BleProv::getProvResult() {
        return result;
    }

    bool BleProv::wait(long timeout) {

        // 等待timeout时间 如果有结果则返回真
        long count = timeout * 2;
        while (result.getResult() == ok_wifi::ProvResultStatus::ResUnknown) {
            std::this_thread::sleep_for(500ms);
            if (timeout == 0) {
                continue;
            }

            count--;
            if (count <= 0) {
                return false;
            }
        }
        return true;
    }

    void BleProv::init() {
        ESP_LOGI(TAG, "Init");

        if (!thread.joinable()) {
            thread = std::thread(&BleProv::run, this);
        }
    }

    void BleProv::stop() {
        ESP_LOGW(TAG, "Stop");
        exitSignal = true;
        wifi_prov_mgr_stop_provisioning();
        if (thread.joinable()) {
            thread.join();
        }
        wifi_prov_mgr_reset_provisioning();
        wifi_prov_mgr_deinit();
        esp_wifi_disconnect();
        esp_wifi_stop();
        esp_wifi_restore();
        esp_wifi_deinit();
        if (net != nullptr) {
            esp_netif_destroy_default_wifi(net);
        }
        result.setResult(ProvResultStatus::ResUnknown);
    }

    void BleProv::restart() {
        ESP_LOGI(TAG, "Restart");
        BleProv::stop();
        BleProv::init();
    }
} // ok_wifi