/**
 * @file button.hpp
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
#include <esp_timer.h>

namespace hdl{

    class button
    {
        #define millis()                        (esp_timer_get_time() / 1000)
        #define digitalRead(pin)                (bool)gpio_get_level(pin)
        private:
            gpio_num_t  _pin;
            uint16_t _delay;
            bool     _state;
            uint32_t _ignore_until;
            bool     _has_changed;

            /*私有构造函数，禁止外部直接实例化*/
            button(){}
            /*禁止拷贝构造和赋值操作*/
            button(const button&) = delete;
            button& operator = (const button&) = delete;
        public:
            const static bool PRESSED = 0;
            const static bool RELEASED = 1;

            /*获取单例实例的静态方法*/
            inline static button& getInstance() {
                static button instance;
                return instance;
            }

            inline void init(gpio_num_t pin, uint16_t debounce_ms = 100)
            {
                _pin = pin;
                _delay = debounce_ms;
                _state = 1;
                _ignore_until = 0;
                _has_changed = false;
            }
            inline void begin()
            {
                gpio_reset_pin(_pin);
                gpio_set_direction(_pin, GPIO_MODE_INPUT);
                gpio_set_pull_mode(_pin, GPIO_PULLUP_ONLY);
            }
            inline bool read()
            {
                /*ignore pin changes until after this delay time*/
                if (_ignore_until > millis())
                {
                    /*ignore any changes during this period*/
                }
                
                /*pin has changed */
                else if (digitalRead(_pin) != _state)
                {
                    _ignore_until = millis() + _delay;
                    _state = !_state;
                    _has_changed = true;
                }
                
                return _state;
            }
            inline bool toggled()
            {
                /* has the button been toggled from on -> off, or vice versa*/
                read();
	            return has_changed();
            }
            inline bool pressed()
            {
                /*has the button gone from off -> on*/
                return (read() == PRESSED && has_changed());
            }
            inline bool released()
            {
                /*has the button gone from on -> off*/
                return (read() == RELEASED && has_changed());
            }
            inline bool has_changed()
            {
                /*mostly internal, tells you if a button has changed after calling the read() function*/
                if (_has_changed)
                {
                    _has_changed = false;
                    return true;
                }
                return false;
            }
            inline static void setwakeup(gpio_num_t pin, uint8_t trigger_level)
            {
                gpio_reset_pin(pin);
                gpio_set_direction(pin, GPIO_MODE_INPUT);
                if(trigger_level == 1){
                    gpio_sleep_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
                    gpio_wakeup_enable(pin, GPIO_INTR_HIGH_LEVEL);
                }else{
                    gpio_sleep_set_pull_mode(pin, GPIO_PULLUP_ONLY);
                    gpio_wakeup_enable(pin, GPIO_INTR_LOW_LEVEL);
                }
            }
    };
}