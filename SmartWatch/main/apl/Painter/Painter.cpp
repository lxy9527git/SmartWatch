/**
 * @file Painter.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "Painter.hpp"


namespace apl{
    /*创建消息气泡*/
    lv_obj_t* Painter::create_msg_bubble(lv_obj_t *parent, const char *text, int is_me)
    {
        /*创建主容器*/
        lv_obj_t *cont = lv_obj_create(parent);

        /*设置边框宽度为0 - 消除边框*/
        lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

        /*根据发送者设置固定位置和大小*/
        if (is_me) {
            /*自己发送的消息 - 固定位置和大小*/
            lv_obj_set_size(cont, PAINTER_MSG_BUBBLE_QUESTION_W, PAINTER_MSG_BUBBLE_QUESTION_H); 
            lv_obj_set_pos(cont, PAINTER_MSG_BUBBLE_QUESTION_X, PAINTER_MSG_BUBBLE_QUESTION_Y);   
            
            /*样式设置*/
            lv_obj_set_style_text_color(cont, lv_color_black(), 0);
        } else {
            /*对方发送的消息 - 固定位置和大小*/
            lv_obj_set_size(cont, PAINTER_MSG_BUBBLE_ANSWER_W, PAINTER_MSG_BUBBLE_ANSWER_H); 
            lv_obj_set_pos(cont, PAINTER_MSG_BUBBLE_ANSWER_X, PAINTER_MSG_BUBBLE_ANSWER_Y);  
            
            /*样式设置*/
            lv_obj_set_style_text_color(cont, lv_color_black(), 0);
        }

        /*创建消息文本*/
        lv_obj_t *msg_label = lv_label_create(cont);
        lv_label_set_text(msg_label, text);
        lv_obj_set_width(msg_label, lv_pct(100));
        lv_obj_set_style_text_font(msg_label, &MyFonts16, 0);
        lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP); /*自动换行*/
        lv_obj_set_scrollbar_mode(msg_label, LV_SCROLLBAR_MODE_OFF);
        lv_obj_align(msg_label, LV_ALIGN_TOP_MID, 0, -10);

        return cont;
    }

    void Painter::get_ai_answer(void* user_data, char* answer)
    {
        Painter* app = static_cast<Painter*>(user_data);
        
        /*检查是否为图片 URL*/
        if (strstr(answer, "https://") != nullptr || 
            strstr(answer, "http://") != nullptr) {
            /*在主线程创建图片气泡*/
            lv_async_call([](void* data) {
                AsyncData* d = static_cast<AsyncData*>(data);
                d->app->create_image_bubble(d->text);
                d->app->image_url = d->text; /*保存URL*/
                free(d->text);
                delete d;
            }, new AsyncData{app, strdup(answer)});
        } else {
            /*文本消息处理*/
            lv_async_call([](void* data){
                AsyncData* d = static_cast<AsyncData*>(data);
                d->app->add_message(d->text, 0);
                free(d->text);  /*释放内存*/
                delete d;       /*删除数据对象*/
            },new AsyncData{app, strdup(answer)});
            /*恢复发送按钮状态*/
            app->set_send_btn_busy(false);
        }
    }
    /*发送按钮事件处理*/
    void Painter::send_btn_event_cb(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        Painter* app = (Painter*)lv_event_get_user_data(e);
        if (code == LV_EVENT_CLICKED) {
            /*更新UI状态*/
            app->set_send_btn_busy(true);

            /*清空消息容器*/
            if (app->msg_cont) {
                lv_obj_clean(app->msg_cont);             /*删除所有子对象（消息气泡）*/
            }
            const char *text = lv_textarea_get_text(app->input_ta);
            if (text && text[0] != '\0') {
                /*模拟发送消息*/
                app->add_message(text, 1);
                bll::ArtificialIntelligence::getInstance().ask_img(text, get_ai_answer, app);
                lv_textarea_set_text(app->input_ta, "");
                lv_obj_add_flag(app->kb, LV_OBJ_FLAG_HIDDEN); /*发送后隐藏键盘*/
            }else {
                /*如果没有文本，恢复按钮状态*/
                app->set_send_btn_busy(false);
            }
        }
    }
    /*获取SR的拼音字符串*/
    void Painter::get_sr_pinyin(void* user_data, char* pinyin)
    {
        Painter* app = static_cast<Painter*>(user_data);
        
        /*先恢复按钮状态*/
        app->set_voice_btn_busy(false);

        std::string question(pinyin);
        question.append(",将这段拼音转为文字，不要调用工具，只要拼音的文字，其余不要。");
        
        /*设置发送按钮为忙状态（因为接下来会调用AI）*/
        app->set_send_btn_busy(true);
        
        bll::ArtificialIntelligence::getInstance().ask_question(question.c_str(), [](void* user_data, char* answer){
            Painter* app = static_cast<Painter*>(user_data);
            /*恢复发送按钮状态*/
            app->set_send_btn_busy(false);

            lv_async_call([](void* data){
                AsyncData* d = static_cast<AsyncData*>(data);
                lv_textarea_set_text(d->app->input_ta, d->text);
                free(d->text);  /*释放内存*/
                delete d;       /*删除数据对象*/
            },new AsyncData{app, strdup(answer)});
        }, user_data);
    }
    /*语音按钮事件处理*/
    void Painter::voice_btn_event_cb(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        Painter* app = (Painter*)lv_event_get_user_data(e);
        if (code == LV_EVENT_CLICKED) {
            /*更新UI状态*/
            app->set_voice_btn_busy(true);
            fml::SpeechRecongnition::getInstance().sr_multinet_clean(get_sr_pinyin, app);
        }
    }
    /*输入框事件回调*/
    void Painter::ta_event_cb(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        Painter* app = (Painter*)lv_event_get_user_data(e);

        if (code == LV_EVENT_FOCUSED) {
            if (!app->pinyin_ime || !app->kb) return;
            /*隐藏消息容器*/
            lv_obj_add_flag(app->msg_cont, LV_OBJ_FLAG_HIDDEN);

            /*将键盘与输入框关联*/
            lv_keyboard_set_textarea(app->kb, app->input_ta);
            
            /*显示键盘*/
            lv_obj_clear_flag(app->kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(app->cand_panel, LV_OBJ_FLAG_HIDDEN);

            /*调整键盘位置*/
            lv_obj_set_y(app->kb, lv_obj_get_y(app->input_ta) - lv_obj_get_height(app->kb) - 10);
            lv_obj_move_foreground(app->kb);

            lv_obj_align_to(app->cand_panel, app->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
        } 
        else if (code == LV_EVENT_DEFOCUSED) {
            /*显示消息容器*/
            lv_obj_clear_flag(app->msg_cont, LV_OBJ_FLAG_HIDDEN);
            /*隐藏键盘*/
            lv_obj_add_flag(app->kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(app->cand_panel, LV_OBJ_FLAG_HIDDEN);
            /* 恢复输入区域位置 */
            lv_obj_set_y(lv_obj_get_parent(app->input_ta), lv_obj_get_height(lv_scr_act()) - PAINTER_MAX_KB_HEIGHT - 10);       /*底部位置*/
        }
    }

    /*JPEG解码任务*/
    void Painter::jpeg_decode_task(void* arg) {
        DecodeTaskData* data = static_cast<DecodeTaskData*>(arg);
        Painter* app = data->app;
        app->decode_task_handle = xTaskGetCurrentTaskHandle(); /*记录任务句柄*/

        /*检查终止标志*/
        if (app->abort_tasks) {
            ESP_LOGW(app->TAG, "Decode task aborted before start");
            delete data;
            app->decode_task_handle = nullptr;
            vTaskDelete(NULL);
            return;
        }
        
        /*复制数据到本地*/
        app->download_buffer = std::move(data->buffer);
        delete data; /*释放任务数据*/
        
        /*执行解码*/
        bool decode_ok = app->decode_jpeg();

        /*检查终止标志*/
        if (app->abort_tasks) {
            ESP_LOGW(app->TAG, "Decode task aborted after processing");
            if (app->img_dsc.data) {
                free((void*)app->img_dsc.data);
                app->img_dsc.data = nullptr;
            }
            app->decode_task_handle = nullptr;
            vTaskDelete(NULL);
            return;
        }
        
        /*通知主线程结果*/
        if (decode_ok && app->img_dsc.data) {
            lv_async_call([](void* arg) {
                Painter* app = static_cast<Painter*>(arg);
                app->scale_and_display_image();
                /*恢复发送按钮状态*/
                app->set_send_btn_busy(false);
            }, app);
        } else {
            lv_async_call([](void* arg) {
                Painter* app = static_cast<Painter*>(arg);
                if (app->current_image) {
                    lv_obj_t* parent = lv_obj_get_parent(app->current_image);
                    lv_obj_clean(parent);
                    lv_obj_t* label = lv_label_create(parent);
                    lv_label_set_text(label, "解码失败");
                    lv_obj_set_style_text_font(label, &MyFonts16, 0);
                    lv_obj_center(label);
                    /*恢复发送按钮状态*/
                    app->set_send_btn_busy(false);
                }
            }, app);
        }
        app->decode_task_handle = nullptr; /*清除任务句柄*/
        vTaskDelete(NULL);
    }

    void Painter::reset()
    {
        /* 1. 设置任务终止标志 */
        abort_tasks = true;
        
        /* 2. 唤醒并等待下载任务退出 */
        if (download_task_handle != nullptr) {
            /*发送通知唤醒可能阻塞的任务*/
            xTaskNotify(download_task_handle, 0, eNoAction);
            /*等待任务退出（最多50ms）*/
            for (int i = 0; i < 5; i++) {
                vTaskDelay(pdMS_TO_TICKS(10));
                if (download_task_handle == nullptr) break;
            }
            /*如果任务仍未退出，强制终止*/
            if (download_task_handle != nullptr) {
                vTaskDelete(download_task_handle);
                download_task_handle = nullptr;
                ESP_LOGW(TAG, "Force deleted download task");
            }
        }
        
        /* 3. 唤醒并等待解码任务退出 */
        if (decode_task_handle != nullptr) {
            xTaskNotify(decode_task_handle, 0, eNoAction);
            for (int i = 0; i < 5; i++) {
                vTaskDelay(pdMS_TO_TICKS(10));
                if (decode_task_handle == nullptr) break;
            }
            if (decode_task_handle != nullptr) {
                vTaskDelete(decode_task_handle);
                decode_task_handle = nullptr;
                ESP_LOGW(TAG, "Force deleted decode task");
            }
        }
        
        /* 4. 清空消息容器 */
        if (msg_cont) {
            lv_obj_clean(msg_cont);
        }
        
        /* 5. 清理图像资源 */
        if (img_dsc.data) {
            free((void*)img_dsc.data);
            img_dsc.data = nullptr;
        }
        memset(&img_dsc, 0, sizeof(lv_img_dsc_t));
        
        /* 6. 隐藏键盘和候选面板 */
        if (kb) lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if (cand_panel) lv_obj_add_flag(cand_panel, LV_OBJ_FLAG_HIDDEN);
        
        /* 7. 清空输入框 */
        if (input_ta) lv_textarea_set_text(input_ta, "");
        
        /* 8. 重置按钮状态 */
        set_send_btn_busy(false);
        set_voice_btn_busy(false);
        
        /* 9. 确保消息容器可见 */
        if (msg_cont) lv_obj_clear_flag(msg_cont, LV_OBJ_FLAG_HIDDEN);
        
        /* 10. 重置图像状态 */
        current_image = NULL;
        image_url.clear();
        download_buffer.clear();
        download_buffer.shrink_to_fit();
        downloaded_size = 0;
        is_jpeg_valid = false;
        
        /* 11. 重置任务标志 */
        abort_tasks = false;
    }

    /*启动图片下载*/
    void Painter::start_image_download(const char* url) {
        image_url = url;
        /*创建下载任务*/
        xTaskCreatePinnedToCore(
        [](void* arg) {
            Painter* app = static_cast<Painter*>(arg);
            app->download_task_handle = xTaskGetCurrentTaskHandle(); /*记录任务句柄*/
            app->download_image();
            app->download_task_handle = nullptr;        /*清除任务句柄*/
            vTaskDelete(NULL);
        },                                              /*任务函数*/
        "jpeg_download_task",                           /*任务名称*/
        4096,                                           /*堆栈大小*/
        this,                                           /*参数*/
        PAINTER_JPEG_DOWNLOAD_TASK_PRIOR,               /*优先级*/
        NULL,                                           /*任务句柄*/
        PAINTER_JPEG_DOWNLOAD_TASK_CORE);               /*内核号*/
    }

    /*JPEG解码函数*/
    bool Painter::decode_jpeg() {
        if (!is_jpeg_valid) {
            ESP_LOGE(TAG, "Skipping decode, invalid JPEG");
            return false;
        }
        
        /*检查图片大小是否超过限制*/
        if (download_buffer.size() > PAINTER_MAX_IMAGE_SIZE) {
            ESP_LOGE(TAG, "Image too large (%d bytes > %d bytes limit)", 
                     download_buffer.size(), PAINTER_MAX_IMAGE_SIZE);
            return false;
        }
        
        /*使用JpegDecoder进行解码*/
        auto result = fml::JpegDecoder::Decode(
            download_buffer.data(), 
            download_buffer.size(),
            PAINTER_MSG_BUBBLE_ANSWER_W,        /*目标宽度*/
            PAINTER_MSG_BUBBLE_ANSWER_H,        /*目标高度*/
            BLL_JPEG_PIXEL_FORMAT,              /*输出格式*/
            BLL_JPEG_ROTATE                     /*旋转角度*/
        );
        
        if (!result.success) {
            ESP_LOGE(TAG, "JPEG decode failed: %s", result.error_message.c_str());
            return false;
        }
        
        /*创建图像描述符*/
        memset(&img_dsc, 0, sizeof(lv_img_dsc_t));
        img_dsc.header.w = result.width;
        img_dsc.header.h = result.height;
        img_dsc.header.cf = BLL_LV_COLOR_FORMAT; 
        img_dsc.data_size = result.data_size;
        img_dsc.data = result.data.release();   /*转移数据所有权*/
        
        return true;
    }

    /*缩放并显示图像*/
    void Painter::scale_and_display_image() {
        if (!img_dsc.data) return;
        
        if (current_image) {
            lv_img_set_src(current_image, &img_dsc);
            lv_obj_clear_flag(current_image, LV_OBJ_FLAG_HIDDEN);
            
            /*删除加载提示*/
            lv_obj_t* parent = lv_obj_get_parent(current_image);
            /*获取子对象列表*/
            std::vector<lv_obj_t*> to_delete;
            uint32_t child_count = lv_obj_get_child_cnt(parent);
            /*收集需要删除的对象*/
            for (uint32_t i = 0; i < child_count; i++) {
                lv_obj_t* child = lv_obj_get_child(parent, i);
                if (child != current_image) {
                    to_delete.push_back(child);
                }
            }
            /*删除收集到的对象*/
            for (lv_obj_t* child : to_delete) {
                lv_obj_del(child);
            }
        }
    }

    /*创建图片消息气泡*/
    void Painter::create_image_bubble(const char* url) {
        /*创建容器*/
        lv_obj_t* cont = lv_obj_create(msg_cont);
        lv_obj_set_size(cont, PAINTER_MSG_BUBBLE_ANSWER_W, PAINTER_MSG_BUBBLE_ANSWER_H);
        lv_obj_set_style_border_width(cont, 0, 0);
        lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(cont, 0, 0);
        lv_obj_align(cont, LV_ALIGN_TOP_LEFT, PAINTER_MSG_BUBBLE_ANSWER_X, PAINTER_MSG_BUBBLE_ANSWER_Y);
        
        /*创建图像对象，初始隐藏*/
        current_image = lv_img_create(cont);
        lv_image_set_inner_align(current_image, LV_IMAGE_ALIGN_CENTER);
        lv_obj_set_size(current_image, PAINTER_MSG_BUBBLE_ANSWER_W, PAINTER_MSG_BUBBLE_ANSWER_H);
        lv_obj_set_style_radius(current_image, 15, 0);
        lv_obj_align(current_image, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_flag(current_image, LV_OBJ_FLAG_HIDDEN);
        
        /*添加加载提示*/
        lv_obj_t* label = lv_label_create(cont);
        lv_label_set_text(label, "加载中...");
        lv_obj_set_style_text_font(label, &MyFonts16, 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        
        /*开始下载*/
        start_image_download(url);
    }

    /*HTTP事件处理函数*/
    esp_err_t Painter::http_event_handler(esp_http_client_event_t *evt) {
        Painter* app = (Painter*)evt->user_data;
        
        switch (evt->event_id) {
            case HTTP_EVENT_ON_DATA:
                if (app) {
                    /*检查图片大小是否超过限制*/
                    app->downloaded_size += evt->data_len;
                    if (app->downloaded_size > PAINTER_MAX_IMAGE_SIZE) {
                        ESP_LOGE(app->TAG, "Image too large (%d bytes > %d bytes limit)", 
                                 app->downloaded_size, PAINTER_MAX_IMAGE_SIZE);
                        return ESP_FAIL; /*终止下载*/
                    }

                    /*追加数据到缓冲区*/
                    const uint8_t* new_data = static_cast<const uint8_t*>(evt->data);
                    app->download_buffer.insert(app->download_buffer.end(), new_data, new_data + evt->data_len);
                    
                    /*检查签名 (只在开始时检查一次)*/
                    if (!app->is_jpeg_valid && app->download_buffer.size() >= 4) {
                        app->is_jpeg_valid = app->verify_jpeg_signature();
                        
                        if (!app->is_jpeg_valid) {
                            ESP_LOGE(app->TAG, "Invalid JPEG signature, aborting");
                            return ESP_FAIL;
                        }
                    }
                }
                break;
                
            case HTTP_EVENT_ON_FINISH:
                if (app) {
                    /*检查是否超过大小限制*/
                    if (app->downloaded_size > PAINTER_MAX_IMAGE_SIZE) {
                        lv_async_call([](void* arg) {
                            Painter* app = static_cast<Painter*>(arg);
                            if (app->current_image) {
                                lv_obj_t* parent = lv_obj_get_parent(app->current_image);
                                lv_obj_clean(parent);
                                lv_obj_t* label = lv_label_create(parent);
                                lv_label_set_text(label, "图片过大");
                                lv_obj_set_style_text_font(label, &MyFonts16, 0);
                                lv_obj_center(label);
                                /*恢复发送按钮状态*/
                                app->set_send_btn_busy(false);
                            }
                        }, app);
                    } else if (app->is_jpeg_valid) {
                        /*创建解码任务数据*/
                        DecodeTaskData* task_data = new DecodeTaskData{
                            .app = app,
                            .buffer = std::move(app->download_buffer) /*转移数据所有权*/
                        };
                        
                        /*清空原始缓冲区*/
                        app->download_buffer.clear();
                        
                        /*创建解码任务*/
                        xTaskCreatePinnedToCore(
                            jpeg_decode_task,                               /*任务函数*/
                            "jpeg_decode_task",                             /*任务名称*/
                            4096,                                           /*堆栈大小*/
                            task_data,                                      /*参数*/
                            PAINTER_JPEG_DECODE_TASK_PRIOR,                 /*优先级*/
                            NULL,                                           /*任务句柄（不需要保存）*/
                            PAINTER_JPEG_DECODE_TASK_CORE);                 /*内核号*/
                    } else {
                        lv_async_call([](void* arg) {
                            Painter* app = static_cast<Painter*>(arg);
                            if (app->current_image) {
                                lv_obj_t* parent = lv_obj_get_parent(app->current_image);
                                lv_obj_clean(parent);
                                lv_obj_t* label = lv_label_create(parent);
                                lv_label_set_text(label, "解码失败");
                                lv_obj_set_style_text_font(label, &MyFonts16, 0);
                                lv_obj_center(label);
                                /*恢复发送按钮状态*/
                                app->set_send_btn_busy(false);
                            }
                        }, app);
                    }
                }
                break;
                
            case HTTP_EVENT_ERROR:
                lv_async_call([](void* arg) {
                    Painter* app = static_cast<Painter*>(arg);
                    if (app->current_image) {
                        lv_obj_t* parent = lv_obj_get_parent(app->current_image);
                        
                        /*根据错误类型显示不同消息*/
                        if (app->downloaded_size > PAINTER_MAX_IMAGE_SIZE) {
                            lv_obj_clean(parent);
                            lv_obj_t* label = lv_label_create(parent);
                            lv_label_set_text(label, "图片过大");
                            lv_obj_set_style_text_font(label, &MyFonts16, 0);
                            lv_obj_center(label);
                        } else {
                            lv_obj_clean(parent);
                            lv_obj_t* label = lv_label_create(parent);
                            lv_label_set_text(label, "下载失败");
                            lv_obj_set_style_text_font(label, &MyFonts16, 0);
                            lv_obj_center(label);
                        }
                        /*恢复发送按钮状态*/
                        app->set_send_btn_busy(false);
                    }
                }, app);
                break;
                
            default:
                break;
        }
        return ESP_OK;
    }

    /*下载图片*/
    void Painter::download_image() {
        is_jpeg_valid = false;
        downloaded_size = 0;
        download_buffer.clear();
        download_buffer.reserve(PAINTER_MAX_IMAGE_SIZE); /*预分配内存*/
        /*清理图像资源*/
        if (img_dsc.data) {
            free((void*)img_dsc.data);
            img_dsc.data = nullptr;
        }
        
        esp_http_client_config_t config = {
            .url = image_url.c_str(),
            .timeout_ms = 15000,
            .event_handler = http_event_handler,
            .buffer_size = 4096,
            .user_data = this,
        };
        
        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (!client) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            lv_async_call([](void* arg) {
                Painter* app = static_cast<Painter*>(arg);
                if (app->current_image) {
                    lv_obj_t* parent = lv_obj_get_parent(app->current_image);
                    lv_obj_clean(parent);
                    lv_obj_t* label = lv_label_create(parent);
                    lv_label_set_text(label, "下载失败");
                    lv_obj_set_style_text_font(label, &MyFonts16, 0);
                    lv_obj_center(label);
                }
                /*恢复发送按钮状态*/
                app->set_send_btn_busy(false);
            }, this);
            return;
        }

        esp_err_t err = ESP_OK;
        while (!abort_tasks) {
            err = esp_http_client_perform(client);
            if (err != ESP_ERR_HTTP_EAGAIN) break;          /*完成或非重试错误*/
            vTaskDelay(pdMS_TO_TICKS(10));                  /*短暂等待后重试*/
        }
        
        /*如果被终止，清理资源*/
        if (abort_tasks) {
            ESP_LOGW(TAG, "HTTP download aborted");
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return;
        }
        
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
            lv_async_call([](void* arg) {
                Painter* app = static_cast<Painter*>(arg);
                if (app->current_image) {
                    lv_obj_t* parent = lv_obj_get_parent(app->current_image);
                    lv_obj_clean(parent);
                    lv_obj_t* label = lv_label_create(parent);
                    lv_label_set_text(label, "下载失败");
                    lv_obj_set_style_text_font(label, &MyFonts16, 0);
                    lv_obj_center(label);
                    /*恢复发送按钮状态*/
                    app->set_send_btn_busy(false);
                }
            }, this);
        }
        
        esp_http_client_cleanup(client);
    }

    /*JPEG签名验证函数*/
    bool Painter::verify_jpeg_signature() {
        if (download_buffer.size() < 4) {
            return false;
        }
        
        /*检查JPEG签名: 0xFF, 0xD8*/
        if (download_buffer[0] != 0xFF || download_buffer[1] != 0xD8) {
            ESP_LOGE(TAG, "Invalid JPEG signature");
            return false;
        }
        
        return true;
    }

    /*设置发送按钮状态*/
    void Painter::set_send_btn_busy(bool enabled) {
        /*设置按钮状态*/
        {
            std::lock_guard<std::mutex> lock(btn_mutex);
            is_send_btn_busy = enabled;
        }
        lv_async_call([](void* arg) {
            Painter* app = static_cast<Painter*>(arg);
            if (!app->is_send_btn_busy) {
                lv_obj_set_style_bg_color(app->send_btn, lv_color_hex(PAINTER_SEND_BTN_UP_COLOR), 0);
                lv_obj_clear_state(app->send_btn, LV_STATE_DISABLED);
            } else {
                lv_obj_set_style_bg_color(app->send_btn, lv_color_hex(PAINTER_BTN_DISABLED_COLOR), 0);
                lv_obj_add_state(app->send_btn, LV_STATE_DISABLED);
            }
        }, this);
    }

    /*设置语音按钮状态*/
    void Painter::set_voice_btn_busy(bool enabled) {
        /*设置按钮状态*/
        {
            std::lock_guard<std::mutex> lock(btn_mutex);
            is_voice_btn_busy = enabled;
        }
        lv_async_call([](void* arg) {
            Painter* app = static_cast<Painter*>(arg);
            if (!app->is_voice_btn_busy) {
                lv_obj_set_style_bg_color(app->voice_btn, lv_color_hex(PAINTER_VOICE_BTN_UP_COLOR), 0);
                lv_obj_clear_state(app->voice_btn, LV_STATE_DISABLED);
            } else {
                lv_obj_set_style_bg_color(app->voice_btn, lv_color_hex(PAINTER_BTN_DISABLED_COLOR), 0);
                lv_obj_add_state(app->voice_btn, LV_STATE_DISABLED);
            }
        }, this);
    }

    Painter::Painter(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* container)
    {
        memset(&lv_screen, 0, sizeof(lv_screen));
        lv_screen.screen = screen;
        lv_screen.tileview = tileview;
        lv_screen.container = container;

        main_cont = NULL;
        title_bar = NULL;
        title_label = NULL;
        msg_cont = NULL;
        input_cont = NULL;
        input_ta = NULL;
        send_btn = NULL;
        send_label = NULL;
        voice_btn = NULL;
        voice_label = NULL;
        pinyin_ime = NULL;
        kb = NULL;
        cand_panel = NULL;
        /*初始化JPEG解码相关变量*/
        memset(&img_dsc, 0, sizeof(img_dsc));
        current_image = NULL;
        is_jpeg_valid = false;
        downloaded_size = 0;
        abort_tasks = false;
        decode_task_handle = NULL;
        download_task_handle = NULL;
        is_send_btn_busy = false;
        is_voice_btn_busy = false;
        ESP_LOGI(TAG, "Painter on construct");
    }

    Painter::~Painter()
    {
        /* 强制终止所有后台任务 */
        abort_tasks = true;

        /*终止下载任务*/
        if (download_task_handle != NULL) {
            if (eTaskGetState(download_task_handle) != eDeleted) {
                vTaskDelete(download_task_handle);
                ESP_LOGW(TAG, "Force deleted download task in destructor");
            }
            download_task_handle = NULL;
        }
        
        /*终止解码任务*/
        if (decode_task_handle != NULL) {
            if (eTaskGetState(decode_task_handle) != eDeleted) {
                vTaskDelete(decode_task_handle);
                ESP_LOGW(TAG, "Force deleted decode task in destructor");
            }
            decode_task_handle = NULL;
        }

        /*确保清理所有资源*/
        reset(); 

        /*删除主容器及其所有子对象*/
        if (main_cont) {
            lv_obj_del(main_cont);
            main_cont = NULL;
        }
        
        /*重置所有指针*/
        title_bar = NULL;
        title_label = NULL;
        msg_cont = NULL;
        input_cont = NULL;
        input_ta = NULL;
        send_btn = NULL;
        send_label = NULL;
        voice_btn = NULL;
        voice_label = NULL;
        pinyin_ime = NULL;
        kb = NULL;
        cand_panel = NULL;
        current_image = NULL;
        ESP_LOGI(TAG, "Painter on deconstruct");
    }
    /*添加消息到容器*/
    void Painter::add_message(const char *text, int is_me)
    {
        /*创建消息气泡 - 位置已在 create_msg_bubble 中固定*/
        create_msg_bubble(msg_cont, text, is_me);
    }
    /*初始化拼音输入法*/
    void Painter::init_pinyin_input()
    {
        /*创建拼音输入法*/
        pinyin_ime = lv_ime_pinyin_create(lv_screen.container);
        lv_obj_set_style_text_font(pinyin_ime, &MyFonts16, 0);
        lv_ime_pinyin_set_dict(pinyin_ime, MyFonts16Dict);
        /*设置候选框中文字体*/
        cand_panel = lv_ime_pinyin_get_cand_panel(pinyin_ime);
        lv_obj_set_style_text_font(cand_panel, &MyFonts16, 0);
        lv_obj_set_size(cand_panel, LV_PCT(100), LV_PCT(10));
        /*创建键盘*/
        kb = lv_keyboard_create(lv_screen.container);
        lv_obj_set_size(kb, LV_PCT(100), PAINTER_MAX_KB_HEIGHT + 20);      
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);    /*初始隐藏*/
        lv_obj_set_style_bg_color(kb, lv_color_hex(0xF0F0F0), 0);
        /* 设置键盘内容高度 */
        lv_obj_set_content_height(kb, PAINTER_MAX_KB_HEIGHT);
        lv_keyboard_set_textarea(kb, input_ta);
        
        /* 绑定输入法 */
        if (pinyin_ime && kb) {
            lv_ime_pinyin_set_keyboard(pinyin_ime, kb);
        }
    }
    /*创建聊天界面*/
    void Painter::create_chat_ui()
    {
        /*创建主容器*/
        main_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
        lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(main_cont, 0, 0);
        lv_obj_set_style_border_width(main_cont, 0, 0);
        lv_obj_set_style_bg_color(main_cont, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_bg_image_src(main_cont, &_painter_bg_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(main_cont, LV_OPA_COVER, 0);              /*设置不透明度*/
        lv_obj_set_style_bg_image_tiled(main_cont, false, 0);                   /*禁止平铺*/
        lv_obj_set_style_bg_image_recolor_opa(main_cont, LV_OPA_TRANSP, 0);     /*禁用颜色覆盖*/
        lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);                   /*禁用滚动*/
        /*顶部标题栏*/
        title_bar = lv_obj_create(main_cont);
        lv_obj_set_size(title_bar, lv_pct(100), PAINTER_TITLE_BAR_HEIGHT);
        lv_obj_set_style_bg_opa(title_bar, LV_OPA_TRANSP, 0); 
        lv_obj_set_style_border_width(title_bar, 0, 0);
        lv_obj_set_style_pad_all(title_bar, 0, 0);
        /*标题*/
        title_label = lv_label_create(title_bar);
        lv_label_set_text(title_label, "小画家");
        lv_obj_set_style_text_font(title_label, &MyFonts16, 0);
        lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 10);
        /*创建消息容器*/
        msg_cont = lv_obj_create(main_cont);
        lv_obj_set_size(msg_cont, lv_pct(100), PAINTER_MSG_AREA_HEIGHT); 
        lv_obj_set_style_border_width(msg_cont, 0, 0);
        lv_obj_set_style_bg_opa(msg_cont, LV_OPA_TRANSP, 0); 
        lv_obj_set_style_pad_all(msg_cont, 0, 0);
        lv_obj_clear_flag(msg_cont, LV_OBJ_FLAG_SCROLLABLE); /*禁用滚动*/
        /*底部输入区域*/
        input_cont = lv_obj_create(main_cont);
        lv_obj_set_size(input_cont, lv_pct(100), PAINTER_INPUT_CONT_HEIGHT);
        lv_obj_set_style_border_width(input_cont, 1, 0);
        lv_obj_set_style_border_color(input_cont, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_bg_opa(input_cont, LV_OPA_TRANSP, 0); 
        lv_obj_set_flex_flow(input_cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(input_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(input_cont, 5, 0); /*内边距减小*/
        /*语音按钮*/
        voice_btn = lv_btn_create(input_cont);
        lv_obj_set_size(voice_btn, PAINTER_SEND_BTN_WIDTH, PAINTER_SEND_BTN_HEIGHT);
        lv_obj_set_style_bg_color(voice_btn, lv_color_hex(PAINTER_VOICE_BTN_UP_COLOR), 0); 
        lv_obj_set_style_radius(voice_btn, 8, 0);
        voice_label = lv_label_create(voice_btn);
        lv_label_set_text(voice_label, LV_SYMBOL_VOLUME_MAX); 
        lv_obj_set_style_text_font(voice_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(voice_label, lv_color_white(), 0);
        lv_obj_center(voice_label);
        lv_obj_add_event_cb(voice_btn, voice_btn_event_cb, LV_EVENT_CLICKED, this);
        /*输入框*/
        input_ta = lv_textarea_create(input_cont);
        lv_obj_set_flex_grow(input_ta, 1);
        lv_obj_set_height(input_ta, PAINTER_INPUT_TA_HEIGHT);
        lv_textarea_set_placeholder_text(input_ta, "写入问题");
        lv_textarea_set_one_line(input_ta, true);
        lv_obj_set_style_text_font(input_ta, &MyFonts16, 0);
        lv_obj_add_event_cb(input_ta, ta_event_cb, LV_EVENT_ALL, this);
        /*发送按钮*/
        send_btn = lv_btn_create(input_cont);
        lv_obj_set_size(send_btn, PAINTER_SEND_BTN_WIDTH, PAINTER_SEND_BTN_HEIGHT);
        lv_obj_set_style_bg_color(send_btn, lv_color_hex(PAINTER_SEND_BTN_UP_COLOR), 0);
        lv_obj_set_style_radius(send_btn, 8, 0);
        send_label = lv_label_create(send_btn);
        lv_label_set_text(send_label, LV_SYMBOL_GPS);
        lv_obj_set_style_text_font(send_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(send_label, lv_color_white(), 0);
        lv_obj_center(send_label);
        lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, this);
    }

    void Painter::onCreate()
    {
        create_chat_ui();
        init_pinyin_input();
        ESP_LOGI(TAG, "Painter on create");
    }

    void Painter::onOpen()
    {
        fml::HdlManager::getInstance().set_no_sleep_for_lvgl();
        bll::ArtificialIntelligence::getInstance().reset();
        reset();
        ESP_LOGI(TAG, "Painter on Open");
    }

    void Painter::onRunning()
    {
        //ESP_LOGI(TAG, "Painter on Running");
    }

    void Painter::onSleeping()
    {
        //ESP_LOGI(TAG, "Painter on Sleeping");
    }

    void Painter::onClose()
    {
        fml::HdlManager::getInstance().clear_no_sleep_for_lvgl();
        reset();
        ESP_LOGI(TAG, "Painter on Close");
    }

    void Painter::onDestroy()
    {
        ESP_LOGI(TAG, "Painter on destroy");
    }

}


