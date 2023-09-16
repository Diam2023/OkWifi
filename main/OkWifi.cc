//
// Created by 35691 on 9/14/2023.
//

#include "OkWifi.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <memory>
#include <cstring>
#include "ProvServer.h"

namespace ok_wifi {
    using namespace std::chrono_literals;

    static const char *TAG = "OkWifi";

    OkWifi::OkWifi() {
    }
}
