/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <memory>
#include <thread>

#include "esp_timer_cxx.hpp"

using namespace std::chrono_literals;

/**
 * CXX Main Task
 */
_Noreturn inline void cpp_main() {

    // Create A Timer For Application
    auto timer = std::make_unique<idf::esp_timer::ESPTimer>([]() {
        std::cout << "Test Timer Output" << std::endl;
    }, "TestTimer");

    // Start Timer  ms
    timer->start_periodic(500ms);

    while (true) {
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
void app_main(void) {
    cpp_main();
}

}