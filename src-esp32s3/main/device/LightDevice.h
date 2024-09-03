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
#include <stddef.h>
#include <stdint.h>
#include <esp_err.h>
#include <esp_matter.h>

class LightDevice
{
public:
    typedef enum _State
    {
        Off = 0,
        ColorWhite = 1,
        ColorYellow = 2,
        ColorRed = 3,
        On = 4,
    } State;

    static void getDefaultConfig(esp_matter::endpoint::extended_color_light::config_t &config);

    LightDevice();
    uint16_t getEndpointID(void);
    LightDevice::State getState(void);
    void setState(LightDevice::State state);
    void clickButtonOn(void);

    esp_err_t init(uint16_t endpoint_id);
    esp_err_t onAttributeUpdate(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val);
    esp_err_t setPower(esp_matter_attr_val_t *val);
    esp_err_t setBrightness(esp_matter_attr_val_t *val);
    esp_err_t setSaturation(esp_matter_attr_val_t *val);
    esp_err_t setTemperature(esp_matter_attr_val_t *val);
    esp_err_t setHue(esp_matter_attr_val_t *val);

private:
    uint16_t _endpointID;
    led_driver_handle_t _handle;

    esp_err_t _matterGetBrightness(esp_matter::node_t *node, esp_matter::endpoint_t *endpoint, esp_matter_attr_val_t *val);
    esp_err_t _matterGetColorMode(esp_matter::node_t *node, esp_matter::endpoint_t *endpoint, esp_matter_attr_val_t *val);
    esp_err_t _matterGetColorTemperature(esp_matter::node_t *node, esp_matter::endpoint_t *endpoint, esp_matter_attr_val_t *val);
    esp_err_t _matterSetColorTemperature(esp_matter::node_t *node, esp_matter::endpoint_t *endpoint, esp_matter_attr_val_t *val);
    esp_err_t _matterGetOnOff(esp_matter::node_t *node, esp_matter::endpoint_t *endpoint, esp_matter_attr_val_t *val);
    esp_err_t _matterSetOnOff(esp_matter::node_t *node, esp_matter::endpoint_t *endpoint, esp_matter_attr_val_t *val);
};
