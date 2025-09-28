/**
 * @file WatchDial.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "WatchDial.hpp"

namespace apl{

    void WatchDial::tileview_event_cb(lv_event_t * e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        WatchDial* app = (WatchDial*)lv_event_get_user_data(e);
        /*滚动事件处理（同步状态栏动画）*/
        if (code == LV_EVENT_SCROLL) {
            lv_coord_t screen_width = lv_disp_get_hor_res(NULL);
            lv_coord_t scroll_x = lv_obj_get_scroll_x(app->lv_screen.tileview);
            /*同步状态栏的位置（仅水平方向）*/
            if(scroll_x <= screen_width)lv_obj_set_x(app->lv_status.container, screen_width - scroll_x);
            else lv_obj_set_x(app->lv_status.container, 0);
        }
    }

    void WatchDial::bg_img_event_cb(lv_event_t* e)
    {
        static int old_jpeg_index = -1;
        if(lv_event_get_code(e) == LV_EVENT_DRAW_POST_END){
            if(old_jpeg_index != ((WatchDial*)lv_event_get_user_data(e))->lv_bg.cur_jpeg_index){
                /*图片已经渲染到屏幕*/
                fml::JpegDecoder::getInstance().StartJpegDec(((WatchDial*)lv_event_get_user_data(e))->lv_bg.cur_jpeg_index, ((WatchDial*)lv_event_get_user_data(e))->lv_bg.jpeg_src);
                old_jpeg_index = ((WatchDial*)lv_event_get_user_data(e))->lv_bg.cur_jpeg_index;
            }
        }
    }

    void WatchDial::get_lv_bg(struct watchdial_lv_bg_t* lv_bg, struct watchdial_lv_screen_t* lv_screen, void* arg)
    {
        if(lv_bg != NULL && lv_screen != NULL){
            fml::JpegDecoder::FindJpegMaterial(BLL_JPEG_DESC_PATH,WATCHDIAL_TAG_NAME,&lv_bg->start_jpeg_index, &lv_bg->end_jpeg_index);
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

    void WatchDial::set_lv_bg(struct watchdial_lv_bg_t* lv_bg, uint8_t* data)
    {
        if(lv_bg != NULL && data != NULL){
            lv_bg->img_dsc.data = data;
            lv_img_set_src(lv_bg->img, &lv_bg->img_dsc);  
            lv_obj_align(lv_bg->img, LV_ALIGN_CENTER, 0, 0);
        }
    }

    void WatchDial::get_lv_status(struct watchdial_lv_status_t* lv_status, struct watchdial_lv_screen_t* lv_screen)
    {
        if(lv_status != NULL && lv_screen != NULL){
            lv_status->container = lv_obj_create(lv_screen->screen);
            lv_obj_set_size(lv_status->container, LV_PCT(100), WATCHDIAL_LV_STATUS_H);
            lv_obj_align(lv_status->container, LV_ALIGN_TOP_MID, 0, 2);
            lv_obj_clear_flag(lv_status->container, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_pad_all(lv_status->container, 5, 0);
            lv_obj_set_style_bg_opa(lv_status->container, LV_OPA_TRANSP,0);
            lv_obj_set_style_border_width(lv_status->container, 0, 0);
            lv_obj_set_x(lv_status->container, LV_PCT(100));

            lv_status->time_hm_lable = lv_label_create(lv_status->container);
            lv_obj_set_style_text_color(lv_status->time_hm_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_status->time_hm_lable, &lv_font_montserrat_16, 0);
            lv_obj_align(lv_status->time_hm_lable, LV_ALIGN_LEFT_MID, 10, 0);

            lv_status->bat_font_lable = lv_label_create(lv_status->container);
            lv_obj_set_style_text_color(lv_status->bat_font_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_status->bat_font_lable, &lv_font_montserrat_16, 0);
            lv_label_set_text_fmt(lv_status->bat_font_lable, "%d%%", 0);
            lv_obj_align(lv_status->bat_font_lable, LV_ALIGN_RIGHT_MID, -10, 0);

            lv_status->bat_icon_lable = lv_label_create(lv_status->container);
            lv_obj_set_style_text_color(lv_status->bat_icon_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_status->bat_icon_lable, &lv_font_montserrat_16, 0);
            lv_label_set_text(lv_status->bat_icon_lable, LV_SYMBOL_BATTERY_EMPTY);
            lv_obj_align(lv_status->bat_icon_lable, LV_ALIGN_RIGHT_MID, -55, 0);

            lv_status->bluetooth_lable = lv_label_create(lv_status->container);
            lv_obj_set_style_text_color(lv_status->bluetooth_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_status->bluetooth_lable, &lv_font_montserrat_16, 0);
            lv_label_set_text(lv_status->bluetooth_lable, LV_SYMBOL_BLUETOOTH);
            lv_obj_align(lv_status->bluetooth_lable, LV_ALIGN_RIGHT_MID, -85, 0);

            lv_status->wifi_lable = lv_label_create(lv_status->container);
            lv_obj_set_style_text_color(lv_status->wifi_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_status->wifi_lable, &lv_font_montserrat_16, 0);
            lv_label_set_text(lv_status->wifi_lable, LV_SYMBOL_WIFI);
            lv_obj_align(lv_status->wifi_lable, LV_ALIGN_RIGHT_MID, -105, 0);

        }
    }

    void WatchDial::set_lv_status_time(struct watchdial_lv_status_t* lv_status, struct tm* tm)
    {
        if(lv_status != NULL && tm != NULL){
            lv_label_set_text_fmt(lv_status->time_hm_lable, "%02d:%02d", tm->tm_hour, tm->tm_min);
        }
    }

    void WatchDial::set_lv_status_bat(struct watchdial_lv_status_t* lv_status, int percent, bool is_charge)
    {
        if(lv_status != NULL){
            /*刷新电量图标*/
            if(percent >= 100){
                lv_label_set_text(lv_status->bat_icon_lable, LV_SYMBOL_BATTERY_FULL); 
            }else if(percent>= 70){
                lv_label_set_text(lv_status->bat_icon_lable, LV_SYMBOL_BATTERY_3); 
            }else if(percent >= 40){
                lv_label_set_text(lv_status->bat_icon_lable, LV_SYMBOL_BATTERY_2); 
            }else if(percent >= 10){
                lv_label_set_text(lv_status->bat_icon_lable, LV_SYMBOL_BATTERY_1); 
            }else if(percent >= 0){
                lv_label_set_text(lv_status->bat_icon_lable, LV_SYMBOL_BATTERY_EMPTY); 
            }
            /*刷新充电图标*/
            if(is_charge){
                lv_obj_set_style_text_color(lv_status->bat_icon_lable, lv_color_hex(0x00ff00), 0);
                lv_obj_set_style_text_color(lv_status->bat_font_lable, lv_color_hex(0x00ff00), 0);
            }else{
                lv_obj_set_style_text_color(lv_status->bat_icon_lable, lv_color_hex(0xffffff), 0);
                lv_obj_set_style_text_color(lv_status->bat_font_lable, lv_color_hex(0xffffff), 0);
            }
            /*修改电池百分比*/
            lv_label_set_text_fmt(lv_status->bat_font_lable, "%d%%", percent);
        }
    }

    void WatchDial::set_lv_status_bluetooth(struct watchdial_lv_status_t* lv_status, bool is_bluetooth)
    {
        if(lv_status != NULL){
            if(is_bluetooth){
                lv_obj_set_style_text_color(lv_status->bluetooth_lable, lv_color_hex(0x00ff00), 0);
            }else{
                lv_obj_set_style_text_color(lv_status->bluetooth_lable, lv_color_hex(0xffffff), 0);
            }
        }
    }

    void WatchDial::set_lv_status_wifi(struct watchdial_lv_status_t* lv_status, bool is_wifi)
    {
        if(lv_status != NULL){
            if(is_wifi){
                lv_obj_set_style_text_color(lv_status->wifi_lable, lv_color_hex(0x00ff00), 0);
            }else{
                lv_obj_set_style_text_color(lv_status->wifi_lable, lv_color_hex(0xffffff), 0);
            }
        }
    }

    void WatchDial::get_lv_time(struct watchdial_lv_time_t* lv_time, struct watchdial_lv_screen_t* lv_screen)
    {
        if(lv_time != NULL && lv_screen != NULL){
            lv_time->hm_lable = lv_label_create(lv_screen->tile);
            lv_obj_set_pos(lv_time->hm_lable, WATCHDIAL_LV_TIME_HM_X, WATCHDIAL_LV_TIME_HM_Y);
            lv_obj_set_size(lv_time->hm_lable, WATCHDAIL_LV_TIME_HM_W, WATCHDIAL_LV_TIME_HM_H);
            lv_obj_set_style_text_color(lv_time->hm_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_time->hm_lable, &lv_font_ArchitectsDaughter_50, 0);
            lv_obj_set_style_text_align(lv_time->hm_lable, LV_TEXT_ALIGN_RIGHT, 0);

            lv_time->mw_lable = lv_label_create(lv_screen->tile);
            lv_obj_set_pos(lv_time->mw_lable, WATCHDIAL_LV_TIME_MW_X, WATCHDIAL_LV_TIME_MW_Y);
            lv_obj_set_size(lv_time->mw_lable, WATCHDAIL_LV_TIME_MW_W, WATCHDIAL_LV_TIME_MW_H);
            lv_obj_set_style_text_color(lv_time->mw_lable, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_text_font(lv_time->mw_lable, &lv_font_ArchitectsDaughter_25, 0);
            lv_obj_set_style_text_align(lv_time->mw_lable, LV_TEXT_ALIGN_RIGHT, 0);
        }
    }

    void WatchDial::set_lv_time(struct watchdial_lv_time_t* lv_time, struct tm* tm)
    {
        if(lv_time != NULL && tm!= NULL){
            lv_label_set_text_fmt(lv_time->hm_lable, "%02d:%02d", tm->tm_hour, tm->tm_min);
            lv_label_set_text_fmt(lv_time->mw_lable, "%d/%d %s", tm->tm_mon + 1, tm->tm_mday, [](struct tm* _tm)->const char*{
                switch(_tm->tm_wday){
                    case 0: return "Sun";
                    case 1: return "Mon";
                    case 2: return "Tue";
                    case 3: return "Wed";
                    case 4: return "Thu";
                    case 5: return "Fri";
                    case 6: return "Sat";
                    default: return "???";
                }
            }(tm));
        }
    }

    WatchDial::WatchDial(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* tile)
    {
        memset(&lv_screen, 0, sizeof(lv_screen));
        memset(&lv_bg, 0, sizeof(lv_bg));
        memset(&lv_status, 0, sizeof(lv_status));
        memset(&lv_time, 0, sizeof(lv_time));

        lv_screen.screen = screen;
        lv_screen.tileview = tileview;
        lv_screen.tile = tile;
        lv_obj_add_event_cb(lv_screen.tileview, tileview_event_cb, LV_EVENT_ALL, this);

        ESP_LOGI(TAG, "WatchDial on construct");
    }

    WatchDial::~WatchDial()
    {
        if(lv_time.hm_lable != NULL)lv_obj_del(lv_time.hm_lable);
        if(lv_time.mw_lable != NULL)lv_obj_del(lv_time.mw_lable);

        /*自动递归删除其子对象*/
        if(lv_status.container != NULL)lv_obj_del(lv_status.container);

        if(lv_bg.img != NULL)lv_obj_del(lv_bg.img);
        if(lv_bg.jpeg_src.jpeg_buff != NULL)heap_caps_free(lv_bg.jpeg_src.jpeg_buff);

        ESP_LOGI(TAG, "WatchDial on deconstruct");
    }

    void WatchDial::onCreate()
    {
        /*获取背景图像控件 */
        get_lv_bg(&lv_bg, &lv_screen, this);
        /*获取状态栏控件*/
        get_lv_status(&lv_status, &lv_screen);
        /*获取时间控件*/
        get_lv_time(&lv_time, &lv_screen);
        /*开启解码*/
        fml::JpegDecoder::getInstance().StartJpegDec(lv_bg.cur_jpeg_index, lv_bg.jpeg_src);
        fml::JpegDecoder::getInstance().WaitJpegDec(NULL, portMAX_DELAY);
        /*解码成功，设置控件*/
        set_lv_bg(&lv_bg, lv_bg.jpeg_src.jpeg_buff);
        /*设置为主界面*/
        lv_tileview_set_tile(lv_screen.tileview,lv_screen.tile,LV_ANIM_OFF);
        ESP_LOGI(TAG, "WatchDial on create");
    }

    void WatchDial::onShow()
    {
        ESP_LOGI(TAG, "WatchDial on show");
    }

    void WatchDial::onForeground()
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
        /*更新电量状态*/
        set_lv_status_bat(&lv_status, HdlManager::getInstance().get_battery_level(), HdlManager::getInstance().get_battery_is_charging());
        /*更新时间状态*/
        struct tm cur_tm = HdlManager::getInstance().get_rtc_time();
        set_lv_time(&lv_time, &cur_tm);
        set_lv_status_time(&lv_status, &cur_tm);
        /*更新wifi状态*/
        set_lv_status_wifi(&lv_status, fml::HdlManager::getInstance().wifi_station_is_connected());
        /*更新bt状态*/
        set_lv_status_bluetooth(&lv_status, fml::HdlManager::getInstance().bt_is_connected());

        //ESP_LOGI(TAG, "WatchDialApp on foreground");
    }

    void WatchDial::onBackground()
    {
        /*更新电量状态*/
        set_lv_status_bat(&lv_status, HdlManager::getInstance().get_battery_level(), HdlManager::getInstance().get_battery_is_charging());
        /*更新时间状态*/
        struct tm cur_tm = HdlManager::getInstance().get_rtc_time();
        set_lv_time(&lv_time, &cur_tm);
        set_lv_status_time(&lv_status, &cur_tm);
        /*更新wifi状态*/
        set_lv_status_wifi(&lv_status, fml::HdlManager::getInstance().wifi_station_is_connected());
        /*更新bt状态*/
        set_lv_status_bluetooth(&lv_status, fml::HdlManager::getInstance().bt_is_connected());
        
        //ESP_LOGI(TAG, "WatchDial on background");
    }

    void WatchDial::onHide()
    {
        ESP_LOGI(TAG, "WatchDial on hide");
    }

    void WatchDial::onDestroy()
    {
        ESP_LOGI(TAG, "WatchDial on destroy");
    }

}




