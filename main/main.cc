/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <memory>
#include <thread>

#include "esp_timer_cxx.hpp"
#include "gpio_cxx.hpp"

// ms s us
using namespace std::chrono_literals;

/**
 * CXX Main Task
 */
_Noreturn inline void cpp_main() {

    // Create A Timer For Application
    auto led_fir = idf::GPIO_Output(idf::GPIONum(12));
    auto led_sec = idf::GPIO_Output(idf::GPIONum(13));
    static bool led_fir_status = false;
    static bool led_sec_status = false;
    auto timer = std::make_unique<idf::esp_timer::ESPTimer>([&led_fir]() {
        if (led_fir_status)
        {
            led_fir.set_high();
        } else {
            led_fir.set_low();
        }
        led_fir_status = !led_fir_status;

        std::cout << "Test Timer Output" << std::endl;

    }, "TestTimer");

    // Start Timer  ms
    timer->start_periodic(500ms);

    while (true) {

        if (led_sec_status)
        {
            led_sec.set_high();
        } else {
            led_sec.set_low();
        }
        led_sec_status = !led_sec_status;

        std::cout << "Test out put" << std::endl;
        std::this_thread::sleep_for(2000ms);
    }
}

extern "C"
{

/**
 * @brief Application main
 * This Function Will Run With MainTask And When Called app_main() MainTask Ending;
 */
[[maybe_unused]] void app_main(void) {
    cpp_main();
}

}