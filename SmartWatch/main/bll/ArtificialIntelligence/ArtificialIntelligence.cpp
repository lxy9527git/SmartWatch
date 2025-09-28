/**
 * @file ArtificialIntelligence.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "ArtificialIntelligence.hpp"
#include <cJSON.h>

namespace bll{
    ArtificialIntelligence::ArtificialIntelligence()
    {
        response_callback = NULL;
        response_user_data = NULL;
    }

    ArtificialIntelligence::~ArtificialIntelligence()
    {

    }

    std::string ArtificialIntelligence::url_encode(const std::string& str) {
        const char* hex = "0123456789ABCDEF";
        std::string encoded;
        
        for (char c : str) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += '+';  /*空格替换为+*/
            } else {
                encoded += '%';
                encoded += hex[(c >> 4) & 0xF];
                encoded += hex[c & 0xF];
            }
        }
        return encoded;
    }

    esp_err_t ArtificialIntelligence::http_event_handler(esp_http_client_event_t *evt) {
        switch (evt->event_id) {
            case HTTP_EVENT_ON_DATA:
                /*处理分块数据*/
                if (evt->user_data) {
                    /*将数据追加到用户提供的缓冲区*/
                    std::string* buffer = static_cast<std::string*>(evt->user_data);
                    buffer->append(static_cast<const char*>(evt->data), evt->data_len);
                }
                break;
                
            case HTTP_EVENT_ON_FINISH:
                /*请求完成*/
                ESP_LOGD("ai", "HTTP请求完成");
                break;
                
            case HTTP_EVENT_ERROR:
                ESP_LOGE("ai", "HTTP请求发生错误");
                break;
                
            default:
                break;
        }
        return ESP_OK;
    }

    char* ArtificialIntelligence::perform_query_weather(char* city) {
        /*事件处理器的静态缓冲区*/
        static std::string http_response_data;
        /* 构建API请求URL */
        std::string encoded_city = url_encode(city);
        std::string url = "https://api.asilu.com/weather/?city=" + encoded_city;
        
        /* 创建HTTP客户端配置 */
        esp_http_client_config_t config = {
            .url = url.c_str(),
            .timeout_ms = 15000,                        /*15秒超时*/
            .disable_auto_redirect = false,             /*允许重定向*/
            .event_handler = http_event_handler,        /*事件处理器*/
            .user_data = &http_response_data,           /*传递缓冲区指针*/
        };
        
        /*清空响应缓冲区*/
        http_response_data.clear();
        
        /*初始化客户端*/
        esp_http_client_handle_t client = esp_http_client_init(&config);
        
        /*设置必要的HTTP头*/
        esp_http_client_set_header(client, "User-Agent", "ESP32-HTTP-Client");
        esp_http_client_set_header(client, "Accept", "application/json");
        esp_http_client_set_header(client, "Connection", "close");

        /* 执行HTTP请求 */
        esp_err_t err = esp_http_client_perform(client);
        
        if (err != ESP_OK) {
            esp_http_client_cleanup(client);
            return strdup("无法连接到天气服务器");
        }

        /* 获取HTTP状态码 */
        int status_code = esp_http_client_get_status_code(client);
        
        /*检查是否为成功响应 (200-299)*/
        if (status_code < 200 || status_code >= 300) {
            esp_http_client_cleanup(client);
            char error_msg[64];
            snprintf(error_msg, sizeof(error_msg), "天气服务返回错误: HTTP %d", status_code);
            return strdup(error_msg);
        }

        /*清理HTTP客户端*/
        esp_http_client_cleanup(client);

        /* 检查是否收到有效响应 */
        if (http_response_data.empty()) {
            return strdup("天气服务返回空响应");
        }

        /* 解析JSON响应 */
        cJSON* json = cJSON_Parse(http_response_data.c_str());
        if (!json) {
            return strdup("解析天气响应时出错");
        }
        
        /* 提取城市名称 */
        cJSON* city_item = cJSON_GetObjectItem(json, "city");
        if (!city_item || !cJSON_IsString(city_item)) {
            cJSON_Delete(json);
            return strdup("天气响应格式错误: 缺少城市信息");
        }
        
        /* 提取PM2.5信息 */
        cJSON* pm25_item = cJSON_GetObjectItem(json, "pm25");
        std::string pm25 = (pm25_item && cJSON_IsString(pm25_item)) ? pm25_item->valuestring : "未知";
        
        /* 提取天气数组 */
        cJSON* weather_array = cJSON_GetObjectItem(json, "weather");
        if (!weather_array || !cJSON_IsArray(weather_array)) {
            cJSON_Delete(json);
            return strdup("天气响应格式错误: 缺少天气数据");
        }
        
        /* 获取今天天气信息 */
        cJSON* today_weather = cJSON_GetArrayItem(weather_array, 0);
        if (!today_weather) {
            cJSON_Delete(json);
            return strdup("天气响应格式错误: 缺少今天天气数据");
        }
        
        /* 提取今天天气详情 */
        cJSON* weather_item = cJSON_GetObjectItem(today_weather, "weather");
        cJSON* temp_item = cJSON_GetObjectItem(today_weather, "temp");
        cJSON* wind_item = cJSON_GetObjectItem(today_weather, "wind");
        
        if (!weather_item || !temp_item || !wind_item || 
            !cJSON_IsString(weather_item) || !cJSON_IsString(temp_item) || !cJSON_IsString(wind_item)) {
            cJSON_Delete(json);
            return strdup("天气响应格式错误: 天气数据不完整");
        }
        
        /* 构建响应字符串 */
        std::string temp_str = temp_item->valuestring;
        /*移除可能存在的℃符号并添加"摄氏度"*/
        size_t pos = temp_str.find("℃");
        if (pos != std::string::npos) {
            temp_str.replace(pos, 3, "摄氏度"); /*"℃"在UTF-8中占3个字节*/
        } else {
            /*如果温度字符串中没有℃符号，直接添加"摄氏度"*/
            temp_str += "摄氏度";
        }

        std::string response = city_item->valuestring;
        response += "今天天气: " + std::string(weather_item->valuestring) + 
                ", 温度: " + temp_str +
                ", 风力: " + std::string(wind_item->valuestring) +
                ", PM2.5: " + pm25;
        
        cJSON_Delete(json);

        ESP_LOGI("perform_query_weather", "%s, %s\n", city, response.c_str());

        return strdup(response.c_str());
    }

    char* ArtificialIntelligence::perform_adjust_volume(int volume_level)
    {
        fml::HdlManager::getInstance().set_audio_volume(volume_level);
        char* response = (char*)malloc(100);
        snprintf(response, 100, "音量已调整至%d%%", volume_level);
        return response;
    }

    void ArtificialIntelligence::ai_response_handler(char* response, void* user_data)
    {
        if (!response) return;
        
        cJSON *json = cJSON_Parse(response);
        if (json) {
            cJSON *type = cJSON_GetObjectItem(json, "type");
            if (type && strcmp(type->valuestring, "TOOL_CALL") == 0) {
                cJSON *id = cJSON_GetObjectItem(json, "id");
                cJSON *func_name = cJSON_GetObjectItem(json, "function");
                cJSON *args = cJSON_GetObjectItem(json, "arguments");
                cJSON *assistant_msg = cJSON_GetObjectItem(json, "assistant_message");
                cJSON *user_msg = cJSON_GetObjectItem(json, "user_message");
                
                if (id && func_name && args && assistant_msg && user_msg) {
                    if (strcmp(func_name->valuestring, "adjust_volume") == 0) {
                        int volume_level = 0;
                        cJSON *args_json = cJSON_Parse(args->valuestring);
                        if (args_json) {
                            cJSON *vol_item = cJSON_GetObjectItem(args_json, "volume_level");
                            if (cJSON_IsNumber(vol_item)) {
                                volume_level = vol_item->valueint;
                            }
                            cJSON_Delete(args_json);
                        }
                        
                        char* result = perform_adjust_volume(volume_level);
                        
                        fml::BigModel::getInstance().functionResponse(
                            id->valuestring,
                            func_name->valuestring,
                            result,
                            assistant_msg->valuestring,
                            user_msg->valuestring,
                            ai_response_handler,
                            user_data,
                            portMAX_DELAY
                        );
                        free(result);
                    } else if (strcmp(func_name->valuestring, "query_weather") == 0) {
                        char* city = nullptr;
                        cJSON *args_json = cJSON_Parse(args->valuestring);
                        if (args_json) {
                            cJSON *city_item = cJSON_GetObjectItem(args_json, "city");
                            if (cJSON_IsString(city_item)) {
                                city = strdup(city_item->valuestring);
                            }
                            cJSON_Delete(args_json);
                        }
                        
                        if (city) {
                            char* result = perform_query_weather(city);
                            
                            fml::BigModel::getInstance().functionResponse(
                                id->valuestring,
                                func_name->valuestring,
                                result,
                                assistant_msg->valuestring,
                                user_msg->valuestring,
                                ai_response_handler,
                                user_data,
                                portMAX_DELAY
                            );
                            free(result);
                            free(city);
                        }
                    }
                }
            }
            cJSON_Delete(json);
        } else {
            ArtificialIntelligence* ai = (ArtificialIntelligence*)user_data;
            if(ai->response_callback != NULL){
                ai->response_callback(ai->response_user_data, response);
            }
        }
    }

    void ArtificialIntelligence::init()
    {
        
    }

    void ArtificialIntelligence::reset()
    {
        response_callback = NULL;
        response_user_data = NULL;
        fml::BigModel::getInstance().reset();
    }

    void ArtificialIntelligence::ask_question(const char* question, ResponseCallBack_t cb, void* user_data)
    {
        response_callback = cb;
        response_user_data = user_data;

        fml::BigModel::getInstance().request(
            question,
            ai_response_handler,
            this,
            portMAX_DELAY,
            FUNCTIONS,
            FUNCTION_COUNT,
            "auto"
        );
    }

    void ArtificialIntelligence::ask_img(const char* desc, ResponseCallBack_t cb, void* user_data)
    {
        response_callback = cb;
        response_user_data = user_data;

        fml::BigModel::getInstance().requestImg(
            desc, 
            ai_response_handler, 
            this
        );
    }
}