/**
 * @file JpegDecoder.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <esp_log.h>
#include "esp_heap_caps.h"
#include "esp_jpeg_dec.h"
#include <sys/stat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <string>
#include <vector>
#include <memory>

namespace fml{

    class JpegDecoder
    {
        #define EVENTGROUP_JPEG_DEC_START_BIT          (1<<0)
        #define EVENTGROUP_JPEG_DEC_END_BIT            (1<<1)
        #define JPEGDECODER_TASK_PRIOR                 (2)
        #define JPEGDECODER_TASK_CORE                  (1)

        struct jpeg_stream {
            jpeg_dec_config_t       config;
            jpeg_dec_handle_t       jpeg_dec;
            jpeg_dec_io_t           jpeg_io;
            jpeg_dec_header_info_t  out_info;
            jpeg_pixel_format_t     output_type;
        };

        struct jpeg_input_info_t{
            uint8_t** jpeg_buff;
            int* jpeg_len;
            int jpeg_number;
            int jpeg_index;
        };

        typedef void (* JpegDecoderInputInfoCallBack_t)(void* input_info_user_data, int Number, int index);

        public:
            struct jpeg_output_info_t{
                uint8_t* jpeg_buff;
                int jpeg_len;
            };

            struct DecodeResult {
                bool success;
                uint16_t width;
                uint16_t height;
                std::unique_ptr<uint8_t[]> data; 
                size_t data_size;
                std::string error_message;
            };

            /*获取单例实例的静态方法*/
            inline static JpegDecoder& getInstance() {
                static JpegDecoder instance;
                return instance;
            }
            
            void Init(const char* dirpath, jpeg_pixel_format_t format, jpeg_rotate_t rotate, JpegDecoderInputInfoCallBack_t cb, void* input_info_user_data);
            void StartJpegDec(int jpeg_index, struct jpeg_output_info_t output);
            bool WaitJpegDec(int* jpeg_index, TickType_t xTicksToWait);
            static int SafeStrtoi(const char *str, int *value);/*自定义安全字符串转整数函数*/
            static bool FindJpegMaterial(const char *path, const char *target, int* start, int* end);
            static DecodeResult Decode(const uint8_t* jpeg_data, size_t jpeg_size,int target_width = 0,int target_height = 0,jpeg_pixel_format_t output_format = JPEG_PIXEL_FORMAT_RGB565_LE,jpeg_rotate_t rotate = JPEG_ROTATE_0D);
        private:
            const char* TAG = "JpegDecoder";
            EventGroupHandle_t xEventGroup;
            struct jpeg_input_info_t  jpeg_input_info;
            struct jpeg_output_info_t jpeg_output_info;
            struct jpeg_stream jpeg_stream;
            TaskHandle_t JpegDecTask_handle; 
            static long get_dir_number(const char *filepath);
            static bool get_nth_file(const char *path, int n, char* filename, int filename_len);
            static void get_jpeg_input_info(void* input_info_user_data, JpegDecoderInputInfoCallBack_t cb, const char* TAG, const char *dirpath, struct jpeg_input_info_t* jpeg_input_info);
            static void JpegDecTask(void * arg);
            /*私有构造函数，禁止外部直接实例化*/
            JpegDecoder();
            ~JpegDecoder();
            /*禁止拷贝构造和赋值操作*/
            JpegDecoder(const JpegDecoder&) = delete;
            JpegDecoder& operator = (const JpegDecoder&) = delete;
            
    };
}



