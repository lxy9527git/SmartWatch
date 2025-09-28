/**
 * @file TextToSpeech.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "TextToSpeech.hpp"

namespace fml{

    void TextToSpeech::TtsTask(void *pvParam)
    {
        TextToSpeech* tts = (TextToSpeech*)pvParam;
        std::vector<int16_t> pcm_vector;

        while(true){
            xQueueReceive(tts->g_speechinfo_que, &tts->speechinfo, portMAX_DELAY);
            /*文字解析成拼音*/
            if(esp_tts_parse_chinese(tts->tts_handle, tts->speechinfo.speech_string.c_str())){
                int len = 0;
                do{
                    /*拼音转换成pcm音频*/
                    short* pcm_data = esp_tts_stream_play(tts->tts_handle, &len, (tts->speechinfo.speech_speed > 5 ? 5 : tts->speechinfo.speech_speed));
                    if(len > 0 && tts->audio_data_cb != NULL){
                        pcm_vector.assign(pcm_data, pcm_data + len);
                        /*播放音频*/
                        tts->audio_data_cb(pcm_vector);
                    }

                }while(len > 0);
            }
            /*重置 tts 流并清除 TTS 实例的所有缓存*/
            esp_tts_stream_reset(tts->tts_handle);
        }
    }

    TextToSpeech::TextToSpeech()
    {
        speechinfo.speech_string = "";
        speechinfo.speech_speed = 3;
        g_speechinfo_que = NULL;
        TtsTask_handle = NULL;
        tts_handle = NULL;
        audio_data_cb = NULL;
    }

    TextToSpeech::~TextToSpeech()
    {
        if(g_speechinfo_que != NULL)vQueueDelete(g_speechinfo_que);
        if(TtsTask_handle != NULL)vTaskDelete(TtsTask_handle);
        if(tts_handle != NULL)esp_tts_destroy(tts_handle);
    }

    void TextToSpeech::tts_register_set_audio_callback(AudioDataCallBack_t audio_data_cb_)
    {
        if(audio_data_cb_ != NULL){
            audio_data_cb = audio_data_cb_;
        }
    }

    esp_err_t TextToSpeech::init()
    {
        /*获取队列*/
        g_speechinfo_que = xQueueCreate(1, sizeof(struct tts_speechinfo_t));

        /*** 1. create esp tts handle ***/
        /*initial voice set from separate voice data partition*/
        const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "voice_data");
        if (part == NULL) { 
            ESP_LOGE(TAG, "Couldn't find voice data partition!\n"); 
            return ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "voice_data paration size:%d\n", part->size);
        }

        void* voicedata;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        esp_partition_mmap_handle_t mmap;
        esp_err_t err = esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, (const void**)&voicedata, &mmap);
#else
        spi_flash_mmap_handle_t mmap;
        esp_err_t err = esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &voicedata, &mmap);
#endif

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Couldn't map voice data partition!\n"); 
            return ESP_FAIL;
        }
        /*配置tts的声音配置文件*/
        esp_tts_voice_t* voice = esp_tts_voice_set_init(&esp_tts_voice_xiaole, (int16_t*)voicedata); 
        /*创建tts对象*/
        tts_handle = esp_tts_create(voice);

        BaseType_t ret_val = xTaskCreatePinnedToCore(TtsTask, "TtsTask", 6*1024, this, TEXTTOSPEECH_TTS_TASK_PRIOR, &TtsTask_handle, TEXTTOSPEECH_TTS_TASK_CORE);
        ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create TtsTask");

        return ESP_OK;

    }

    BaseType_t TextToSpeech::tts_set_speech(struct tts_speechinfo_t *data, TickType_t xTicksToWait)
    {
        return xQueueSend(g_speechinfo_que, data, xTicksToWait);
    }

}
