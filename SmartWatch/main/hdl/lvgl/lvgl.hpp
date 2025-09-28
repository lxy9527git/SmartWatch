/**
 * @file lvgl.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <esp_log.h>
#include <lvgl.h>
#include "disp.hpp"
#include "esp_timer.h"

namespace hdl{

    class lvgl
    {
        private:
            /*私有构造函数，禁止外部直接实例化*/
            lvgl(){}
            /*禁止拷贝构造和赋值操作*/
            lvgl(const lvgl&) = delete;
            lvgl& operator = (const lvgl&) = delete;

            /*Flush the content of the internal buffer the specific area on the display
             *You can use DMA or any hardware acceleration to do this operation in the background but
             *'lv_disp_flush_ready()' has to be called when finished.*/
            inline static void _disp_flush(lv_disp_t *drv, const lv_area_t *area, uint8_t *px_map)
            {
                uint32_t w = (area->x2 - area->x1 + 1);
                uint32_t h = (area->y2 - area->y1 + 1); 

                disp::getInstance().startWrite();
                disp::getInstance().setWindow(area->x1, area->y1, area->x2, area->y2);
                disp::getInstance().pushPixels((uint16_t*)px_map, w * h, true);
                disp::getInstance().endWrite();

                /*IMPORTANT!!!
                *Inform the graphics library that you are ready with the flushing*/
                lv_disp_flush_ready(drv);
            }

            /*Will be called by the library to read the touchpad*/
            inline static void _touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
            {
                if(disp::getInstance().getTouchRaw(&disp::getInstance()._tpp)){
                    data->point.x = disp::getInstance()._tpp.x;
                    data->point.y = disp::getInstance()._tpp.y;
                    data->state = LV_INDEV_STATE_PR;
                }else{
                    data->state = LV_INDEV_STATE_REL;
                }
            }

            inline static void increase_lvgl_tick(void *arg)
            {
                lv_tick_inc(1);
            }

            inline static void _lv_tick_init()
            {

                const esp_timer_create_args_t lvgl_tick_timer_args = {
                        .callback = &increase_lvgl_tick,
                        .arg = NULL,
                        .dispatch_method = ESP_TIMER_TASK,
                        .name = "lvgl_tick",
                        .skip_unhandled_events = true
                    };
                esp_timer_handle_t lvgl_tick_timer = NULL;
                ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
                ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 1 * 1000)); //创建定时器，更新LVGL的内部时钟基准 
            }

            inline static void _lv_port_disp_init(uint16_t depth_size)
            {
                static lv_display_t *disp = lv_display_create(disp::getInstance().width(),disp::getInstance().height());
                void *buf1 = NULL;
                void *buf2 = NULL;

                //分配内存
                buf1 = heap_caps_malloc(disp::getInstance().width() * disp::getInstance().height() * depth_size, MALLOC_CAP_SPIRAM);
                buf2 = heap_caps_malloc(disp::getInstance().width() * disp::getInstance().height() * depth_size, MALLOC_CAP_SPIRAM);
                /* If failed */
                if ((buf1 == NULL) || (buf2 == NULL)) {
                    ESP_LOGE(getInstance().TAG, "malloc buffer from PSRAM fialed:%d,%d,%d", (int)disp::getInstance().width(), (int)disp::getInstance().height(), (int)depth_size);
                    abort();
                } else {
                    ESP_LOGI(getInstance().TAG, "malloc buffer from PSRAM successful:%d,%d,%d", (int)disp::getInstance().width(), (int)disp::getInstance().height(), (int)depth_size);
                    ESP_LOGI(getInstance().TAG, "free PSRAM: %d\r\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
                }

                //注册
                lv_display_set_flush_cb(disp, _disp_flush);
                lv_display_set_buffers(disp, buf1, buf2, disp::getInstance().width() * disp::getInstance().height() * depth_size, LV_DISPLAY_RENDER_MODE_FULL);
            }

            inline static void _lv_port_indev_init()
            {
                static lv_indev_t *indev = lv_indev_create();
                lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
                lv_indev_set_read_cb(indev, _touchpad_read);
            }
        public:
            const char* TAG = "lvgl";

            /*获取单例实例的静态方法*/
            inline static lvgl& getInstance() {
                static lvgl instance;
                return instance;
            }    

            inline static void init(uint16_t depth_size)
            {
                /* Display + IIC */
                disp::getInstance().init();
                disp::getInstance().setBrightness(200);
                
                lv_init();
                _lv_port_disp_init(depth_size);
                _lv_port_indev_init();
                _lv_tick_init();
            }

            inline static void update(){ lv_timer_handler();}
    };
}