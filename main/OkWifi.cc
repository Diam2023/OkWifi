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
        // 初始化扫描配网服务器 打开自动扫描器
        ProvServerScanner::getInstance().init();
        std::this_thread::sleep_for(2s);

        // 获取自动扫描结果
        bool firstScanResult = ProvServerScanner::getInstance().checkFounded();

        // 停止自动扫描器
        ProvServerScanner::getInstance().deinit();

        // 获取蓝牙配网结果的实例指针
        auto &res_ = BleProv::getInstance().getProvResult();

        // 初始化退出标记
        bool exitFlag = false;

        while (true) {
            // 检查退出标记
            if (exitFlag) {
                break;
            }

            // 有限状态机 状态循环

            switch (nowMode) {
                case OkWifiStartMode::ModeUnInitialize:
                    // 状态机起始状态
                    ESP_LOGI(TAG, "ModeUnInitialize");
                    // 通过第一次自动扫描的结果 选择启动模式
                    if (firstScanResult) {
                        // 直接连接配网服务器进行配网
                        nowMode = OkWifiStartMode::ModeClient;
                    } else {
                        // 接收蓝牙配网信息的同时扫描配网服务器
                        nowMode = OkWifiStartMode::ModeWaitBleProvAndServer;
                    }
                    break;
                case OkWifiStartMode::ModeWaitBleProvAndServer:
                    ESP_LOGI(TAG, "ModeWaitBleProvAndServer");
                    // 蓝牙配网初始化
                    BleProv::getInstance().init();

                    // 阻塞1s监听蓝牙配网的状态
                    if (BleProv::getInstance().wait(1)) {
                        // 有结果
                        switch (res_.getResult()) {
                            case ProvResultStatus::ResOk:
                                // 蓝牙配网成功
                                // 转换模式为 作为配网服务器 分发配网消息给客户端
                                nowMode = OkWifiStartMode::ModeServer;

                                // 停止蓝牙配网并释放蓝牙内存空间
                                BleProv::getInstance().stop();

                                // 进入配网服务器状态
                                continue;
                            case ProvResultStatus::ResUnknown:
                                // 未知状态/默认状态
                                [[fallthrough]];
                            case ProvResultStatus::ResError:
                                // 蓝牙配网超时
                                // std::this_thread::sleep_for(1s);
                                // 再次开始蓝牙配网
                                [[fallthrough]];
                            default:
                                break;
                        }
                    }

                    // 配网没有结果/等待响应超时
                    std::this_thread::sleep_for(1s);

                    static wifi_prov_sta_state_t state;
                    // 获取配网状态
                    if (wifi_prov_mgr_get_wifi_state(&state) == ESP_OK) {
                        // 如果连接成功就等待配网结果
                        if (state == WIFI_PROV_STA_CONNECTED) {
                            // 连接成功后检查配网成功后等待最多15s获取结果
                            if (BleProv::getInstance().wait(15)) {
                                // 获取到结果，之后回到最初再次判断结果
                                continue;
                            } else {
                                // 超时没有结果就重启配网
                                BleProv::getInstance().restart();
                            }
                        }
                    }

                    // 尝试单次扫描检查是否有配网服务器
                    try {
                        if (ProvServerScanner::getInstance().scanOnce(1s)) {

                            // 扫描到目标服务器
                            ESP_LOGI(TAG, "Found ProvServer Name: %s",
                                     ProvServerScanner::getInstance().getServerSsid().c_str());

                            // 停止蓝牙配网
                            BleProv::getInstance().stop();
                            // 改变状态到连接配网服务器模式
                            nowMode = OkWifiStartMode::ModeClient;

                            continue;
                        }
                    } catch (idf::event::EventException &exception) {
                        // 检查到在蓝牙配网过程中扫描WIFI SSID出现冲突的错误
                        if (exception.error == ESP_ERR_WIFI_STATE) {
                            // 停止SSID扫描功能
                            ESP_LOGW(TAG, "Acc Stop Scan for 5s!");
                            // 重启WIFI连接
                            esp_wifi_scan_stop();
                            esp_wifi_disconnect();
                            esp_wifi_connect();
                            // Stop Scan When Ble Received Prov Message
                            // 等待5s 等待WIFI响应
                            std::this_thread::sleep_for(5s);
                        }
                    }
                    std::this_thread::sleep_for(1s);
                    break;
                case OkWifiStartMode::ModeServer:
                    ESP_LOGI(TAG, "ModeServer");

                    // 重置默认EventLoop
                    esp_event_loop_delete_default();
                    esp_event_loop_create_default();

                    // 获取默认SSID与默认PWD
                    ProvServer::getInstance().setProvSsid(res_.getSsid());
                    ProvServer::getInstance().setProvPwd(res_.getPwd());

                    prov_ssid_res = res_.getSsid();
                    prov_pwd_res = res_.getPwd();

                    // 设定检查客户端活动周期的时间
                    ProvServer::getInstance().init();

                    // 进入等待服务器生命结束状态
                    nowMode = OkWifiStartMode::ModeWaitServerCompleted;
                    continue;
                case OkWifiStartMode::ModeClient:
                    ESP_LOGI(TAG, "ModeClient");

                    // 设定预定服务器SSID与PWD
                    ProvClient::getInstance().setServerSsid(ProvServerScanner::getInstance().getServerSsid());

                    // 初始化配网客户端
                    ProvClient::getInstance().init();

                    // 等待服务器响应 检查服务器连接状态
                    if (ProvClient::getInstance().waitWifiConnect() !=
                        ok_wifi::ClientProvResult::ProvConnected) {
                        ESP_LOGE(TAG, "Connect Failed!");
                        // 连接失败重新进入监听状态
                        nowMode = OkWifiStartMode::ModeWaitBleProvAndServer;
                        continue;
                    }

                    // 连接成功，发送Http请求
                    if (ProvClient::getInstance().sendRequest()) {
                        // 请求成功，获取配网SSID与PWD
                        prov_ssid_res = ProvClient::getInstance().getProvSsid();
                        prov_pwd_res = ProvClient::getInstance().getProvPwd();
                        ProvClient::getInstance().deinit();
                        // 结束状态
                        nowMode = OkWifiStartMode::ModeCompleted;
                    } else {
                        ESP_LOGE(TAG, "Request Failed!");
                        // 请求失败 并超时 重新进入监听模式
                        nowMode = OkWifiStartMode::ModeWaitBleProvAndServer;
                    }
                    break;
                case OkWifiStartMode::ModeWaitServerCompleted:
                    // 等待服务器生命结束
                    ProvServer::getInstance().waitCompleted();

                    // 进入配网完成状态
                    nowMode = OkWifiStartMode::ModeCompleted;
                    break;
                case OkWifiStartMode::ModeCompleted:
                    // 状态机 结束状态
                    exitFlag = true;
                    ESP_LOGI(TAG, "OkWifi Prov Down!");
                    break;
            }

            ESP_LOGI(TAG, "Mode Circle");
            std::this_thread::sleep_for(2s);
        }
        ESP_LOGI(TAG, "End Lifetime");
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
