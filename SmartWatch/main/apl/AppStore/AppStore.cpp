/**
 * @file AppStore.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "AppStore.hpp"

const lv_image_dsc_t* marquee_img_srcs[] = {&_assistant_icon_RGB565_40x40, 
                                            &_painter_icon_RGB565_40x40,
                                            &_gamepad_icon_RGB565_40x40};  
                                            
const int marquee_img_count = sizeof(marquee_img_srcs) / sizeof(marquee_img_srcs[0]);

namespace apl{

    void AppStore::bg_img_event_cb(lv_event_t* e)
    {
        static int old_jpeg_index = -1;
        if(lv_event_get_code(e) == LV_EVENT_DRAW_POST_END){
            if(old_jpeg_index != ((AppStore*)lv_event_get_user_data(e))->lv_bg.cur_jpeg_index){
                /*图片已经渲染到屏幕*/
                fml::JpegDecoder::getInstance().StartJpegDec(((AppStore*)lv_event_get_user_data(e))->lv_bg.cur_jpeg_index, ((AppStore*)lv_event_get_user_data(e))->lv_bg.jpeg_src);
                old_jpeg_index = ((AppStore*)lv_event_get_user_data(e))->lv_bg.cur_jpeg_index;
            }
        }
    }

    void AppStore::get_lv_bg(struct appstore_lv_bg_t* lv_bg, struct appstore_lv_screen_t* lv_screen, void* arg)
    {
        if(lv_bg != NULL && lv_screen != NULL){
            fml::JpegDecoder::FindJpegMaterial(BLL_JPEG_DESC_PATH,APPSTORE_TAG_NAME,&lv_bg->start_jpeg_index, &lv_bg->end_jpeg_index);
            lv_bg->jpeg_src.jpeg_len = BLL_JPEG_OUTPUT_WIDTH * BLL_JPEG_OUTPUT_HEIGHT * BLL_JPEG_PIXEL_BYTE;
            lv_bg->jpeg_src.jpeg_buff = (uint8_t*)heap_caps_aligned_alloc(16, lv_bg->jpeg_src.jpeg_len, MALLOC_CAP_SPIRAM);
            lv_bg->cur_jpeg_index = lv_bg->start_jpeg_index;
            lv_bg->img = lv_img_create(lv_screen->tile);
            lv_obj_add_event_cb(lv_bg->img, bg_img_event_cb, LV_EVENT_DRAW_POST_END, arg);
            lv_obj_align(lv_bg->img, LV_ALIGN_CENTER, 0, 0);
            //设置背景图像描述
            lv_bg->img_dsc.header.w = BLL_JPEG_OUTPUT_WIDTH;
            lv_bg->img_dsc.header.h = BLL_JPEG_OUTPUT_HEIGHT;
            lv_bg->img_dsc.header.cf = BLL_LV_COLOR_FORMAT; 
            lv_bg->img_dsc.data = lv_bg->jpeg_src.jpeg_buff;
            lv_bg->img_dsc.data_size = lv_bg->jpeg_src.jpeg_len;
        }
    }

    void AppStore::set_lv_bg(struct appstore_lv_bg_t* lv_bg, uint8_t* data)
    {
        if(lv_bg != NULL && data != NULL){
            lv_bg->img_dsc.data = data;
            lv_img_set_src(lv_bg->img, &lv_bg->img_dsc);  
            lv_obj_align(lv_bg->img, LV_ALIGN_CENTER, 0, 0);
        }
    }

    void AppStore::lv_marquee_timer_cb(lv_timer_t *timer)
    {
        struct appstore_lv_marquee_t* lv_marquee = (struct appstore_lv_marquee_t*)lv_timer_get_user_data(timer);
        /*移动顶部容器所有图片（向左）*/
        for (uint32_t i = 0; i < lv_obj_get_child_cnt(lv_marquee->container_top); i++) {
            lv_obj_t *child = lv_obj_get_child(lv_marquee->container_top, i);
            lv_obj_set_x(child, lv_obj_get_x(child) - APPSTORE_LV_MARQUEE_MOVE_SPEED);
        }
        /*移动底部容器所有图片（向右）*/
        for (uint32_t i = 0; i < lv_obj_get_child_cnt(lv_marquee->container_bottom); i++) {
            lv_obj_t *child = lv_obj_get_child(lv_marquee->container_bottom, i);
            lv_obj_set_x(child, lv_obj_get_x(child) + APPSTORE_LV_MARQUEE_MOVE_SPEED);
        }
        /*检查顶部容器首图是否移出左侧*/
        if (lv_obj_get_child_cnt(lv_marquee->container_top)) {
            lv_obj_t *first = lv_obj_get_child(lv_marquee->container_top, 0);
            if (lv_obj_get_x(first) + APPSTORE_LV_MARQUEE_ICON_W < 0) {
                /*计算新位置（底部容器末尾）*/
                lv_coord_t new_x = -APPSTORE_LV_MARQUEE_ICON_W;
                if (lv_obj_get_child_cnt(lv_marquee->container_bottom)) {
                    lv_obj_t *last = lv_obj_get_child(lv_marquee->container_bottom, 
                        lv_obj_get_child_cnt(lv_marquee->container_bottom) - 1);
                    new_x = lv_obj_get_x(last) - APPSTORE_LV_MARQUEE_ICON_W - 10;
                }
                lv_obj_set_parent(first, lv_marquee->container_bottom);
                lv_obj_set_pos(first, new_x, 0);
            }
        }
        /*检查底部容器首图是否移出右侧*/
        if (lv_obj_get_child_cnt(lv_marquee->container_bottom)) {
            lv_obj_t *first = lv_obj_get_child(lv_marquee->container_bottom, 0);
            if (lv_obj_get_x(first) > APPSTORE_LV_MARQUEE_W) {
                /*计算新位置（顶部容器末尾）*/
                lv_coord_t new_x = APPSTORE_LV_MARQUEE_W;
                if (lv_obj_get_child_cnt(lv_marquee->container_top)) {
                    lv_obj_t *last = lv_obj_get_child(lv_marquee->container_top, 
                        lv_obj_get_child_cnt(lv_marquee->container_top) - 1);
                    new_x = lv_obj_get_x(last) + APPSTORE_LV_MARQUEE_ICON_W + 10;
                }
                lv_obj_set_parent(first, lv_marquee->container_top);
                lv_obj_set_pos(first, new_x, 0);
            }
        }
    }

    void AppStore::get_lv_marquee(struct appstore_lv_marquee_t* lv_marquee, struct appstore_lv_screen_t* lv_screen, void* user_data)
    {
        if(lv_marquee != NULL && lv_screen != NULL){
            /*创建顶部容器（水平布局）*/
            lv_marquee->container_top = lv_obj_create(lv_screen->tile);
            lv_obj_set_size(lv_marquee->container_top, APPSTORE_LV_MARQUEE_W, APPSTORE_LV_MARQUEE_H);
            lv_obj_align(lv_marquee->container_top, LV_ALIGN_CENTER, 0, (-APPSTORE_LV_MARQUEE_ICON_H)/2 - 5);
            lv_obj_set_style_bg_opa(lv_marquee->container_top, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_opa(lv_marquee->container_top, LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(lv_marquee->container_top, 0, 0);
            lv_obj_clear_flag(lv_marquee->container_top, LV_OBJ_FLAG_SCROLLABLE);
            /*创建底部容器（水平布局）*/
            lv_marquee->container_bottom = lv_obj_create(lv_screen->tile);
            lv_obj_set_size(lv_marquee->container_bottom, APPSTORE_LV_MARQUEE_W, APPSTORE_LV_MARQUEE_H);
            lv_obj_align(lv_marquee->container_bottom, LV_ALIGN_CENTER, 0, APPSTORE_LV_MARQUEE_ICON_H/2 + 5);
            lv_obj_set_style_bg_opa(lv_marquee->container_bottom, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_opa(lv_marquee->container_bottom, LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(lv_marquee->container_bottom, 0, 0);
            lv_obj_clear_flag(lv_marquee->container_bottom, LV_OBJ_FLAG_SCROLLABLE);
            /*添加图标到顶部容器*/
            int img_x = 0;
            for(int i = 0; i < marquee_img_count; i++){
                lv_obj_t *img = lv_img_create(lv_marquee->container_top);
                lv_img_set_src(img, marquee_img_srcs[i]);
                lv_obj_set_size(img, APPSTORE_LV_MARQUEE_ICON_W, APPSTORE_LV_MARQUEE_ICON_H);
                lv_obj_set_pos(img, img_x, 0);
                lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_set_style_radius(img, 14, 0);
                lv_obj_set_style_clip_corner(img, true, 0);
                if(lv_screen->icon_cb != NULL)lv_obj_add_event_cb(img, lv_screen->icon_cb, LV_EVENT_ALL, lv_screen->icon_cb_data);
                img_x += APPSTORE_LV_MARQUEE_ICON_W + 10;
            }
            /*创建移动定时器*/
            lv_marquee->timer = lv_timer_create(lv_marquee_timer_cb, 60, lv_marquee); //这里注意，如果这里周期<=30，触发回调，所有图片的X有可能都为0。
            lv_timer_pause(lv_marquee->timer);
        }
    }

    AppStore::AppStore(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* tile, lv_marquee_icon_event_cb_t _icon_cb, void* _icon_cb_data)
    {
        memset(&lv_screen, 0, sizeof(lv_screen));
        memset(&lv_bg, 0, sizeof(lv_bg));
        memset(&lv_marquee, 0, sizeof(lv_marquee));

        lv_screen.screen = screen;
        lv_screen.tileview = tileview;
        lv_screen.tile = tile;
        lv_screen.icon_cb = _icon_cb;
        lv_screen.icon_cb_data = _icon_cb_data;
        ESP_LOGI(TAG, "AppStore on construct");
    }

    AppStore::~AppStore()
    {
        if(lv_marquee.container_top != NULL)lv_obj_del(lv_marquee.container_top);
        if(lv_marquee.container_bottom != NULL)lv_obj_del(lv_marquee.container_bottom);
        if(lv_marquee.timer != NULL)lv_timer_del(lv_marquee.timer);

        if(lv_bg.img != NULL)lv_obj_del(lv_bg.img);
         if(lv_bg.jpeg_src.jpeg_buff != NULL)heap_caps_free(lv_bg.jpeg_src.jpeg_buff);

        ESP_LOGI(TAG, "AppStore on deconstruct");
    }

    void AppStore::onCreate()
    {
        /*获取背景图像控件 */
        get_lv_bg(&lv_bg, &lv_screen, this);
        /*开启解码*/
        fml::JpegDecoder::getInstance().StartJpegDec(lv_bg.cur_jpeg_index, lv_bg.jpeg_src);
        fml::JpegDecoder::getInstance().WaitJpegDec(NULL, portMAX_DELAY);
        /*解码成功，设置控件*/
        set_lv_bg(&lv_bg, lv_bg.jpeg_src.jpeg_buff);
        /*获取选取框控件*/
        get_lv_marquee(&lv_marquee, &lv_screen, this);
        ESP_LOGI(TAG, "AppStore on create");
    }

    void AppStore::onOpen()
    {
        if(lv_marquee.timer != NULL)lv_timer_resume(lv_marquee.timer);
        ESP_LOGI(TAG, "AppStore on Open");
    }

    void AppStore::onRunning()
    {
        int cur_jpeg_index;
        /*刷新背景图*/
        if(fml::JpegDecoder::getInstance().WaitJpegDec(&cur_jpeg_index, 0) == true){
            if(lv_bg.start_jpeg_index <= cur_jpeg_index && cur_jpeg_index <= lv_bg.end_jpeg_index){
                /*设置下一个要解码的index*/
                lv_bg.cur_jpeg_index++;
                if(lv_bg.cur_jpeg_index > lv_bg.end_jpeg_index)lv_bg.cur_jpeg_index = lv_bg.start_jpeg_index;
                /*解码成功，设置控件*/
                set_lv_bg(&lv_bg, lv_bg.jpeg_src.jpeg_buff);
            }else{
                fml::JpegDecoder::getInstance().StartJpegDec(lv_bg.cur_jpeg_index, lv_bg.jpeg_src);
            }
        }

        //ESP_LOGI(TAG, "AppStore on Running");
    }

    void AppStore::onSleeping()
    {
        //ESP_LOGI(TAG, "AppStore on Sleeping");
    }

    void AppStore::onClose()
    {
        if(lv_marquee.timer != NULL)lv_timer_pause(lv_marquee.timer);
        ESP_LOGI(TAG, "AppStore on Close");
    }

    void AppStore::onDestroy()
    {
        ESP_LOGI(TAG, "AppStore on destroy");
    }

}

