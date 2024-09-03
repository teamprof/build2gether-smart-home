/* Copyright 2024 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <iot_button.h>
#include <device.h>

#include "../ArduProfFreeRTOS.h"
#include "../AppEvent.h"

class ButtonBoot
{
public:
    ButtonBoot(ardufreertos::MessageQueue *queue) : _queue(queue),
                                                    _handle(NULL),
                                                    _pin(0)
    {
    }

    bool init(void)
    {
        // https://espressif-docs.readthedocs-hosted.com/projects/espressif-esp-iot-solution/en/latest/input_device/button.html
        button_config_t config = button_driver_get_config();
        button_handle_t handle = iot_button_create(&config);
        iot_button_register_cb(handle, BUTTON_SINGLE_CLICK, [](void *button_handle, void *usr_data)
                               {
                                   auto instance = static_cast<ButtonBoot *>(usr_data);
                                   if (instance && instance->_queue)
                                   {
                                       instance->_queue->postEvent(EventSystem, SysButtonClick, instance->_pin);
                                   }
                                   //
                               },
                               this);
        _handle = handle;
        _pin = config.gpio_button_config.gpio_num;
        return handle != NULL;
    }

    int32_t getPin(void)
    {
        return _pin;
    }

private:
    ardufreertos::MessageQueue *_queue;
    button_handle_t _handle;
    int32_t _pin;
};