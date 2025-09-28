/**
 * @file hdl_config.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <driver/gpio.h>
#include "lvgl.h"

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

/**< Pixel-Led */
#define BUILTIN_LED_GPIO            GPIO_NUM_2

/**< Audio */
#define AUDIO_I2S_MIC_GPIO_WS       GPIO_NUM_38
#define AUDIO_I2S_MIC_GPIO_SCK      GPIO_NUM_39
#define AUDIO_I2S_MIC_GPIO_DIN      GPIO_NUM_40
#define AUDIO_I2S_SPK_GPIO_DOUT     GPIO_NUM_42
#define AUDIO_I2S_SPK_GPIO_BCLK     GPIO_NUM_41
#define AUDIO_I2S_SPK_GPIO_LRCK     GPIO_NUM_48

/**< Button */
#define BOOT_BUTTON_GPIO            GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO       GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO     GPIO_NUM_NC
#define RESET_NVS_BUTTON_GPIO       GPIO_NUM_NC
#define RESET_FACTORY_BUTTON_GPIO   GPIO_NUM_NC

/**< Board I2C Bus */
#define BOARD_I2C_NUM               I2C_NUM_0
#define BOARD_SCL_PIN               GPIO_NUM_5
#define BOARD_SDA_PIN               GPIO_NUM_4
#define BOARD_I2C_FREQ              (400000)

/**< Display P169H002-CTP */
#define DISPLAY_MOSI                GPIO_NUM_47
#define DISPLAY_SCK                 GPIO_NUM_21
#define DISPLAY_DC                  GPIO_NUM_13
#define DISPLAY_CS                  GPIO_NUM_14
#define DISPLAY_REST                GPIO_NUM_NC
#define DISPLAY_WIDTH               240
#define DISPLAY_HEIGHT              280
#define DISPLAY_SWAP_XY             false
#define DISPLAY_MIRROR_X            false
#define DISPLAY_MIRROR_Y            false
#define BACKLIGHT_INVERT            false
#define DISPLAY_OFFSET_X            0
#define DISPLAY_OFFSET_Y            20
#define DISPLAY_BACKLIGHT_PIN       GPIO_NUM_1
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_BACKLIGHT_CHANNEL   0
#define DISPLAY_BACKLIGHT_FREQ      44100
#define DISPLAY_COLORDEPTH          (16 | 0x0100)

/**< Display P169H002-CTP-Touch */
#define DISPLAY_TOUCH_AS_LISTEN_BUTTON 1
#define DISPLAY_TOUCH_I2C_NUM       BOARD_I2C_NUM
#define DISPLAY_TOUCH_I2C_ADDR      (0x15)
#define DISPLAY_TOUCH_SDA           BOARD_SDA_PIN
#define DISPLAY_TOUCH_SCL           BOARD_SCL_PIN
#define DISPLAY_TOUCH_INT           GPIO_NUM_18
#define DISPLAY_TOUCH_FREQ          BOARD_I2C_FREQ

/**< LVGL */
#define LGVL_COLORDEPTH             (LV_COLOR_FORMAT_RGB565)

/**< PMIC */
#define AXP2101_I2C_ADDR            (0x34)
#define AXP2101_I2C_FREQ            BOARD_I2C_FREQ

/**< IMU */
#define LSM_CS                      GPIO_NUM_9
#define LSM_MISO                    GPIO_NUM_10
#define LSM_MOSI                    GPIO_NUM_11
#define LSM_SCK                     GPIO_NUM_12
#define LSM_IQ                      GPIO_NUM_16

