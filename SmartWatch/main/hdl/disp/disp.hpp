/**
 * @file disp.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *` 
 */
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "hdl_config.hpp"

namespace hdl{

    class disp : public lgfx::LGFX_Device
    {
        #define DISP_DEFAULT_BRIGHTNESS_VOLUME                             (200)

        private:
            lgfx::Panel_ST7789P3    _panel_instance;
            lgfx::Bus_SPI           _bus_instance;
            lgfx::Light_PWM         _light_instance;
            lgfx::Touch_CST816S     _touch_instance;

            /*私有构造函数，禁止外部直接实例化*/
            disp()
            {
                {
                    /*获取用于配置总线的结构*/
                    auto cfg = _bus_instance.config();    
                    /*SPI设置（ESP32默认使用VSPI）选择要使用的SPI  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST*/
                    cfg.spi_host = SPI2_HOST;
                    /*随着ESP-IDF版本的升级，vspi_host， hspi_host的描述将不再推荐，所以如果出现错误，请用spi2_host，请使用spi3_host。SPI模式（0~3）*/
                    cfg.spi_mode = 3;
                    /*发射时SPI时钟（高达80mhz，舍入80mhz除以整数）*/
                    cfg.freq_write = 80 * 1000 * 1000;
                    /*接收SPI时钟*/
                    cfg.freq_read  = 16000000;
                    /*如果通过MOSI引脚接收，则设置true*/
                    cfg.spi_3wire  = true;
                    /*如果要使用事务锁，则设置true*/
                    cfg.use_lock   = true;
                    /*设定要使用的DMA信道（0=不使用DMA / 1=1ch / 2=ch / spi_cdma_ch_auto =自动设定）*/
                    cfg.dma_channel = SPI_DMA_CH_AUTO;
                    /*随著ESP-IDF版本的升级，DMA频道推荐使用spi_cdma_ch_auto（自动设定）。不推荐指定1ch和2ch。设置SPI的SCLK引脚编号*/
                    cfg.pin_sclk = DISPLAY_SCK;
                    /*设置SPI的MOSI引脚编号*/
                    cfg.pin_mosi = DISPLAY_MOSI;
                    /*设置SPI的MISO引脚编号（-1 = disable）*/
                    cfg.pin_miso = GPIO_NUM_NC;
                    /*设置SPI的D/C引脚编号（-1 = disable）*/
                    cfg.pin_dc   = DISPLAY_DC;
                    /*将设定值反映在总线上。*/
                    _bus_instance.config(cfg); 
                    /*将总线设置在面板上。*/
                    _panel_instance.setBus(&_bus_instance); 
                }

                {
                    /*设置显示面板控制。获取用于显示面板设置的结构。*/
                    auto cfg = _panel_instance.config();
                    /*CS所连接的引脚编号（-1 = disable）*/
                    cfg.pin_cs           =    DISPLAY_CS;
                    /*RST所连接的引脚编号（-1 = disable）*/
                    cfg.pin_rst          =    DISPLAY_REST; 
                    /*BUSY所连接的引脚编号（-1 = disable）*/
                    cfg.pin_busy         =    GPIO_NUM_NC;
                    /*下面的设置值是每个面板的通用初始值，所以请尝试评论不清楚的项目。实际可显示宽度*/
                    cfg.panel_width      =    DISPLAY_WIDTH;
                    /*实际可显示的高度*/
                    cfg.panel_height     =    DISPLAY_HEIGHT; 
                    /*面板的X方向偏移量*/
                    cfg.offset_x         =    DISPLAY_OFFSET_X;
                    /*面板Y方向偏移量*/
                    cfg.offset_y         =    DISPLAY_OFFSET_Y;
                    /*旋转方向的值偏移0到7（4到7是上下反转）*/
                    cfg.offset_rotation  =     0; 
                    /*像素读取之前的伪读取比特数*/
                    cfg.dummy_read_pixel =     16;
                    /*非像素数据读取之前的伪读取比特数*/
                    cfg.dummy_read_bits  =     1;
                    /*如果数据读取可用，则设置为true*/
                    cfg.readable         =  false;
                    /*如果面板的明暗反转，则设置为true*/
                    cfg.invert           = true;
                    /*如果面板的红色和蓝色被替换，则设置为true*/
                    cfg.rgb_order        = true;
                    /*对于以16bit为单位发送数据长度的16bit并行或SPI面板，设置为true*/
                    cfg.dlen_16bit       = false;
                    /*SD卡和总线共享时设置为true（使用drawJpgFile等进行总线控制）*/
                    cfg.bus_shared       =  false;
                    /*请仅在使用ST7735或ILI9163等可改变像素数量的驱动器时设置以下设置。驱动IC支持的最大宽度*/
                    //cfg.memory_width     =   240;
                    /*驱动IC支持的最大高度*/
                    //cfg.memory_height    =   320;
                    _panel_instance.config(cfg);
                }

                {
                    /*设置背光控制。（如有必要删除）获取用于背光设置的结构。*/
                    auto cfg = _light_instance.config();
                    /*背光连接的引脚编号*/
                    cfg.pin_bl = DISPLAY_BACKLIGHT_PIN; 
                    /*如果反转背光亮度true*/
                    cfg.invert = DISPLAY_BACKLIGHT_OUTPUT_INVERT;
                    /*背光PWM频率*/
                    cfg.freq   = DISPLAY_BACKLIGHT_FREQ;
                    /*要使用的PWM频道编号*/
                    cfg.pwm_channel = DISPLAY_BACKLIGHT_CHANNEL;
                    _light_instance.config(cfg);
                    /*将背光设置在面板上。*/
                    _panel_instance.setLight(&_light_instance);
                }

                {
                    // 设置触摸屏控制。（如有必要删除）
                    auto cfg = _touch_instance.config();
                    /*从触摸屏获得的最小X值（原始值）*/
                    cfg.x_min      = 0;
                    /*从触摸屏获得的最大X值（原始值）*/
                    cfg.x_max      = DISPLAY_WIDTH;
                    /*从触摸屏获得的最小Y值（原始值）*/
                    cfg.y_min      = 0;
                    /*从触摸屏获得的最大Y值（原始值）*/
                    cfg.y_max      = DISPLAY_HEIGHT;
                    /*INT所连接的引脚编号*/
                    cfg.pin_int    = DISPLAY_TOUCH_INT;
                    /*设置true，如果您使用屏幕和公共总线*/
                    cfg.bus_shared = false;
                    /*显示和触摸方向不匹配时的调整设置0到7的值*/
                    cfg.offset_rotation = 0;
                    /*选择要使用的I2C（0或1）*/
                    cfg.i2c_port = DISPLAY_TOUCH_I2C_NUM; 
                    /*I2C设备地址号码*/
                    cfg.i2c_addr = DISPLAY_TOUCH_I2C_ADDR; 
                    /*SDA所连接的引脚编号*/
                    cfg.pin_sda  = DISPLAY_TOUCH_SDA;
                    /*SCL所连接的引脚编号*/
                    cfg.pin_scl  = DISPLAY_TOUCH_SCL;
                    /*I2C时钟设置*/
                    cfg.freq = DISPLAY_TOUCH_FREQ;
                    _touch_instance.config(cfg);
                    /*将触摸屏设置在面板上。*/
                    _panel_instance.setTouch(&_touch_instance);
                }
                /*设置要使用的面板。*/
                setPanel(&_panel_instance);
            }
            /*禁止拷贝构造和赋值操作*/
            disp(const disp&) = delete;
            disp& operator = (const disp&) = delete;
        public:
            lgfx::touch_point_t     _tpp;

            /*获取单例实例的静态方法*/
            inline static disp& getInstance() {
                static disp instance;
                return instance;
            }
    };
}