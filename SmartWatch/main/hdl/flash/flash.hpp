/**
 * @file flash.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <string>
#include <nvs_flash.h>
#include <esp_log.h>

namespace hdl{

    class flash
    {
        private:
            const char *TAG = "flash";
            std::string ns_;
            nvs_handle_t nvs_handle_ = 0;
            bool read_write_ = false;
            bool dirty_ = false;
        public:
            inline flash(const std::string& ns, bool read_write = false) : ns_(ns), read_write_(read_write)
            {
                nvs_open(ns.c_str(), read_write_ ? NVS_READWRITE : NVS_READONLY, &nvs_handle_);
            }

            inline ~flash()
            {
                if (nvs_handle_ != 0) {
                    if (read_write_ && dirty_) {
                        ESP_ERROR_CHECK(nvs_commit(nvs_handle_));
                    }
                    nvs_close(nvs_handle_);
                }
            }

            inline std::string GetString(const std::string& key, const std::string& default_value = "")
            {
                if (nvs_handle_ == 0) {
                    return default_value;
                }

                size_t length = 0;
                if (nvs_get_str(nvs_handle_, key.c_str(), NULL, &length) != ESP_OK) {
                    return default_value;
                }

                std::string value;
                value.resize(length);
                ESP_ERROR_CHECK(nvs_get_str(nvs_handle_, key.c_str(), value.data(), &length));
                while (!value.empty() && value.back() == '\0') {
                    value.pop_back();
                }
                return value;
            }

            inline void SetString(const std::string& key, const std::string& value)
            {
                if (read_write_) {
                    ESP_ERROR_CHECK(nvs_set_str(nvs_handle_, key.c_str(), value.c_str()));
                    dirty_ = true;
                } else {
                    ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
                }
            }

            inline int32_t GetInt(const std::string& key, int32_t default_value = 0)
            {
                if (nvs_handle_ == 0) {
                    return default_value;
                }

                int32_t value;
                if (nvs_get_i32(nvs_handle_, key.c_str(), &value) != ESP_OK) {
                    return default_value;
                }
                return value;
            }

            inline void SetInt(const std::string& key, int32_t value)
            {
                if (read_write_) {
                    ESP_ERROR_CHECK(nvs_set_i32(nvs_handle_, key.c_str(), value));
                    dirty_ = true;
                } else {
                    ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
                }
            }

            inline void EraseKey(const std::string& key)
            {
                if (read_write_) {
                    auto ret = nvs_erase_key(nvs_handle_, key.c_str());
                    if (ret != ESP_ERR_NVS_NOT_FOUND) {
                        ESP_ERROR_CHECK(ret);
                    }
                } else {
                    ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
                }
            }

            inline void EraseAll()
            {
                if (read_write_) {
                    ESP_ERROR_CHECK(nvs_erase_all(nvs_handle_));
                } else {
                    ESP_LOGW(TAG, "Namespace %s is not open for writing", ns_.c_str());
                }
            }
    };

}









