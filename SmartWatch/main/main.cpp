/**
 * @file main.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "apl.hpp"

static void CPU_PrintInfo()
{
    /*
    vTaskGetRunTimeStats() 使用
    注意:
    使用 vTaskGetRunTimeStats() 前需使能:
    make menuconfig -> Component config -> FreeRTOS -> configUSE_TRACE_FACILITY
    make menuconfig -> Component config -> FreeRTOS -> Enable FreeRTOS trace facility -> configUSE_STATS_FORMATTING_FUNCTIONS
    make menuconfig -> Component config -> FreeRTOS -> Enable display of xCorelD in vlaskList
    make menuconfig -> Component config -> FreeRTOS -> configGENERATE_RUN_TIME_STATS
    通过上面配置，等同于使能 FreeRTOSConfig.h 中如下三个宏:
    configGENERATE_RUN_TIME_STATS，configUSE_STATS_FORMATTING_FUNCTIONS 和 configSUPPORT_DYNAMIC_ALLOCATION
    */

    static uint8_t CPU_RunInfo[400]; //保存任务运行时间信息
    
    // memset(CPU_RunInfo, 0, 400); // 信息缓冲区清零
    // vTaskList((char *)&CPU_RunInfo); // 获取任务列表信息

    // ESP_LOGI("app_main","----------------------------------------------------");
    // ESP_LOGI("app_main","任务列表 (名称, 状态, 优先级, 剩余堆栈, ID):");
    // ESP_LOGI("app_main","%s", CPU_RunInfo);
    
    // 新增：显示每个任务的堆栈使用情况
    ESP_LOGI("app_main","\n堆栈使用情况:");
    ESP_LOGI("app_main","任务名称        分配大小(字)  高水位线(字)  使用率");
    
    // 获取当前任务数量
    UBaseType_t numTasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *taskStatusArray = (TaskStatus_t *)malloc(numTasks * sizeof(TaskStatus_t));
    
    if (taskStatusArray) {
        // 获取所有任务状态
        numTasks = uxTaskGetSystemState(taskStatusArray, numTasks, NULL);
        
        for (UBaseType_t i = 0; i < numTasks; i++) {
            const char *taskName = taskStatusArray[i].pcTaskName;
            UBaseType_t stackSize = taskStatusArray[i].usStackHighWaterMark;
            
            // 计算堆栈使用情况
            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(taskStatusArray[i].xHandle);
            float usagePercent = 100.0 * (1.0 - ((float)highWaterMark / stackSize));
            
            ESP_LOGI("app_main", "%-16s %-14u %-14u %.1f%%", 
                    taskName, stackSize, highWaterMark, usagePercent);
        }
        
        free(taskStatusArray);
    } else {
        ESP_LOGE("app_main", "内存不足，无法分配任务状态数组");
    }

    // memset(CPU_RunInfo, 0, 400); // 信息缓冲区清零
    // vTaskGetRunTimeStats((char *)&CPU_RunInfo); // 获取任务运行时间统计

    // ESP_LOGI("app_main","\n任务运行时间统计:");
    // ESP_LOGI("app_main","%s", CPU_RunInfo);

    /*详细内存统计: MALLOC_CAP_INTERNAL ...*/
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    ESP_LOGI("app_main","MALLOC_CAP_INTERNAL: free=%dKB, alloc=%dKB, largest=%d, mini=%d, allocb=%d, freeb=%d, tb=%d",
    info.total_free_bytes/1024, 
    info.total_allocated_bytes/1024,
    info.largest_free_block,
    info.minimum_free_bytes,
    info.allocated_blocks,
    info.free_blocks,
    info.total_blocks);

    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    ESP_LOGI("app_main","MALLOC_CAP_SPIRAM: free=%dKB, alloc=%dKB, largest=%d, mini=%d, allocb=%d, freeb=%d, tb=%d",
    info.total_free_bytes/1024, 
    info.total_allocated_bytes/1024,
    info.largest_free_block,
    info.minimum_free_bytes,
    info.allocated_blocks,
    info.free_blocks,
    info.total_blocks);
}

extern "C" void app_main(void)
{
    /*初始化驱动层*/
    fml::HdlManager::getInstance().init();
    /*初始化应用层*/
    apl::apl::getInstance().init();
    
    while (1) {
        fml::HdlManager::getInstance().update();
        apl::apl::getInstance().update();
        vTaskDelay(pdMS_TO_TICKS(10));

        /*打印信息*/
        static int count = 0;
        count++;
        if(count >= 500){
            count = 0;
            CPU_PrintInfo();
        }
        
    }
}
