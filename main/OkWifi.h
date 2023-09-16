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
        std::string service_ssid;
        std::string ap_pwd = "OkWifi";


        std::thread thread;

        OkWifiStartMode nowMode;
    public:

        /**
         * Create Thread to run it.
         */
        OkWifi();

        /**
         * Start Scan, if error it will call OkWifi And Run it.
         */
        void run();

        void init();

        void deinit();

        OkWifiStartMode getStatus() {
            return nowMode;
        }

        std::thread& operator*() {
            return thread;
        };

        static OkWifi &getInstance() {
            static OkWifi instance;
            return instance;
        };
    };

} // OKWIFI
#endif //OKWIFI_OKWIFI_H
