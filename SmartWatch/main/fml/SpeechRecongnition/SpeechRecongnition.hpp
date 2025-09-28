/**
 * @file SpeechRecongnition.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <esp_check.h>
#include <esp_log.h>
#include <vector>
#include <string>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_mn_speech_commands.h"

namespace fml{

    class SpeechRecongnition
    {
        #define SPEECHRECONGNITION_MN_COMMAND_MAX_NUM                               (200)                   /*MultiNet 是为了在 ESP32-S3 系列上离线实现多命令词识别而设计的轻量化模型，目前支持 200 个以内的自定义命令词识别。*/
        #define SPEECHRECONGNITION_MN_CONFIG_TIMEOUT_MS                             (1000 * 60 * 10)

        #define SPEECHRECONGNITION_FEED_TASK_PRIOR                                  (3)
        #define SPEECHRECONGNITION_FEED_TASK_CORE                                   (1)
        #define SPEECHRECONGNITION_DETECT_TASK_PRIOR                                (3)
        #define SPEECHRECONGNITION_DETECT_TASK_CORE                                 (0)
        
        typedef int  (*AudioDataCallBack_t)(std::vector<int16_t>& data);
        typedef void (*MultinetCleanCallBack_t)(void* user_data, char* pinyin);
        typedef void (*MultinetResultCallBack_t)(void* user_data, char* pinyin);       

        public:
            struct sr_result_t{
                esp_mn_state_t      state;
                int                 command_id;
                std::string         out_string; 
            };
            /*获取单例实例的静态方法*/
            inline static SpeechRecongnition& getInstance() {
                static SpeechRecongnition instance;
                return instance;
            }
            void sr_register_get_audio_callback(AudioDataCallBack_t audio_data_cb_);
            esp_err_t init(const char *input_format = "MR", char** command_list = NULL, uint8_t command_list_num = 0);
            void sr_multinet_clean(MultinetCleanCallBack_t cb, void* user_data);
            void sr_multinet_result(MultinetResultCallBack_t cb, void* user_data);
            BaseType_t sr_get_result(struct sr_result_t *result, TickType_t xTicksToWait);
            static bool sr_add_command_list(char** command_list = NULL, uint8_t command_list_num = 0);
            static void sr_remove_command_list(char** command_list = NULL, uint8_t command_list_num = 0);

        private:
            const char* TAG = "SpeechRecongnition";
            struct sr_result_t sr_result;
            model_iface_data_t* model_data;
            esp_mn_iface_t* multinet;
            esp_afe_sr_iface_t* afe_iface;
            esp_afe_sr_data_t* afe_data;
            afe_config_t* afe_config;
            QueueHandle_t g_result_que;
            srmodel_list_t* models;
            TaskHandle_t FeedTask_handle; 
            TaskHandle_t DetectTask_handle; 
            AudioDataCallBack_t audio_data_cb;
            bool clean_trigger;
            void* clean_trigger_user_data;
            MultinetCleanCallBack_t clean_trigger_callback;
            void* result_user_data;
            MultinetResultCallBack_t result_callback;
            static void sr_send_result(SpeechRecongnition* sr, esp_mn_results_t *mn_result, bool is_clean_trigger);
            static void FeedTask(void *pvParam);
            static void DetectTask(void *pvParam);
            /*私有构造函数，禁止外部直接实例化*/
            SpeechRecongnition();
            ~SpeechRecongnition();
            /*禁止拷贝构造和赋值操作*/
            SpeechRecongnition(const SpeechRecongnition&) = delete;
            SpeechRecongnition& operator = (const SpeechRecongnition&) = delete;
    };

}