/**
 * @file BigModel.hpp
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
#include <freertos/semphr.h>
#include <time.h>
#include <inttypes.h>
#include "esp_http_client.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include "cJSON.h"

namespace fml{

    class BigModel
    {
        #define BIGMODEL_REQUEST_QUEUE_LEN                                                  (10)
        #define BIGMODEL_RESPONSE_QUEUE_LEN                                                 (10)

        #define BIGMODEL_REQUEST_TASK_PRIOR                                                 (2)
        #define BIGMODEL_REQUEST_TASK_CORE                                                  (0)
        #define BIGMODEL_RESPONSE_TASK_PRIOR                                                (2)
        #define BIGMODEL_RESPONSE_TASK_CORE                                                 (0)
        #define BIGMODEL_RESPONSE_RINGBUFFER_SIZE                                           (4096)
        #define BIGMODEL_MAX_RETRIES                                                        (3)                                                 /*最大重试次数*/

        #define BIGMODEL_HTTPS_URL                                                          "https://open.bigmodel.cn/api/paas/v4/chat/completions"
        #define BIGMODEL_IMAGE_HTTPS_URL                                                    "https://open.bigmodel.cn/api/paas/v4/images/generations"
        #define BIGMODEL_AUTH_SCHEME                                                        "Bearer"

        #define BIGMODEL_API_KEY_ID                                                         "7ff229ad3ed240e4af58d397c55e3d3a"                  /*您的API Key ID，      建议用户自己申请，不用和别人抢占并发数*/
        #define BIGMODEL_API_KEY_SECRET                                                     "DO88vDuO4AAoPc9u"                                  /*您的API Key Secret，  建议用户自己申请，不用和别人抢占并发数*/
        #define BIGMODEL_MODEL_NAME                                                         "glm-4-flash"                                       /*免费语言模型*/
        #define BIGMODEL_IMAGE_MODEL_NAME                                                   "cogview-3-flash"                                   /*免费图像生成模型*/

        #define BIGMODEL_MAX_TOKENS                                                         (4095)                                              /*响应长度控制,最大生成token数*/  
        #define BIGMODEL_TEMPERATURE                                                        (0.95)                                              /*控制生成随机性 (0-2)*/
        #define BIGMODEL_TOP_P                                                              (0.7)                                               /*控制生成多样性 (0-1)*/

        #define BIGMODEL_SYSTEM_PROMPT                                                      "你是一个AI助手，运行在智能设备上。请根据用户问题选择适当的工具调用。"

        #define BIGMODEL_IMAGE_SIZE_DEFAULT                                                 "720x720"                                           /* 默认图片尺寸 */

        #define BIGMODEL_IMAGE_QUALITY_DEFAULT                                              "standard"                                          /* 默认图片质量 */

        #define BIGMODEL_IMAGE_N_DEFAULT                                                    (1)                                                 /* 默认生成数量 */

        public:
            /*函数定义结构*/
            typedef struct {
                const char* name;         /*函数名称*/
                const char* description;  /*函数描述*/
                const char* parameters;   /*JSON格式的参数定义*/
            } FunctionDef;

            /* 请求类型枚举 */
            enum RequestType {
                REQUEST_TYPE_CHAT,        /* 聊天请求 */
                REQUEST_TYPE_IMAGE        /* 图像生成请求*/
            };

            /*定义请求消息结构*/
            typedef struct {
                RequestType type;         /* 请求类型 */
                char *prompt;             /*用户输入的问题*/
                void (*callback)(char *, void *); /*响应处理回调函数*/
                void* user_data;          /*用户自定义数据*/
                FunctionDef *functions;   /*函数定义数组*/
                size_t function_count;    /*函数数量*/
                const char *tool_choice;  /*工具调用策略 ("auto"|"none"|函数名)*/
                bool is_function_response; /*标记是否为函数响应*/
                const char* tool_call_id;  /*工具调用ID（函数响应时使用）*/
                const char* function_name; /*函数名称（函数响应时使用）*/
                const char* function_result; /*函数执行结果（函数响应时使用）*/
                const char* assistant_message;  /*保存助理的完整消息*/
                const char* original_user_message; /*原始用户消息*/
                /*图像生成专用字段*/
                const char* image_size;    /* 图片尺寸 */
                const char* image_quality; /* 图片质量 */
                int image_n;               /* 生成图片数量 */
            } api_request_t;

            /*定义响应消息结构*/
            typedef struct {
                RequestType type;                   /* 响应类型 */
                char *response;                     /*API返回的完整响应*/
                void (*callback)(char *, void *);   /*对应的回调函数*/
                void *user_data;                    /*用户自定义数据*/  
            } api_response_t;

            /*线程安全的环形缓冲区*/
            typedef struct {
                char* buffer;
                size_t head;
                size_t tail;
                size_t capacity;
                size_t size;
                SemaphoreHandle_t mutex;
            } RingBuffer;

            /*获取单例实例的静态方法*/
            inline static BigModel& getInstance() {
                static BigModel instance;
                return instance;
            }

            void init();

            void reset();

            /* 文本聊天请求 */
            void request(const char *prompt, void (*callback)(char *, void *), void* user_data, TickType_t xTicksToWait = portMAX_DELAY,
                         FunctionDef *functions = nullptr, size_t function_count = 0,
                         const char *tool_choice = "auto");

            /* 图像生成请求 */
            void requestImg(const char *prompt, 
                          void (*callback)(char *, void *), 
                          void* user_data, 
                          const char *size = BIGMODEL_IMAGE_SIZE_DEFAULT,
                          const char *quality = BIGMODEL_IMAGE_QUALITY_DEFAULT,
                          int n = BIGMODEL_IMAGE_N_DEFAULT,
                          TickType_t xTicksToWait = portMAX_DELAY);
            
            /* 函数响应 */
            void functionResponse(const char *tool_call_id, const char *function_name, 
                                 const char *function_result, const char* assistant_message,
                                 const char* original_user_message,
                                 void (*callback)(char *, void *), 
                                 void* user_data, TickType_t xTicksToWait = portMAX_DELAY);

        private:
            const char* TAG = "BigModel";
            QueueHandle_t request_queue;
            QueueHandle_t response_queue;
            RingBuffer response_buffer;
            esp_http_client_handle_t persistent_client;
            char* persistent_jwt;
            time_t jwt_expiry;
            TaskHandle_t RequestTask_handle; 
            TaskHandle_t ResponseTask_handle; 
            volatile bool stop_tasks;
            SemaphoreHandle_t task_stop_sem;
            void init_ring_buffer(size_t capacity);
            bool ring_buffer_put(const char* data, size_t len);
            size_t ring_buffer_get(char* dest, size_t max_len);
            void ring_buffer_clear();
            static bool check_connection(BigModel* bm);
            static esp_err_t http_event_handler(esp_http_client_event_t *evt);
            static esp_http_client_handle_t create_persistent_client(void* user_data);
            static void base64_url_encode(const uint8_t *data, size_t input_length, char *output, size_t output_size);
            static void hmac_sha256(const char *key, const char *data, uint8_t *output);
            static char* generate_jwt_token();
            static void update_jwt_token(BigModel* bm);
            static cJSON* prepare_request(const char* TAG, 
                                         const char *prompt,
                                         FunctionDef *functions,
                                         size_t function_count,
                                         const char *tool_choice,
                                         bool is_function_response,
                                         const char* tool_call_id,
                                         const char* function_name,
                                         const char* function_result,
                                         const char* assistant_message,
                                         const char* original_user_message);
            static cJSON* prepare_image_request(const char* TAG, 
                                              const char *prompt,
                                              const char *size,
                                              const char *quality,
                                              int n);                             
            static void set_http_headers(BigModel* bm);
            static void log_full_request(BigModel* bm, char* post_data);
            static void RequestTask(void *pvParam);
            static void ResponseTask(void *pvParam);
            void safe_stop_tasks();
            /*私有构造函数，禁止外部直接实例化*/
            BigModel();
            ~BigModel();
            /*禁止拷贝构造和赋值操作*/
            BigModel(const BigModel&) = delete;
            BigModel& operator = (const BigModel&) = delete;
    };

}


