/**
 * @file ArtificialIntelligence.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include "fml.hpp"

namespace bll{

    class ArtificialIntelligence
    {
        typedef void (*ResponseCallBack_t)(void* user_data, char* answer);  

        private:
            const char* TAG = "ArtificialIntelligence";
            fml::BigModel::FunctionDef FUNCTIONS[2] = {
                {
                    "adjust_volume",
                    "由设备本地执行音量调整操作。请根据用户请求生成音量级别参数。",
                    R"JSON({
                        "type": "object",
                        "properties": {
                            "volume_level": {
                                "type": "integer",
                                "description": "音量级别(0-100)",
                                "minimum": 0,
                                "maximum": 100
                            }
                        },
                        "required": ["volume_level"]
                    })JSON"
                },
                {
                    "query_weather",
                    "查询指定城市的天气信息，包括温度、天气状况和空气质量等。",
                    R"JSON({
                        "type": "object",
                        "properties": {
                            "city": {
                                "type": "string",
                                "description": "要查询天气的城市名称"
                            }
                        },
                        "required": ["city"]
                    })JSON"
                }
            };
            const size_t FUNCTION_COUNT = sizeof(FUNCTIONS) / sizeof(FUNCTIONS[0]);
            ResponseCallBack_t response_callback;
            void* response_user_data;
            /*私有构造函数，禁止外部直接实例化*/
            ArtificialIntelligence();
            ~ArtificialIntelligence();
            /*禁止拷贝构造和赋值操作*/
            ArtificialIntelligence(const ArtificialIntelligence&) = delete;
            ArtificialIntelligence& operator = (const ArtificialIntelligence&) = delete;
            static std::string url_encode(const std::string& str);
            static esp_err_t http_event_handler(esp_http_client_event_t *evt);
            static char* perform_query_weather(char* city);
            static char* perform_adjust_volume(int volume_level);
            static void ai_response_handler(char* response, void* user_data);
        public:
            /*获取单例实例的静态方法*/
            inline static ArtificialIntelligence& getInstance() {
                static ArtificialIntelligence instance;
                return instance;
            }
            void init();
            void reset();
            void ask_question(const char* question, ResponseCallBack_t cb, void* user_data);
            void ask_img(const char* desc, ResponseCallBack_t cb, void* user_data);
    };

}