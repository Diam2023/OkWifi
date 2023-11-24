//
// Created by 35691 on 9/14/2023.
//

#ifndef OKWIFI_OKWIFI_H
#define OKWIFI_OKWIFI_H

#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <esp_netif_types.h>
#include "BleProv.h"


namespace ok_wifi {

    enum class OkWifiStartMode {
        ModeUnInitialize,
        ModeWaitBleProvAndServer,
        ModeServer,
        ModeClient,
        ModeWaitServerCompleted,
        ModeCompleted
    };

    /**
     * This Class Will Scan Ap If exist Prov Ap, this will connect it, And When Ap is not exist, it will call ble Prov
     * It Will steal scanning when ble prov is processing.
     */
    class OkWifi {
    private:
        // Ap station to completed
        std::string prov_ssid_res;
        std::string prov_pwd_res;
        pthread_t thread;

        OkWifiStartMode nowMode;

        /**
         * 外部停止信号
         */
        bool stopSignal;

        /**
         * 线程存活状态 true表示退出
         */
        volatile bool threadStatus;
    public:

        [[nodiscard]] const std::string &getProvSsidRes() const;

        [[nodiscard]] const std::string &getProvPwdRes() const;

        /**
         * Create Thread to run it.
         */
        OkWifi();

        /**
         * Start Scan, if error it will call OkWifi And Run it.
         */
        void run();

        void init();

        void waitExit();

        /**
         * 检查OkWifi主线程是否退出
         * @return true已经退出 false运行中
         */
        bool checkExit();

        OkWifiStartMode getStatus() {
            return nowMode;
        }

        void join() const {
            pthread_join(thread, nullptr);
        };

        static OkWifi &getInstance() {
            static OkWifi instance;
            return instance;
        };

        /**
         * 强制停止配网
         */
        void stop();
    };

} // OKWIFI
#endif //OKWIFI_OKWIFI_H
