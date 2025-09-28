/**
 * @file TextToSpeech.hpp
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
#include <esp_log.h>
#include <esp_check.h>
#include <vector>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_system.h"
#include "esp_tts.h"
#include "esp_tts_voice_xiaole.h"
#include "esp_tts_voice_template.h"
#include "esp_tts_player.h"
#include "esp_partition.h"


namespace fml{

    class TextToSpeech
    {
        #define TEXTTOSPEECH_TTS_TASK_PRIOR                                                 (2)
        #define TEXTTOSPEECH_TTS_TASK_CORE                                                  (1)

        typedef int (*AudioDataCallBack_t)(std::vector<int16_t>& data);     
        
        public:
            struct tts_speechinfo_t{
                std::string         speech_string; 
                unsigned int        speech_speed;
            };
            /*获取单例实例的静态方法*/
            inline static TextToSpeech& getInstance() {
                static TextToSpeech instance;
                return instance;
            }
            void tts_register_set_audio_callback(AudioDataCallBack_t audio_data_cb_);
            esp_err_t init();
            BaseType_t tts_set_speech(struct tts_speechinfo_t *data, TickType_t xTicksToWait);

        private:
            const char* TAG = "TextToSpeech";
            struct tts_speechinfo_t speechinfo;
            QueueHandle_t g_speechinfo_que;
            TaskHandle_t TtsTask_handle; 
            esp_tts_handle_t tts_handle;
            AudioDataCallBack_t audio_data_cb;
            static void TtsTask(void *pvParam);
            /*私有构造函数，禁止外部直接实例化*/
            TextToSpeech();
            ~TextToSpeech();
            /*禁止拷贝构造和赋值操作*/
            TextToSpeech(const TextToSpeech&) = delete;
            TextToSpeech& operator = (const TextToSpeech&) = delete;
    };

}