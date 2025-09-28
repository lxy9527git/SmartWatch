/**
 * @file power.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <driver/i2c.h>
#include <esp_log.h>
#include "hdl_config.hpp"
#include "disp.hpp"

namespace hdl{

    class power
    {
        struct Config_t{
            int16_t pin_scl = -1;
            int16_t pin_sda = -1;
            int16_t pin_int = -1;
            int8_t i2c_port = 0;
            int16_t dev_addr = AXP2101_I2C_ADDR;
            uint32_t freq = AXP2101_I2C_FREQ;
        };
        private:
            const char *TAG = "AXP2101";
            Config_t _cfg;
            uint8_t _data_buffer[2];

            /*私有构造函数，禁止外部直接实例化*/
            power(){}
            /*禁止拷贝构造和赋值操作*/
            power(const power&) = delete;
            power& operator = (const power&) = delete;

            inline bool _writeReg(uint8_t reg, uint8_t data)
            {
                _data_buffer[0] = reg;
                _data_buffer[1] = data;
                return lgfx::i2c::transactionWrite(_cfg.i2c_port, _cfg.dev_addr, _data_buffer, 2, _cfg.freq).has_value();
            }

            inline bool _readReg(uint8_t reg, uint8_t readsize)
            {
                if(readsize > 2)return false;

                return lgfx::i2c::transactionWriteRead(_cfg.i2c_port, _cfg.dev_addr, &reg, 1, _data_buffer, readsize, _cfg.freq).has_value();
            }
        public:
            /*获取单例实例的静态方法*/
            inline static power& getInstance() {
                static power instance;
                return instance;
            }
            inline Config_t config(){return _cfg;}
            inline void config(const Config_t &cfg){_cfg = cfg;}
            inline void setPin(const int &sda, const int &scl, const int& intr)
            {
                _cfg.pin_sda = sda;
                _cfg.pin_scl = scl;
                _cfg.pin_int = intr;
            }

            /* Setup gpio and reset */
            inline void gpioInit(void)
            {
                ESP_LOGD(TAG, "setup int gpio");

                if (_cfg.pin_int > 0) {
                    gpio_reset_pin((gpio_num_t)_cfg.pin_int);
                    gpio_set_direction((gpio_num_t)_cfg.pin_int, GPIO_MODE_INPUT);
                }
            }

            inline bool init(void)
            {
                gpioInit();

                // ** EFUSE defaults **
                _writeReg(0x22, 0b110); // PWRON > OFFLEVEL as POWEROFF Source enable
                _writeReg(0x27, 0x10);  // hold 4s to power off
            
                _writeReg(0x92, 0x1C); // 配置 aldo1 输出为 3.3V

                _readReg(0x90,1);   // XPOWERS_AXP2101_LDO_ONOFF_CTRL0
                uint8_t value = _data_buffer[0] | 0x01;   // set bit 1 (ALDO1)
                _writeReg(0x90, value);  // and power channels now enabled
            
                _writeReg(0x64, 0x03); // CV charger voltage setting to 4.2V
                
                _writeReg(0x61, 0x05); // set Main battery precharge current to 125mA
                _writeReg(0x62, 0x08); // set Main battery charger current to 200mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
                _writeReg(0x63, 0x15); // set Main battery term charge current to 125mA
            
                _writeReg(0x14, 0x00); // set minimum system voltage to 4.1V (default 4.7V), for poor USB cables
                _writeReg(0x15, 0x00); // set input voltage limit to 3.88v, for poor USB cables
                _writeReg(0x16, 0x05); // set input current limit to 2000mA
            
                _writeReg(0x24, 0x01); // set Vsys for PWROFF threshold to 3.2V (default - 2.6V and kill battery)
                _writeReg(0x50, 0x14); // set TS pin to EXTERNAL input (not temperature)

                return true;
            }

            inline bool init(const int &sda, const int &scl, const int &intr = -1)
            {
                setPin(sda, scl, intr);
                return init();
            }

            /* Power key status */
            inline bool isKeyPressed()
            {
                /* IRQ status 1 */
                _readReg(0x49, 1);
                if (_data_buffer[0] & 0b00001000) {
                    /* Clear */
                    _writeReg(0x49, _data_buffer[0]);
                    return true;
                }
                return false;
            }

            inline bool isKeyLongPressed()
            {
                /* IRQ status 1 */
                _readReg(0x49, 1);
                if (_data_buffer[0] & 0b00000100) {
                    /* Clear */
                    _writeReg(0x49, _data_buffer[0]);
                    return true;
                }
                return false;
            }

            /* Charging status */
            inline bool isCharging()
            {
                /* PMU status 2 */
                _readReg(0x01, 1);
                if ((_data_buffer[0] & 0b01100000) == 0b00100000) {
                    return true;
                }
                return false;
            }

            inline bool isChargeDone()
            {
                /* PMU status 2 */
                _readReg(0x01, 1);
                if ((_data_buffer[0] & 0b00000111) == 0b00000100) {
                    return true;
                }
                return false;
            }

            /* Bettery status */
            inline uint8_t batteryLevel()
            {
                /* Battery percentage data */
                _readReg(0xA4, 1);
                return _data_buffer[0];
            }

            /* Power control */
            inline void powerOff()
            {
                /* PMU common configuration */
                _readReg(0x10, 1);
                /* Soft power off */
                _writeReg(0x10, (_data_buffer[0] | 0b00000001));
            }

            /* Power Reset */
            inline void powerReset()
            {
                /* PMU common configuration */
                _readReg(0x10, 1);
                /* Soft power off */
                _writeReg(0x10, (_data_buffer[0] | 0b00000010));
            }
    };
}

