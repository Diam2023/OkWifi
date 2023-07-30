#include <sys/cdefs.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"





void PutWaterTimerHandler(TimerHandle_t handler) {
    printf("Test Timer\n");
}


_Noreturn inline void app_cpp() {
    // Create A Timer For Application
    auto timer = xTimerCreate("Put Water", 500, pdTRUE, nullptr, &PutWaterTimerHandler);
    // Start Timer  ms
    xTimerStart(timer, 5000);

    int i = 0;
    while (true) {
        i++;
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}


extern "C"
{


//static void main_task(void* args)
//{
//#if !CONFIG_FREERTOS_UNICORE
//    // Wait for FreeRTOS initialization to finish on other core, before replacing its startup stack
//    esp_register_freertos_idle_hook_for_cpu(other_cpu_startup_idle_hook_cb, !xPortGetCoreID());
//    while (!s_other_cpu_startup_done) {
//        ;
//    }
//    esp_deregister_freertos_idle_hook_for_cpu(other_cpu_startup_idle_hook_cb, !xPortGetCoreID());
//#endif
//
//    // [refactor-todo] check if there is a way to move the following block to esp_system startup
//    heap_caps_enable_nonos_stack_heaps();
//
//    // Now we have startup stack RAM available for heap, enable any DMA pool memory
//#if CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL
//    if (esp_psram_is_initialized()) {
//        esp_err_t r = esp_psram_extram_reserve_dma_pool(CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL);
//        if (r != ESP_OK) {
//            ESP_EARLY_LOGE(TAG, "Could not reserve internal/DMA pool (error 0x%x)", r);
//            abort();
//        }
//    }
//#endif
//
//    //Initialize TWDT if configured to do so
//#if CONFIG_ESP_TASK_WDT_INIT
//    esp_task_wdt_config_t twdt_config = {
//            .timeout_ms = CONFIG_ESP_TASK_WDT_TIMEOUT_S * 1000,
//            .idle_core_mask = 0,
//#if CONFIG_ESP_TASK_WDT_PANIC
//            .trigger_panic = true,
//#endif
//    };
//#if CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0
//    twdt_config.idle_core_mask |= (1 << 0);
//#endif
//#if CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1
//    twdt_config.idle_core_mask |= (1 << 1);
//#endif
//    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
//#endif // CONFIG_ESP_TASK_WDT
//
//    app_main();
//    vTaskDelete(NULL);
//}


/**
 * @brief Application main
 * This Function Will Run With MainTask And When Called app_main() MainTask Ending;
 */
void app_main(void) {
    app_cpp();
}

}