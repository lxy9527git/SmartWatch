/**
 * @file BigModel.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "BigModel.hpp"

namespace fml{

    void BigModel::init_ring_buffer(size_t capacity)
    {
        if (response_buffer.buffer != NULL) {
            free(response_buffer.buffer);
            response_buffer.buffer = NULL;
        }
        response_buffer.buffer = (char*)malloc(capacity);
        if (response_buffer.buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate ring buffer memory");
            return;
        }
        response_buffer.head = 0;
        response_buffer.tail = 0;
        response_buffer.capacity = capacity;
        response_buffer.size = 0;
        if (response_buffer.mutex != NULL) {
            vSemaphoreDelete(response_buffer.mutex);
            response_buffer.mutex = NULL;
        }
        response_buffer.mutex = xSemaphoreCreateMutex();
        if (response_buffer.mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create ring buffer mutex");
        }
    }
    /*向环形缓冲区添加数据（非阻塞）*/
    bool BigModel::ring_buffer_put(const char* data, size_t len)
    {
        if(xSemaphoreTake(response_buffer.mutex, pdMS_TO_TICKS(10))){
            /*检查是否有足够空间*/
            if (len > (response_buffer.capacity - response_buffer.size)) {
                xSemaphoreGive(response_buffer.mutex);
                return false;
            }
            
            /*添加数据*/
            for (size_t i = 0; i < len; i++) {
                response_buffer.buffer[response_buffer.head] = data[i];
                response_buffer.head = (response_buffer.head + 1) % response_buffer.capacity;
                response_buffer.size++;
            }
            
            xSemaphoreGive(response_buffer.mutex);
            return true;
        }
        return false;
    }
    /*从环形缓冲区获取数据（非阻塞）*/
    size_t BigModel::ring_buffer_get(char* dest, size_t max_len)
    {
        size_t read = 0;
        if(xSemaphoreTake(response_buffer.mutex, pdMS_TO_TICKS(10))){
            while (response_buffer.size > 0 && read < max_len) {
                dest[read] = response_buffer.buffer[response_buffer.tail];
                response_buffer.tail = (response_buffer.tail + 1) % response_buffer.capacity;
                response_buffer.size--;
                read++;
            }
            xSemaphoreGive(response_buffer.mutex);
        }
        return read;
    }
    /*清空环形缓冲区*/
    void BigModel::ring_buffer_clear()
    {
        if(xSemaphoreTake(response_buffer.mutex, pdMS_TO_TICKS(10))){
            response_buffer.head = 0;
            response_buffer.tail = 0;
            response_buffer.size = 0;
            xSemaphoreGive(response_buffer.mutex);
        }
    }

    bool BigModel::check_connection(BigModel* bm)
    {
        /*检查连接状态并重建*/
        if(bm->persistent_client == NULL) {
            ESP_LOGI(bm->TAG, "Creating new HTTP client");
            bm->persistent_client = create_persistent_client(bm);
            return (bm->persistent_client != NULL);
        }
        /*方法1：检查传输类型*/
        esp_http_client_transport_t transport = esp_http_client_get_transport_type(bm->persistent_client);
        if (transport == HTTP_TRANSPORT_UNKNOWN) {
            ESP_LOGW(bm->TAG, "Invalid transport type, recreating client");
            esp_http_client_cleanup(bm->persistent_client);
            bm->persistent_client = create_persistent_client(bm);
            return (bm->persistent_client != NULL);
        }
        /*方法2：检查最近错误状态*/
        int errno_val = esp_http_client_get_errno(bm->persistent_client);
        if (errno_val != 0) {
            ESP_LOGW(bm->TAG, "Connection error detected (errno=%d), recreating", errno_val);
            esp_http_client_cleanup(bm->persistent_client);
            bm->persistent_client = create_persistent_client(bm);
            return (bm->persistent_client != NULL);
        }
        
        return true;
    }

    esp_err_t BigModel::http_event_handler(esp_http_client_event_t *evt)
    {
        BigModel* bm = (BigModel*)evt->user_data;
        switch(evt->event_id) {
            case HTTP_EVENT_ON_DATA: {
                if (!evt->data || evt->data_len <= 0) break;
                
                /*非阻塞方式添加数据到环形缓冲区*/
                if(!bm->ring_buffer_put((const char*)evt->data, evt->data_len)){
                    ESP_LOGW(bm->TAG, "Ring buffer full, data dropped");
                }
                break;
            }
            
            case HTTP_EVENT_ON_FINISH: {
                /*通知请求任务数据已完整接收*/
                break;
            }
            
            case HTTP_EVENT_ERROR: {
                ESP_LOGE(bm->TAG, "HTTP_EVENT_ERROR");
                /*标记连接不可用*/
                if(bm->persistent_client) {
                    esp_http_client_close(bm->persistent_client);
                }
                break;
            }

            case HTTP_EVENT_DISCONNECTED: {
                ESP_LOGW(bm->TAG, "HTTP_EVENT_DISCONNECTED");
                break;
            }
            
            default:
                break;
        }
        return ESP_OK;
    }

    esp_http_client_handle_t BigModel::create_persistent_client(void* user_data)
    {
        esp_http_client_config_t config = {
            .url = BIGMODEL_HTTPS_URL,              /*默认URL，实际使用时根据请求类型会修改*/
            .method = HTTP_METHOD_POST,
            .timeout_ms = 30000,                    /*30秒超时*/
            .disable_auto_redirect = false,         /*允许自动重定向*/ 
            .event_handler = http_event_handler,    /*添加事件处理器*/
            .user_data = user_data,                 /*传递类实例指针*/
            .skip_cert_common_name_check = true,    /*跳过证书通用名检查*/
            .keep_alive_enable = true,              /*启用长连接*/
            .keep_alive_idle = 30,                  /*空闲30秒*/
            .keep_alive_interval = 10,              /*每10秒发送保活包*/
            .keep_alive_count = 8,                  /*最多8次重试*/
        };

        return esp_http_client_init(&config);
    }
    /*ase64 URL安全编码*/
    void BigModel::base64_url_encode(const uint8_t *data, size_t input_length, char *output, size_t output_size)
    {
        size_t output_length = 0;
        mbedtls_base64_encode((unsigned char *)output, output_size, &output_length, data, input_length);
        /*转换为URL安全格式*/
        for (size_t i = 0; i < output_length; i++) {
            if (output[i] == '+') output[i] = '-';
            if (output[i] == '/') output[i] = '_';
            if (output[i] == '=') { /*移除填充*/
                output[i] = '\0';
                break;
            }
        }
    }
    /*HMAC-SHA256签名*/
    void BigModel::hmac_sha256(const char *key, const char *data, uint8_t *output)
    {
        mbedtls_sha256_context ctx;
        uint8_t k_ipad[64] = {0};
        uint8_t k_opad[64] = {0};
        uint8_t tmp_hash[32];
        
        size_t key_len = strlen(key);
        size_t data_len = strlen(data);

        /*密钥处理*/
        if (key_len > 64) {
            mbedtls_sha256((const unsigned char *)key, key_len, tmp_hash, 0);
            memcpy(k_ipad, tmp_hash, 32);
            memcpy(k_opad, tmp_hash, 32);
        } else {
            memcpy(k_ipad, key, key_len);
            memcpy(k_opad, key, key_len);
        }

        /*填充*/
        for (int i = 0; i < 64; i++) {
            k_ipad[i] ^= 0x36;
            k_opad[i] ^= 0x5c;
        }

        /*计算内层哈希*/
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0);
        mbedtls_sha256_update(&ctx, k_ipad, 64);
        mbedtls_sha256_update(&ctx, (const unsigned char *)data, data_len);
        mbedtls_sha256_finish(&ctx, tmp_hash);
        mbedtls_sha256_free(&ctx);
        
        /*计算外层哈希*/
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts(&ctx, 0);
        mbedtls_sha256_update(&ctx, k_opad, 64);
        mbedtls_sha256_update(&ctx, tmp_hash, 32);
        mbedtls_sha256_finish(&ctx, output);
        mbedtls_sha256_free(&ctx);
    }
    /*生成JWT令牌*/
    char* BigModel::generate_jwt_token()
    {
        /*获取当前时间戳（秒）*/
        time_t now;
        time(&now);
        uint64_t timestamp_ms = (uint64_t)now * 1000;
        uint64_t exp_ms = timestamp_ms + 3600 * 1000;   /*1小时后过期*/

        /*创建Header*/
        char header[] = "{\"alg\":\"HS256\",\"sign_type\":\"SIGN\"}";
        char header_encoded[128];
        base64_url_encode((uint8_t*)header, strlen(header), header_encoded, sizeof(header_encoded));

        /*创建Payload*/
        char payload[256];
        snprintf(payload, sizeof(payload), 
                "{\"api_key\":\"%s\",\"exp\":%" PRIu64 ",\"timestamp\":%" PRIu64 "}",
                BIGMODEL_API_KEY_ID, exp_ms, timestamp_ms);
        
        char payload_encoded[256];
        base64_url_encode((uint8_t*)payload, strlen(payload), payload_encoded, sizeof(payload_encoded));

        /*组合签名数据*/
        char signing_input[384];
        snprintf(signing_input, sizeof(signing_input), "%s.%s", header_encoded, payload_encoded);

        /*计算签名*/
        uint8_t signature_bin[32];
        hmac_sha256(BIGMODEL_API_KEY_SECRET, signing_input, signature_bin);
        
        char signature_encoded[128];
        base64_url_encode(signature_bin, sizeof(signature_bin), signature_encoded, sizeof(signature_encoded));

        /*组合完整JWT*/
        char *jwt = (char*)malloc(strlen(header_encoded) + strlen(payload_encoded) + strlen(signature_encoded) + 3);
        sprintf(jwt, "%s.%s.%s", header_encoded, payload_encoded, signature_encoded);
        
        return jwt;

    }

    void BigModel::update_jwt_token(BigModel* bm)
    {
        time_t now;
        time(&now);

        /*如果令牌不存在或即将过期(提前5分钟更新)*/
        if(bm->persistent_jwt == NULL || now > (bm->jwt_expiry - 300)){
            if (bm->persistent_jwt) {
                free(bm->persistent_jwt);
                bm->persistent_jwt = NULL;
            }
            bm->persistent_jwt = generate_jwt_token();
            if (bm->persistent_jwt) {
                bm->jwt_expiry = now + 3600; // 1小时有效期
                ESP_LOGI(bm->TAG, "JWT token updated");
            } else {
                ESP_LOGE(bm->TAG, "Failed to generate JWT token");
            }
        }
    }

    cJSON* BigModel::prepare_request(const char* TAG, 
                                    const char *prompt,
                                    FunctionDef *functions,
                                    size_t function_count,
                                    const char *tool_choice,
                                    bool is_function_response,
                                    const char* tool_call_id,
                                    const char* function_name,
                                    const char* function_result,
                                    const char* assistant_message,
                                    const char* original_user_message)
    {
        cJSON *request = NULL;
        cJSON *messages = NULL;
        cJSON *system_message = NULL;
        cJSON *user_message = NULL;
        cJSON *tool_message = NULL;
        cJSON *tools = NULL;
        cJSON *tool_choice_obj = NULL;
        cJSON *function_obj = NULL;
        
        /*创建主对象*/
        request = cJSON_CreateObject();
        if (!request) {
            ESP_LOGE(TAG, "Failed to create request object");
            goto cleanup;
        }
        
        /*添加模型*/
        if (!cJSON_AddStringToObject(request, "model", BIGMODEL_MODEL_NAME)) {
            ESP_LOGE(TAG, "Failed to add model");
            goto cleanup;
        }
        
        /*添加参数*/
        if (!cJSON_AddNumberToObject(request, "max_tokens", BIGMODEL_MAX_TOKENS) ||
            !cJSON_AddNumberToObject(request, "temperature", BIGMODEL_TEMPERATURE) ||
            !cJSON_AddNumberToObject(request, "top_p", BIGMODEL_TOP_P)) {
            ESP_LOGE(TAG, "Failed to add parameters");
            goto cleanup;
        }
        
        /*创建消息数组*/
        messages = cJSON_AddArrayToObject(request, "messages");
        if (!messages) {
            ESP_LOGE(TAG, "Failed to create messages array");
            goto cleanup;
        }
        
        /*创建系统消息*/
        system_message = cJSON_CreateObject();
        if (!system_message) {
            ESP_LOGE(TAG, "Failed to create system message");
            goto cleanup;
        }
        
        /*填充系统消息*/
        if (!cJSON_AddStringToObject(system_message, "role", "system") ||
            !cJSON_AddStringToObject(system_message, "content", BIGMODEL_SYSTEM_PROMPT)) {
            ESP_LOGE(TAG, "Failed to add system message fields");
            goto cleanup;
        }
        
        /*添加到消息数组*/
        cJSON_AddItemToArray(messages, system_message);
        system_message = NULL; // 已添加到父对象，不需要单独释放
        
        /*函数响应时需要完整上下文消息链*/
        if (is_function_response) {
            /*1. 添加原始用户消息*/
            user_message = cJSON_CreateObject();
            if (!user_message) {
                ESP_LOGE(TAG, "Failed to create user message");
                goto cleanup;
            }
            
            if (!cJSON_AddStringToObject(user_message, "role", "user") ||
                !cJSON_AddStringToObject(user_message, "content", original_user_message)) {
                ESP_LOGE(TAG, "Failed to add user message fields");
                goto cleanup;
            }
            
            cJSON_AddItemToArray(messages, user_message);
            user_message = NULL;
            
            /*2. 添加助理消息（包含工具调用）*/
            cJSON *assistant_msg_json = cJSON_Parse(assistant_message);
            if (assistant_msg_json) {
                cJSON_AddItemToArray(messages, assistant_msg_json);
            } else {
                ESP_LOGE(TAG, "Failed to parse assistant message, using fallback");
                // 创建安全的回退消息
                assistant_msg_json = cJSON_CreateObject();
                cJSON_AddStringToObject(assistant_msg_json, "role", "assistant");
                cJSON_AddStringToObject(assistant_msg_json, "content", "[Invalid assistant message]");
                cJSON_AddItemToArray(messages, assistant_msg_json);
            }
            
            /*3. 添加工具执行结果消息*/
            tool_message = cJSON_CreateObject();
            if (!tool_message) {
                ESP_LOGE(TAG, "Failed to create tool message");
                goto cleanup;
            }
            
            if (!cJSON_AddStringToObject(tool_message, "role", "tool") ||
                !cJSON_AddStringToObject(tool_message, "tool_call_id", tool_call_id) ||
                !cJSON_AddStringToObject(tool_message, "content", function_result)) {
                ESP_LOGE(TAG, "Failed to add tool message fields");
                goto cleanup;
            }
            
            cJSON_AddItemToArray(messages, tool_message);
            tool_message = NULL;
        } 
        /*普通用户请求*/
        else {
            /*创建用户消息*/
            user_message = cJSON_CreateObject();
            if (!user_message) {
                ESP_LOGE(TAG, "Failed to create user message");
                goto cleanup;
            }
            
            /*填充用户消息*/
            if (!cJSON_AddStringToObject(user_message, "role", "user") ||
                !cJSON_AddStringToObject(user_message, "content", prompt)) {
                ESP_LOGE(TAG, "Failed to add user message fields");
                goto cleanup;
            }
            
            /*添加到消息数组*/
            cJSON_AddItemToArray(messages, user_message);
            user_message = NULL; /*已添加到父对象，不需要单独释放*/
        }

        /*添加工具定义（普通请求且有函数时）*/
        if (!is_function_response && function_count > 0) {
            tools = cJSON_AddArrayToObject(request, "tools");
            if (!tools) {
                ESP_LOGE(TAG, "Failed to create tools array");
                goto cleanup;
            }
            
            for (size_t i = 0; i < function_count; i++) {
                cJSON *tool = cJSON_CreateObject();
                if (!tool) {
                    ESP_LOGE(TAG, "Failed to create tool object");
                    goto cleanup;
                }
                
                /*添加工具类型为function*/
                if (!cJSON_AddStringToObject(tool, "type", "function")) {
                    ESP_LOGE(TAG, "Failed to add tool type");
                    cJSON_Delete(tool);
                    goto cleanup;
                }
                
                cJSON *function = cJSON_CreateObject();
                if (!function) {
                    ESP_LOGE(TAG, "Failed to create function object");
                    cJSON_Delete(tool);
                    goto cleanup;
                }
                
                if (!cJSON_AddStringToObject(function, "name", functions[i].name) ||
                    !cJSON_AddStringToObject(function, "description", functions[i].description)) {
                    ESP_LOGE(TAG, "Failed to add function fields");
                    cJSON_Delete(function);
                    cJSON_Delete(tool);
                    goto cleanup;
                }
                
                /*解析参数JSON*/
                cJSON *parameters = cJSON_Parse(functions[i].parameters);
                if (!parameters) {
                    ESP_LOGE(TAG, "Invalid parameters JSON for function: %s", functions[i].name);
                    cJSON_Delete(function);
                    cJSON_Delete(tool);
                    goto cleanup;
                }
                
                cJSON_AddItemToObject(function, "parameters", parameters);
                cJSON_AddItemToObject(tool, "function", function);
                cJSON_AddItemToArray(tools, tool);
            }
        }

        /*添加工具选择策略*/
        if (!is_function_response && tool_choice) {
            if (strcmp(tool_choice, "auto") == 0 || strcmp(tool_choice, "none") == 0) {
                if (!cJSON_AddStringToObject(request, "tool_choice", tool_choice)) {
                    ESP_LOGE(TAG, "Failed to add tool_choice string");
                    goto cleanup;
                }
            } else {
                /*指定具体函数*/
                tool_choice_obj = cJSON_CreateObject();
                if (!tool_choice_obj) {
                    ESP_LOGE(TAG, "Failed to create tool_choice object");
                    goto cleanup;
                }
                
                if (!cJSON_AddStringToObject(tool_choice_obj, "type", "function")) {
                    ESP_LOGE(TAG, "Failed to add tool_choice type");
                    goto cleanup;
                }
                
                function_obj = cJSON_CreateObject();
                if (!function_obj) {
                    ESP_LOGE(TAG, "Failed to create function object for tool_choice");
                    goto cleanup;
                }
                
                if (!cJSON_AddStringToObject(function_obj, "name", tool_choice)) {
                    ESP_LOGE(TAG, "Failed to add function name for tool_choice");
                    goto cleanup;
                }
                
                cJSON_AddItemToObject(tool_choice_obj, "function", function_obj);
                cJSON_AddItemToObject(request, "tool_choice", tool_choice_obj);
                function_obj = NULL; /*所有权已转移*/
                tool_choice_obj = NULL; /*所有权已转移*/
            }
        }

        return request;

    cleanup:
        /*清理所有分配的资源*/
        if (request) cJSON_Delete(request);
        if (system_message) cJSON_Delete(system_message);
        if (user_message) cJSON_Delete(user_message);
        if (tool_message) cJSON_Delete(tool_message);
        if (tools) cJSON_Delete(tools);
        if (tool_choice_obj) cJSON_Delete(tool_choice_obj);
        if (function_obj) cJSON_Delete(function_obj);
        
        return NULL;
    }

    cJSON* BigModel::prepare_image_request(const char* TAG, 
                                         const char *prompt,
                                         const char *size,
                                         const char *quality,
                                         int n) {
        cJSON *request = cJSON_CreateObject();
        if (!request) {
            ESP_LOGE(TAG, "Failed to create image request object");
            return NULL;
        }
        
        /*添加模型名称*/
        if (!cJSON_AddStringToObject(request, "model", BIGMODEL_IMAGE_MODEL_NAME)) {
            ESP_LOGE(TAG, "Failed to add model to image request");
            cJSON_Delete(request);
            return NULL;
        }
        
        /*添加提示词*/
        if (!cJSON_AddStringToObject(request, "prompt", prompt)) {
            ESP_LOGE(TAG, "Failed to add prompt to image request");
            cJSON_Delete(request);
            return NULL;
        }
        
        /*添加尺寸（可选）*/
        if (size && strlen(size) > 0) {
            if (!cJSON_AddStringToObject(request, "size", size)) {
                ESP_LOGE(TAG, "Failed to add size to image request");
                cJSON_Delete(request);
                return NULL;
            }
        }
        
        /*添加质量（可选）*/
        if (quality && strlen(quality) > 0) {
            if (!cJSON_AddStringToObject(request, "quality", quality)) {
                ESP_LOGE(TAG, "Failed to add quality to image request");
                cJSON_Delete(request);
                return NULL;
            }
        }
        
        /*添加生成数量（可选）*/
        if (n > 0) {
            if (!cJSON_AddNumberToObject(request, "n", n)) {
                ESP_LOGE(TAG, "Failed to add n to image request");
                cJSON_Delete(request);
                return NULL;
            }
        }
        
        return request;
    }

    void BigModel::set_http_headers(BigModel* bm)
    {
        /*设置认证头*/
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "%s %s", BIGMODEL_AUTH_SCHEME, bm->persistent_jwt);
        esp_http_client_set_header(bm->persistent_client, "Authorization", auth_header);
        /*设置内容类型*/
        esp_http_client_set_header(bm->persistent_client, "Content-Type", "application/json");
        /*设置接受类型*/
        esp_http_client_set_header(bm->persistent_client, "Accept", "application/json");
        /*设置缓存控制*/
        esp_http_client_set_header(bm->persistent_client, "Cache-Control", "no-cache");
        /*设置用户代理（可选）*/
        esp_http_client_set_header(bm->persistent_client, "User-Agent", "ESP32-Zhipu-Client/1.0");
    }

    void BigModel::log_full_request(BigModel* bm, char* post_data)
    {
        /*1. 获取并打印 URL*/
        char url_buf[256];
        esp_err_t url_err = esp_http_client_get_url(bm->persistent_client, url_buf, sizeof(url_buf));
        if (url_err != ESP_OK) {
            ESP_LOGE(bm->TAG, "Failed to get URL: %s", esp_err_to_name(url_err));
            return;
        }
        
        ESP_LOGI(bm->TAG, "===== [HTTP Request Start] =====");
        ESP_LOGI(bm->TAG, "> URL: %s", url_buf);
        
        /*2. 打印请求方法（硬编码为 POST，因为 BigModel 只使用 POST）*/
        ESP_LOGI(bm->TAG, "> Method: POST");
        
        /*3. 打印请求头（手动记录已知头）*/
        ESP_LOGI(bm->TAG, "> Headers:");
        
        /*手动获取并打印已知头*/
        char *auth_header = NULL;
        esp_http_client_get_header(bm->persistent_client, "Authorization", &auth_header);
        if (auth_header) {
            ESP_LOGI(bm->TAG, "> Authorization: %s", auth_header);
        }
        
        char *content_type = NULL;
        esp_http_client_get_header(bm->persistent_client, "Content-Type", &content_type);
        if (content_type) {
            ESP_LOGI(bm->TAG, "> Content-Type: %s", content_type);
        }
        
        char *accept = NULL;
        esp_http_client_get_header(bm->persistent_client, "Accept", &accept);
        if (accept) {
            ESP_LOGI(bm->TAG, "> Accept: %s", accept);
        }
        
        char *user_agent = NULL;
        esp_http_client_get_header(bm->persistent_client, "User-Agent", &user_agent);
        if (user_agent) {
            ESP_LOGI(bm->TAG, "> User-Agent: %s", user_agent);
        }
        
        /*4. 打印请求体*/
        if (post_data && strlen(post_data) > 0) {
            size_t len = strlen(post_data);
            ESP_LOGI(bm->TAG, "> Body (%d bytes):", len);
            
            /*完整打印文本内容*/
            ESP_LOGI(bm->TAG, "> %.*s", (512) > (len) ? (len) : (512), post_data);
            
            /*打印十六进制转储*/
            ESP_LOGI(bm->TAG, "> Hex dump:");
            for (size_t i = 0; i < len; i += 16) {
                char hex_line[80] = {0};
                char ascii_line[20] = {0};
                char *hex_ptr = hex_line;
                char *ascii_ptr = ascii_line;
                
                /*地址*/
                hex_ptr += snprintf(hex_ptr, sizeof(hex_line), "> %04x: ", i);
                
                /*十六进制部分*/
                for (int j = 0; j < 16; j++) {
                    if (i + j < len) {
                        hex_ptr += snprintf(hex_ptr, sizeof(hex_line) - (hex_ptr - hex_line), 
                                        "%02x ", (uint8_t)post_data[i+j]);
                        
                        *ascii_ptr++ = isprint(post_data[i+j]) ? post_data[i+j] : '.';
                    } else {
                        hex_ptr += snprintf(hex_ptr, sizeof(hex_line) - (hex_ptr - hex_line), "   ");
                        *ascii_ptr++ = ' ';
                    }
                    
                    if (j == 7) {
                        hex_ptr += snprintf(hex_ptr, sizeof(hex_line) - (hex_ptr - hex_line), " ");
                    }
                }
                
                *ascii_ptr = '\0';
                ESP_LOGI(bm->TAG, "%s  %s", hex_line, ascii_line);
            }
        }

        // ESP_LOGI(bm->TAG, "Raw post_data len:%d", strlen(post_data));
        // ESP_LOG_BUFFER_HEXDUMP(bm->TAG, post_data, strlen(post_data), ESP_LOG_INFO);
        
        /*5. 获取并打印传输类型*/
        esp_http_client_transport_t transport = esp_http_client_get_transport_type(bm->persistent_client);
        const char *transport_str = "UNKNOWN";
        switch (transport) {
            case HTTP_TRANSPORT_OVER_TCP: transport_str = "HTTP"; break;
            case HTTP_TRANSPORT_OVER_SSL: transport_str = "HTTPS"; break;
            default: break;
        }
        ESP_LOGI(bm->TAG, "> Transport: %s", transport_str);
        
        /*6. 打印时间戳*/
        time_t now = time(NULL);
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
        ESP_LOGI(bm->TAG, "> Timestamp: %s", time_str);
        
        ESP_LOGI(bm->TAG, "===== [HTTP Request End] =======");
    }

    void BigModel::RequestTask(void *pvParam)
    {
        BigModel* bm = (BigModel*)pvParam;
        api_request_t req;
        bool task_running = true;

        while(task_running){
            /*检查停止标志*/
            if (bm->stop_tasks) {
                task_running = false;
                break;
            }
            
            /*等待请求进入队列*/
            if(xQueueReceive(bm->request_queue, &req, pdMS_TO_TICKS(100)) == pdTRUE){
                cJSON *request_json = NULL;
                char *post_data = NULL;
                bool request_failed = false;
                char* target_url = BIGMODEL_HTTPS_URL;
                
                /*检查并更新JWT令牌*/
                update_jwt_token(bm);

                /*根据请求类型准备数据*/
                if (req.type == REQUEST_TYPE_CHAT) {
                    /*准备请求数据*/
                    request_json = prepare_request(
                        bm->TAG, 
                        req.prompt,
                        req.functions,
                        req.function_count,
                        req.tool_choice,
                        req.is_function_response,
                        req.tool_call_id,
                        req.function_name,
                        req.function_result,
                        req.assistant_message,
                        req.original_user_message
                    );
                } 
                else if (req.type == REQUEST_TYPE_IMAGE) {
                    request_json = prepare_image_request(
                        bm->TAG,
                        req.prompt,
                        req.image_size,
                        req.image_quality,
                        req.image_n
                    );
                    target_url = BIGMODEL_IMAGE_HTTPS_URL; /*使用图像API URL*/
                }
                else {
                    ESP_LOGE(bm->TAG, "Unknown request type: %d", req.type);
                    request_failed = true;
                }
                
                if (!request_json && !request_failed) {
                    ESP_LOGE(bm->TAG, "Failed to prepare request");
                    request_failed = true;
                } 
                else if (!request_failed) {
                    /*将JSON转换为字符串*/
                    post_data = cJSON_PrintUnformatted(request_json);
                    cJSON_Delete(request_json);
                    request_json = nullptr;
                    
                    if (!post_data) {
                        ESP_LOGE(bm->TAG, "Failed to print request JSON");
                        request_failed = true;
                    }
                }
                
                if (!request_failed) {
                    int retry_count = 0;
                    bool request_success = false;
                    
                    while(retry_count < BIGMODEL_MAX_RETRIES && !request_success && !bm->stop_tasks) {
                        /*检查并重建连接*/
                        if(!check_connection(bm)) {
                            ESP_LOGE(bm->TAG, "Connection check failed");
                            break;
                        }

                        /*设置目标URL*/
                        esp_err_t set_url_err = esp_http_client_set_url(bm->persistent_client, target_url);
                        
                        if (set_url_err != ESP_OK) {
                            ESP_LOGE(bm->TAG, "Failed to set URL: %s", esp_err_to_name(set_url_err));
                            break;
                        }

                        /*设置HTTP头*/
                        set_http_headers(bm);
                        
                        /*设置POST数据*/
                        esp_http_client_set_post_field(bm->persistent_client, post_data, strlen(post_data));

                        //log_full_request(bm, post_data);
                        
                        /*清空前一次响应的数据*/
                        bm->ring_buffer_clear();
                        
                        /*执行请求*/
                        esp_err_t err = esp_http_client_perform(bm->persistent_client);
                        
                        if (err == ESP_OK){
                            int status_code = esp_http_client_get_status_code(bm->persistent_client);
                            
                            /*处理不同状态码*/
                            switch (status_code) {
                                case 200: {
                                    /*分配内存保存响应*/
                                    char* response_data = (char*)malloc(bm->response_buffer.size + 1);
                                    if (response_data) {
                                        /*从环形缓冲区获取数据*/
                                        size_t read = bm->ring_buffer_get(response_data, bm->response_buffer.size);
                                        response_data[read] = '\0';
                                        /*创建响应结构*/
                                        api_response_t resp = {
                                            .type = req.type,
                                            .response = response_data,
                                            .callback = req.callback,
                                            .user_data = req.user_data,
                                        };
                                        /*发送到响应队列*/
                                        if (xQueueSend(bm->response_queue, &resp, 0) != pdTRUE) {
                                            ESP_LOGE(bm->TAG, "Failed to send response to queue");
                                            free(response_data);
                                        }
                                    } else {
                                        ESP_LOGE(bm->TAG, "Failed to allocate response memory");
                                    }
                                    request_success = true;
                                    break;
                                }
                                case 401: /*未授权*/
                                    free(bm->persistent_jwt);
                                    bm->persistent_jwt = NULL;
                                    update_jwt_token(bm);
                                    retry_count++;
                                    break;
                                case 429: /*限流错误*/
                                    ESP_LOGW(bm->TAG, "API限流,等待后重试");
                                    vTaskDelay(pdMS_TO_TICKS(5000)); /*等待5秒*/
                                    retry_count++;
                                    break;
                                case 503: /*服务不可用*/
                                    ESP_LOGW(bm->TAG, "服务不可用,等待后重试");
                                    vTaskDelay(pdMS_TO_TICKS(10000)); /*等待10秒*/
                                    retry_count++;
                                    break;
                                default:
                                    ESP_LOGE(bm->TAG, "API错误: HTTP %d", status_code);
                                    esp_http_client_cleanup(bm->persistent_client);
                                    bm->persistent_client = NULL;
                                    vTaskDelay(pdMS_TO_TICKS(1000));
                                    retry_count++;
                            }
                        } else {
                            ESP_LOGE(bm->TAG, "HTTP请求失败(%d/%d): %s", 
                                    retry_count+1, BIGMODEL_MAX_RETRIES, esp_err_to_name(err));
                            
                            /*网络级错误，重置连接*/
                            ESP_LOGW(bm->TAG, "Network error, resetting connection");
                            esp_http_client_cleanup(bm->persistent_client);
                            bm->persistent_client = NULL;
                            
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            retry_count++;
                        }
                    }
                    
                    if(!request_success && !bm->stop_tasks) {
                        ESP_LOGE(bm->TAG, "Request failed after %d retries", BIGMODEL_MAX_RETRIES);
                        /*发送错误回调通知*/
                        char* response_data = (char*)malloc(50);
                        if (response_data) {
                            snprintf(response_data, 50, "Request failed after %d retries", BIGMODEL_MAX_RETRIES);
                            api_response_t resp = {
                                .type = req.type,
                                .response = response_data,
                                .callback = req.callback,
                                .user_data = req.user_data,
                            };
                            if (xQueueSend(bm->response_queue, &resp, 0) != pdTRUE) {
                                free(response_data);
                            }
                        }
                    }
                }
                
                /*释放请求资源*/
                if (post_data) {
                    free(post_data);
                    post_data = NULL;
                }

                /*释放请求资源*/
                if(req.type == REQUEST_TYPE_CHAT){
                    /*释放请求结构中的资源*/
                    if(req.is_function_response){
                        if (req.tool_call_id) free((void*)req.tool_call_id);
                        if (req.function_name) free((void*)req.function_name);
                        if (req.function_result) free((void*)req.function_result);
                        if (req.assistant_message) free((void*)req.assistant_message);
                        if (req.original_user_message) free((void*)req.original_user_message);
                    }else{
                        if (req.prompt) free(req.prompt);
                        if (req.functions) free(req.functions);
                        if (req.tool_choice) free((void*)req.tool_choice);
                    }
                }
                else if (req.type == REQUEST_TYPE_IMAGE) {
                    if (req.prompt) free(req.prompt);
                    if (req.image_size) free((void*)req.image_size);
                    if (req.image_quality) free((void*)req.image_quality);
                }
            }
        }
        
        /*通知复位功能任务已停止*/
        if (bm->task_stop_sem) {
            xSemaphoreGive(bm->task_stop_sem);
        }
        
        //ESP_LOGI(bm->TAG, "RequestTask exiting");
        vTaskDelete(NULL);
    }

    void BigModel::ResponseTask(void *pvParam)
    {
        BigModel* bm = (BigModel*)pvParam;
        api_response_t resp;
        bool task_running = true;
        
        while(task_running){
            /*检查停止标志*/
            if (bm->stop_tasks) {
                task_running = false;
                break;
            }
            
            /*处理响应队列*/
            if(xQueueReceive(bm->response_queue, &resp, pdMS_TO_TICKS(100)) == pdTRUE){
                /* 检查是否为错误消息 */
                if (strstr(resp.response, "failed") != NULL) {
                    /* 直接传递错误信息给回调 */
                    if (resp.callback) {
                        resp.callback(resp.response, resp.user_data);
                    }
                    free(resp.response);
                    continue; /*跳过JSON解析*/
                }

                /* 根据响应类型处理 */
                if (resp.type == REQUEST_TYPE_CHAT) {
                    /*解析JSON响应*/
                    cJSON *root = cJSON_Parse(resp.response);
                    if(root){
                        cJSON *choices = cJSON_GetObjectItem(root, "choices");
                        if(choices && cJSON_IsArray(choices)){
                            cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
                            cJSON *message = cJSON_GetObjectItem(first_choice, "message");
                            
                            if (message) {
                                /*检查工具调用*/
                                cJSON *tool_calls = cJSON_GetObjectItem(message, "tool_calls");
                                if (tool_calls && cJSON_IsArray(tool_calls)) {
                                    /*处理所有工具调用*/
                                    int tool_count = cJSON_GetArraySize(tool_calls);
                                    for (int i = 0; i < tool_count; i++) {
                                        cJSON *tool_call = cJSON_GetArrayItem(tool_calls, i);
                                        if (tool_call) {
                                            cJSON *id = cJSON_GetObjectItem(tool_call, "id");
                                            cJSON *function = cJSON_GetObjectItem(tool_call, "function");
                                            
                                            if (id && function) {
                                                cJSON *func_name = cJSON_GetObjectItem(function, "name");
                                                cJSON *func_args = cJSON_GetObjectItem(function, "arguments");
                                                
                                                if (func_name && func_args) {
                                                    /*创建包含所有信息的JSON对象*/
                                                    cJSON *tool_info = cJSON_CreateObject();
                                                    cJSON_AddStringToObject(tool_info, "type", "TOOL_CALL");
                                                    cJSON_AddStringToObject(tool_info, "id", id->valuestring);
                                                    cJSON_AddStringToObject(tool_info, "function", func_name->valuestring);
                                                    cJSON_AddStringToObject(tool_info, "arguments", func_args->valuestring);
                                                    
                                                    /*添加助理的完整消息*/
                                                    char* assistant_content = cJSON_PrintUnformatted(message);
                                                    cJSON_AddStringToObject(tool_info, "assistant_message", assistant_content);
                                                    free(assistant_content);
                                                    
                                                    /*获取原始用户消息（从请求中）*/
                                                    cJSON *user_content = cJSON_GetObjectItem(message, "content");
                                                    if (user_content && cJSON_IsString(user_content)) {
                                                        cJSON_AddStringToObject(tool_info, "user_message", user_content->valuestring);
                                                    } else {
                                                        cJSON_AddStringToObject(tool_info, "user_message", "[Unknown user message]");
                                                    }
                                                    
                                                    /*转换为JSON字符串*/
                                                    char *tool_call_info = cJSON_PrintUnformatted(tool_info);
                                                    cJSON_Delete(tool_info);
                                                    
                                                    /*通过回调返回工具调用信息*/
                                                    if (resp.callback) resp.callback(tool_call_info, resp.user_data);
                                                    free(tool_call_info);
                                                }
                                            }
                                        }
                                    }
                                } 
                                /*处理普通文本响应*/
                                else {
                                    cJSON *content = cJSON_GetObjectItem(message, "content");
                                    if(content && cJSON_IsString(content)) {
                                        if(resp.callback) resp.callback(content->valuestring, resp.user_data);
                                    }
                                }
                            }
                        }
                        cJSON_Delete(root);
                    } else {
                        ESP_LOGE(bm->TAG, "Failed to parse response JSON: %s", resp.response);
                        if(resp.callback) resp.callback("Failed to parse response JSON", resp.user_data);
                    }
                }
                else if (resp.type == REQUEST_TYPE_IMAGE) {
                    /* 图像生成响应处理 */
                    cJSON *root = cJSON_Parse(resp.response);
                    if(root){
                        cJSON *data = cJSON_GetObjectItem(root, "data");
                        if(data && cJSON_IsArray(data)){
                            cJSON *first_image = cJSON_GetArrayItem(data, 0);
                            if(first_image){
                                cJSON *url = cJSON_GetObjectItem(first_image, "url");
                                if(url && cJSON_IsString(url)){
                                    /* 返回图片URL */
                                    if(resp.callback) resp.callback(url->valuestring, resp.user_data);
                                } else {
                                    if(resp.callback) resp.callback("Image generation failed: no URL", resp.user_data);
                                }
                            }
                        } else {
                            cJSON *error_msg = cJSON_GetObjectItem(root, "error");
                            if(error_msg && cJSON_IsObject(error_msg)){
                                cJSON *message = cJSON_GetObjectItem(error_msg, "message");
                                if(message && cJSON_IsString(message)){
                                    if(resp.callback) resp.callback(message->valuestring, resp.user_data);
                                }
                            } else {
                                if(resp.callback) resp.callback("Image generation failed: unknown error", resp.user_data);
                            }
                        }
                        cJSON_Delete(root);
                    } else {
                        if(resp.callback) resp.callback("Image generation failed: invalid JSON", resp.user_data);
                    }
                }
                free(resp.response); /*释放响应内存*/
            }
        }
        
        /*通知复位功能任务已停止*/
        if (bm->task_stop_sem) {
            xSemaphoreGive(bm->task_stop_sem);
        }
        
        //ESP_LOGI(bm->TAG, "ResponseTask exiting");
        vTaskDelete(NULL);
    }

    void BigModel::safe_stop_tasks()
    {
        /*设置停止标志*/
        stop_tasks = true;
        
        /*创建信号量用于等待任务停止*/
        task_stop_sem = xSemaphoreCreateCounting(2, 0);
        if (task_stop_sem == NULL) {
            ESP_LOGE(TAG, "Failed to create task stop semaphore");
            return;
        }
        
        /*唤醒任务以便它们可以检查停止标志*/
        if (RequestTask_handle != NULL) {
            xTaskNotify(RequestTask_handle, 0, eNoAction);
        }
        if (ResponseTask_handle != NULL) {
            xTaskNotify(ResponseTask_handle, 0, eNoAction);
        }
        
        /*等待两个任务都停止*/
        for (int i = 0; i < 2; i++) {
            if (xSemaphoreTake(task_stop_sem, pdMS_TO_TICKS(10000))) {
                //ESP_LOGI(TAG, "Task %d stopped", i);
            } else {
                ESP_LOGE(TAG, "Task %d did not stop in time", i);
            }
        }
        
        /*删除信号量*/
        vSemaphoreDelete(task_stop_sem);
        task_stop_sem = NULL;
        
        /*重置任务句柄*/
        RequestTask_handle = NULL;
        ResponseTask_handle = NULL;
    }

    BigModel::BigModel()
    {
        request_queue = NULL;
        response_queue = NULL;
        persistent_client = NULL;
        persistent_jwt = NULL;
        jwt_expiry = 0;
        RequestTask_handle = NULL;
        ResponseTask_handle = NULL;
        memset(&response_buffer, 0, sizeof(response_buffer));
        stop_tasks = false;
        task_stop_sem = NULL;
    }

    BigModel::~BigModel()
    {
        /*安全停止任务*/
        if (RequestTask_handle || ResponseTask_handle) {
            safe_stop_tasks();
        }
        
        /*清理队列*/
        if (request_queue != NULL) {
            /*清空请求队列*/
            while (uxQueueMessagesWaiting(request_queue) > 0) {
                api_request_t req;
                if (xQueueReceive(request_queue, &req, 0) == pdTRUE) {
                    /*释放请求资源*/
                    if (req.type == REQUEST_TYPE_CHAT) {
                        if (req.is_function_response) {
                            if (req.tool_call_id) free((void*)req.tool_call_id);
                            if (req.function_name) free((void*)req.function_name);
                            if (req.function_result) free((void*)req.function_result);
                            if (req.assistant_message) free((void*)req.assistant_message);
                            if (req.original_user_message) free((void*)req.original_user_message);
                        } else {
                            if (req.prompt) free(req.prompt);
                            if (req.functions) free(req.functions);
                            if (req.tool_choice) free((void*)req.tool_choice);
                        }
                    } else if (req.type == REQUEST_TYPE_IMAGE) {
                        if (req.prompt) free(req.prompt);
                        if (req.image_size) free((void*)req.image_size);
                        if (req.image_quality) free((void*)req.image_quality);
                    }
                }
            }
            vQueueDelete(request_queue);
            request_queue = NULL;
        }
        if (response_queue != NULL) {
            /*清空响应队列*/
            while (uxQueueMessagesWaiting(response_queue) > 0) {
                api_response_t resp;
                if (xQueueReceive(response_queue, &resp, 0) == pdTRUE) {
                    if (resp.response) free(resp.response);
                }
            }
            vQueueDelete(response_queue);
            response_queue = NULL;
        }
        
        /*清理HTTP客户端*/
        if (persistent_client != NULL) {
            esp_http_client_cleanup(persistent_client);
            persistent_client = NULL;
        }
        
        /*清理JWT令牌*/
        if (persistent_jwt != NULL) {
            free(persistent_jwt);
            persistent_jwt = NULL;
        }
        
        /*清理环形缓冲区*/
        if (response_buffer.buffer != NULL) {
            free(response_buffer.buffer);
            response_buffer.buffer = NULL;
        }
        if (response_buffer.mutex != NULL) {
            vSemaphoreDelete(response_buffer.mutex);
            response_buffer.mutex = NULL;
        }
    }

    void BigModel::init()
    {
        /*如果已初始化，先清理资源*/
        if (request_queue != NULL || response_queue != NULL || 
            RequestTask_handle != NULL || ResponseTask_handle != NULL) {
            ESP_LOGW(TAG, "BigModel already initialized, cleaning up first");
            reset();
        }
        
        /*重置停止标志*/
        stop_tasks = false;
        
        /*初始化环形缓冲区（例如4KB）*/
        init_ring_buffer(BIGMODEL_RESPONSE_RINGBUFFER_SIZE);
        
        /*创建队列*/
        request_queue = xQueueCreate(BIGMODEL_REQUEST_QUEUE_LEN, sizeof(api_request_t));
        if (request_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create request queue");
            return;
        }
        response_queue = xQueueCreate(BIGMODEL_RESPONSE_QUEUE_LEN, sizeof(api_response_t));
        if (response_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create response queue");
            vQueueDelete(request_queue);
            request_queue = NULL;
            return;
        }
        
        /*创建请求和响应处理任务*/
        BaseType_t ret = xTaskCreatePinnedToCore(RequestTask, "RequestTask", 8*1024, this, 
                               BIGMODEL_REQUEST_TASK_PRIOR, &RequestTask_handle, 
                               BIGMODEL_REQUEST_TASK_CORE);
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create RequestTask");
        }                       
        
        ret = xTaskCreatePinnedToCore(ResponseTask, "ResponseTask", 4*1024, this, 
                               BIGMODEL_RESPONSE_TASK_PRIOR, &ResponseTask_handle, 
                               BIGMODEL_RESPONSE_TASK_CORE);
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Failed to create ResponseTask");
        }                       
    }

    void BigModel::reset()
    {
        //ESP_LOGI(TAG, "Resetting BigModel module...");
        
        /*1. 安全停止任务*/
        safe_stop_tasks();
        
        /*2. 清理HTTP客户端*/
        if (persistent_client != NULL) {
            esp_http_client_cleanup(persistent_client);
            persistent_client = NULL;
        }
        
        /*3. 清理JWT令牌*/
        if (persistent_jwt != NULL) {
            free(persistent_jwt);
            persistent_jwt = NULL;
        }
        jwt_expiry = 0;
        
        /*4. 清空队列并删除队列对象*/
        if (request_queue != NULL) {
            /*清空请求队列中的所有消息*/
            while (uxQueueMessagesWaiting(request_queue) > 0) {
                api_request_t req;
                if (xQueueReceive(request_queue, &req, 0) == pdTRUE) {
                    /*释放请求中的资源*/
                    if (req.type == REQUEST_TYPE_CHAT) {
                        if (req.is_function_response) {
                            if (req.tool_call_id) free((void*)req.tool_call_id);
                            if (req.function_name) free((void*)req.function_name);
                            if (req.function_result) free((void*)req.function_result);
                            if (req.assistant_message) free((void*)req.assistant_message);
                            if (req.original_user_message) free((void*)req.original_user_message);
                        } else {
                            if (req.prompt) free(req.prompt);
                            if (req.functions) free(req.functions);
                            if (req.tool_choice) free((void*)req.tool_choice);
                        }
                    } else if (req.type == REQUEST_TYPE_IMAGE) {
                        if (req.prompt) free(req.prompt);
                        if (req.image_size) free((void*)req.image_size);
                        if (req.image_quality) free((void*)req.image_quality);
                    }
                }
            }
            /*删除队列对象*/
            vQueueDelete(request_queue);
            request_queue = NULL;
        }
        
        if (response_queue != NULL) {
            /*清空响应队列中的所有消息*/
            while (uxQueueMessagesWaiting(response_queue) > 0) {
                api_response_t resp;
                if (xQueueReceive(response_queue, &resp, 0) == pdTRUE) {
                    if (resp.response) {
                        free(resp.response);
                    }
                }
            }
            /*删除队列对象*/
            vQueueDelete(response_queue);
            response_queue = NULL;
        }
        
        /*5. 清空环形缓冲区*/
        ring_buffer_clear();
        
        /*6. 重新初始化模块*/
        //ESP_LOGI(TAG, "Reinitializing BigModel...");
        init();
    }

    void BigModel::request(const char *prompt, void (*callback)(char *, void *), void* user_data, TickType_t xTicksToWait,
                         FunctionDef *functions, size_t function_count,
                         const char *tool_choice)
    {
        /*复制提示词到堆内存*/
        char *prompt_copy = strdup(prompt);
        if (prompt_copy == NULL) {
            ESP_LOGE(TAG, "Failed to copy prompt");
            return;
        }
        FunctionDef *funcs_copy = NULL;
        if (functions && function_count > 0) {
            funcs_copy = (FunctionDef*)malloc(sizeof(FunctionDef) * function_count);
            if (funcs_copy == NULL) {
                ESP_LOGE(TAG, "Failed to copy functions");
                free(prompt_copy);
                return;
            }
            memcpy(funcs_copy, functions, sizeof(FunctionDef) * function_count);
        }

        char *tc_copy = NULL;
        if (tool_choice) {
            tc_copy = strdup(tool_choice);
            if (tc_copy == NULL) {
                ESP_LOGE(TAG, "Failed to copy tool_choice");
                free(prompt_copy);
                free(funcs_copy);
                return;
            }
        }

        /*创建请求结构*/
        api_request_t req = {
            .type = REQUEST_TYPE_CHAT,
            .prompt = prompt_copy,
            .callback = callback,
            .user_data = user_data,
            .functions = funcs_copy,
            .function_count = function_count,
            .tool_choice = tc_copy,
            .is_function_response = false,
            .tool_call_id = NULL,
            .function_name = NULL,
            .function_result = NULL,
            .assistant_message = NULL,
            .original_user_message = NULL,
            .image_size = NULL,
            .image_quality = NULL,
            .image_n = 0
        };
        /*将请求放入队列*/
        if (xQueueSend(request_queue, &req, xTicksToWait) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send request to queue");
            /*释放资源*/
            free(prompt_copy);
            free(funcs_copy);
            free(tc_copy);
        }
    }

    void BigModel::requestImg(const char *prompt, 
                            void (*callback)(char *, void *), 
                            void* user_data, 
                            const char *size,
                            const char *quality,
                            int n,
                            TickType_t xTicksToWait) {
        /*复制参数到堆内存*/
        char *prompt_copy = strdup(prompt);
        if (prompt_copy == NULL) {
            ESP_LOGE(TAG, "Failed to copy prompt");
            return;
        }
        
        char *size_copy = NULL;
        if (size) {
            size_copy = strdup(size);
            if (size_copy == NULL) {
                ESP_LOGE(TAG, "Failed to copy size");
                free(prompt_copy);
                return;
            }
        }
        
        char *quality_copy = NULL;
        if (quality) {
            quality_copy = strdup(quality);
            if (quality_copy == NULL) {
                ESP_LOGE(TAG, "Failed to copy quality");
                free(prompt_copy);
                free(size_copy);
                return;
            }
        }
        
        // 创建图像生成请求结构
        api_request_t req = {
            .type = REQUEST_TYPE_IMAGE,
            .prompt = prompt_copy,
            .callback = callback,
            .user_data = user_data,
            .functions = NULL,
            .function_count = 0,
            .tool_choice = NULL,
            .is_function_response = false,
            .tool_call_id = NULL,
            .function_name = NULL,
            .function_result = NULL,
            .assistant_message = NULL,
            .original_user_message = NULL,
            .image_size = size_copy,
            .image_quality = quality_copy,
            .image_n = n
        };
        
        /*将请求放入队列*/
        if (xQueueSend(request_queue, &req, xTicksToWait) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send image request to queue");
            /*释放资源*/
            free(prompt_copy);
            free(size_copy);
            free(quality_copy);
        }
    }

    void BigModel::functionResponse(const char *tool_call_id, const char *function_name, 
                                 const char *function_result, const char* assistant_message,
                                 const char* original_user_message,
                                 void (*callback)(char *, void *), 
                                 void* user_data, TickType_t xTicksToWait)
    {
        /*复制参数到堆内存*/
        char *tool_call_id_copy = strdup(tool_call_id);
        char *function_name_copy = strdup(function_name);
        char *function_result_copy = strdup(function_result);
        char *assistant_message_copy = strdup(assistant_message);
        char *original_user_message_copy = strdup(original_user_message);\

        /*检查内存分配是否成功*/
        if (!tool_call_id_copy || !function_name_copy || !function_result_copy || 
            !assistant_message_copy || !original_user_message_copy) {
            ESP_LOGE(TAG, "Memory allocation failed for function response");
            if (tool_call_id_copy) free(tool_call_id_copy);
            if (function_name_copy) free(function_name_copy);
            if (function_result_copy) free(function_result_copy);
            if (assistant_message_copy) free(assistant_message_copy);
            if (original_user_message_copy) free(original_user_message_copy);
            return;
        }

        /*创建请求结构*/
        api_request_t req = {
            .type = REQUEST_TYPE_CHAT,
            .prompt = NULL,
            .callback = callback,
            .user_data = user_data,
            .functions = NULL,
            .function_count = 0,
            .tool_choice = "auto",
            .is_function_response = true,
            .tool_call_id = tool_call_id_copy,
            .function_name = function_name_copy,
            .function_result = function_result_copy,
            .assistant_message = assistant_message_copy,
            .original_user_message = original_user_message_copy,
            .image_size = NULL,
            .image_quality = NULL,
            .image_n = 0
        };
        
        /*将请求放入队列*/
        if (xQueueSend(request_queue, &req, xTicksToWait) != pdTRUE) {
            ESP_LOGE(TAG, "Failed to send function response to queue");
            /*释放资源*/
            free(tool_call_id_copy);
            free(function_name_copy);
            free(function_result_copy);
            free(assistant_message_copy);
            free(original_user_message_copy);
        }
    }

}

