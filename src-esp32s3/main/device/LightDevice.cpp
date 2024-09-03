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
#include <device.h>
#include <esp_matter.h>
#include <led_driver.h>

#include "../ArduProfFreeRTOS.h"
#include "LightDevice.h"

#ifndef CONFIG_BSP_LED_RGB_GPIO
#define CONFIG_BSP_LED_RGB_GPIO 8 // for SEEED XIAO ESP32S3 Sense
#endif

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

///////////////////////////////////////////////////////////////////////////////
#define COLOR_TEMPERATURE_RED 1700
#define COLOR_TEMPERATURE_YELLOW 520
#define COLOR_TEMPERATURE_WHITE 160

// Standard max values (used for remapping attributes)
#define STANDARD_BRIGHTNESS 100
#define STANDARD_HUE 360
#define STANDARD_SATURATION 100
#define STANDARD_TEMPERATURE_FACTOR 1000000

// Matter max values (used for remapping attributes)
#define MATTER_BRIGHTNESS 254
#define MATTER_HUE 254
#define MATTER_SATURATION 254
#define MATTER_TEMPERATURE_FACTOR 1000000

// Default attribute values used during initialization
#define DEFAULT_POWER true
#define DEFAULT_BRIGHTNESS 64
#define DEFAULT_HUE 128
#define DEFAULT_SATURATION 254

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
static const char *TAG = "LightDevice";

///////////////////////////////////////////////////////////////////////////////
LightDevice::LightDevice() : _endpointID(0)
{
}

uint16_t LightDevice::getEndpointID(void)
{
    return _endpointID;
}

LightDevice::State LightDevice::getState(void)
{
    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, _endpointID);
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    _matterGetOnOff(node, endpoint, &val);
    ESP_LOGI(TAG, "%s: _matterGetOnOff() returns val.type=%u, val.val.b=%u", __func__, val.type, val.val.b);
    if (!val.val.b)
    {
        ESP_LOGI(TAG, "%s: State::Off", __func__);
        return State::Off;
    }

    _matterGetColorTemperature(node, endpoint, &val);

    uint32_t value = val.val.u16;
    ESP_LOGI(TAG, "%s: val.type=%u, val.val.u16=%u, value=%lu", __func__, val.type, val.val.u16, value);
    if (value < ((COLOR_TEMPERATURE_YELLOW + COLOR_TEMPERATURE_WHITE) / 2))
    {
        ESP_LOGI(TAG, "%s: State::ColorWhite", __func__);
        return State::ColorWhite;
    }
    else if (value < ((COLOR_TEMPERATURE_RED + COLOR_TEMPERATURE_YELLOW) / 2))
    {
        ESP_LOGI(TAG, "%s: State::ColorYellow", __func__);
        return State::ColorYellow;
    }
    else
    {
        ESP_LOGI(TAG, "%s: State::ColorRed", __func__);
        return State::ColorRed;
    }
}

void LightDevice::setState(LightDevice::State state)
{
    node_t *node = node::get();
    endpoint_t *endpoint = endpoint::get(node, _endpointID);
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);

    switch (state)
    {
    case On:
        val.type = ESP_MATTER_VAL_TYPE_BOOLEAN;
        val.val.b = true;
        _matterSetOnOff(node, endpoint, &val);
        setPower(&val);
        break;

    case ColorWhite:
        val.type = ESP_MATTER_VAL_TYPE_UINT16;
        val.val.u16 = COLOR_TEMPERATURE_WHITE;
        _matterSetColorTemperature(node, endpoint, &val);
        setTemperature(&val);

        val.type = ESP_MATTER_VAL_TYPE_BOOLEAN;
        val.val.b = true;
        _matterSetOnOff(node, endpoint, &val);
        setPower(&val);
        break;

    case ColorYellow:
        val.type = ESP_MATTER_VAL_TYPE_UINT16;
        val.val.u16 = COLOR_TEMPERATURE_YELLOW;
        _matterSetColorTemperature(node, endpoint, &val);
        setTemperature(&val);

        val.type = ESP_MATTER_VAL_TYPE_BOOLEAN;
        val.val.b = true;
        _matterSetOnOff(node, endpoint, &val);
        setPower(&val);
        break;

    case ColorRed:
        val.type = ESP_MATTER_VAL_TYPE_UINT16;
        val.val.u16 = COLOR_TEMPERATURE_RED;
        _matterSetColorTemperature(node, endpoint, &val);
        setTemperature(&val);

        val.type = ESP_MATTER_VAL_TYPE_BOOLEAN;
        val.val.b = true;
        _matterSetOnOff(node, endpoint, &val);
        setPower(&val);
        break;

    case Off:
    default:
        val.type = ESP_MATTER_VAL_TYPE_BOOLEAN;
        val.val.b = false;
        _matterSetOnOff(node, endpoint, &val);
        setPower(&val);
        break;
    }

    _matterGetColorTemperature(node, endpoint, &val);
}

void LightDevice::clickButtonOn(void)
{
    auto state = getState();
    ESP_LOGI(TAG, "%s: last state=%d", __func__, state);
    switch (state)
    {
    case Off:
        state = ColorWhite;
        break;

    case ColorWhite:
        state = ColorYellow;
        break;

    case ColorYellow:
        state = ColorRed;
        break;

    case ColorRed:
    case On:
    default:
        state = Off;
        break;
    }
    ESP_LOGI(TAG, "%s: new state=%d", __func__, state);
    setState(state);
}

esp_err_t LightDevice::init(uint16_t endpoint_id)
{
    esp_err_t err = ESP_OK;

    led_driver_config_t config = {
        .gpio = CONFIG_BSP_LED_RGB_GPIO,
        .channel = 0,
    };
    led_driver_handle_t handle = led_driver_init(&config);
    ESP_LOGI(TAG, "led_driver_config_t.gpio=%d, handle=%p", config.gpio, handle);

    _endpointID = endpoint_id;
    _handle = handle;

    return err;
}

esp_err_t LightDevice::onAttributeUpdate(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val)
{
    // ESP_LOGI(TAG, "%s: cluster_id=%lu, attribute_id=%lu, val=%p", __func__, cluster_id, attribute_id, val);

    esp_err_t err = ESP_OK;
    switch (cluster_id)
    {
    case Identify::Id:
        ESP_LOGI(TAG, "%s: Clusters::Identify - attribute_id=%lu, val.type=%u, val.val.u8=%u", __func__, attribute_id, val->type, val->val.u8);
        break;
    case Groups::Id:
    {
        ESP_LOGI(TAG, "%s: Clusters::Groups - attribute_id=%lu, val.type=%u", __func__, attribute_id, val->type);
        break;
    }
    case OnOff::Id:
    {
        if (attribute_id == OnOff::Attributes::OnOff::Id)
        {
            ESP_LOGI(TAG, "%s: Clusters::OnOff - Attributes::OnOff, val.type=%u", __func__, val->type);
            err |= setPower(val);
        }
        else
        {
            ESP_LOGW(TAG, "%s: Clusters::OnOff - unsupported attribute_id=%lu", __func__, attribute_id);
        }
        break;
    }
    case LevelControl::Id:
    {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id)
        {
            ESP_LOGI(TAG, "%s: Clusters::LevelControl - Attributes::CurrentLevel, val.type=ESP_MATTER_VAL_TYPE_NULLABLE_UINT8 (%u)", __func__, val->type);
            err |= setBrightness(val);
        }
        else
        {
            ESP_LOGW(TAG, "%s: Clusters::LevelControl - unsupported attribute_id=%lu", __func__, attribute_id);
        }
        break;
    }
    case ColorControl::Id:
    {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id)
        {
            ESP_LOGI(TAG, "%s: Clusters::ColorControl - Attributes::CurrentHue, val.type=%u", __func__, val->type);
            err |= setHue(val);
        }
        else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id)
        {
            ESP_LOGI(TAG, "%s: Clusters::ColorControl - Attributes::CurrentSaturation, val.type=%u", __func__, val->type);
            err |= setSaturation(val);
        }
        else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id)
        {
            ESP_LOGI(TAG, "%s: Clusters::ColorControl - Attributes::ColorTemperatureMireds, val.type=%u", __func__, val->type);
            err |= setTemperature(val);
        }
        else
        {
            ESP_LOGW(TAG, "%s: Clusters::ColorControl - unsupported attribute_id=%lu", __func__, attribute_id);
        }
        break;
    }
    default:
    {
        ESP_LOGW(TAG, "%s: unsupported cluster_id=0x%04lx (%lu)", __func__, cluster_id, cluster_id);
        break;
    }
    }

    if (err)
    {
        ESP_LOGW(TAG, "%s: err = %d", __func__, err);
    }
    return err;
}

void LightDevice::getDefaultConfig(extended_color_light::config_t &config)
{
    // Hue is a degree on the color wheel from 0 to 360. 0 is red, 120 is green, and 240 is blue.
    // Saturation is a percentage value. 0% means a shade of gray, and 100% is the full color.

    config.on_off.on_off = DEFAULT_POWER;
    config.on_off.lighting.start_up_on_off = nullptr;
    config.level_control.current_level = DEFAULT_BRIGHTNESS;
    config.level_control.lighting.start_up_current_level = DEFAULT_BRIGHTNESS;
    config.color_control.color_mode = EMBER_ZCL_COLOR_MODE_COLOR_TEMPERATURE;
    config.color_control.enhanced_color_mode = EMBER_ZCL_ENHANCED_COLOR_MODE_COLOR_TEMPERATURE;
    config.color_control.color_temperature.startup_color_temperature_mireds = nullptr;
}

esp_err_t LightDevice::setPower(esp_matter_attr_val_t *val)
{
    ESP_LOGI(TAG, "%s: val->val.b=%u", __func__, val->val.b);
    return led_driver_set_power(_handle, val->val.b);
}

esp_err_t LightDevice::setBrightness(esp_matter_attr_val_t *val)
{
    if (val->type == ESP_MATTER_VAL_TYPE_NULLABLE_UINT8)
    {
        // ESP_LOGW(TAG, "%s: val->val.p=%p, val->val.u8=%u", __func__, val->val.p, val->val.u8);
        int value = REMAP_TO_RANGE(val->val.u8, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
        ESP_LOGI(TAG, "%s: val->val.u8=%u -> value=%d, MATTER_BRIGHTNESS=%u, STANDARD_BRIGHTNESS=%u",
                 __func__, val->val.u8, value, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
        return led_driver_set_brightness(_handle, value);
    }
    else
    {
        ESP_LOGW(TAG, "%s: unsupported val->type=%u", __func__, val->type);
        return ESP_FAIL;
    }
}

esp_err_t LightDevice::setSaturation(esp_matter_attr_val_t *val)
{
    int value = REMAP_TO_RANGE(val->val.u8, MATTER_SATURATION, STANDARD_SATURATION);
    ESP_LOGI(TAG, "%s: value=%d, val->val.u8=%u, MATTER_SATURATION=%u, STANDARD_SATURATION=%u", __func__, value, val->val.u8, MATTER_SATURATION, STANDARD_SATURATION);
    return led_driver_set_saturation(_handle, value);
}

esp_err_t LightDevice::setTemperature(esp_matter_attr_val_t *val)
{
    uint32_t value = REMAP_TO_RANGE_INVERSE(val->val.u16, STANDARD_TEMPERATURE_FACTOR);
    ESP_LOGI(TAG, "%s: value=%lu, val->val.u16=%u, STANDARD_TEMPERATURE_FACTOR=%u", __func__, value, val->val.u16, STANDARD_TEMPERATURE_FACTOR);
    return led_driver_set_temperature(_handle, value);
}

esp_err_t LightDevice::setHue(esp_matter_attr_val_t *val)
{
    int value = REMAP_TO_RANGE(val->val.u8, MATTER_HUE, STANDARD_HUE);
    ESP_LOGI(TAG, "%s: value=%d, val->val.u8=%d, MATTER_HUE=%d, STANDARD_HUE=%d", __func__, value, val->val.u8, MATTER_HUE, STANDARD_HUE);
    return led_driver_set_hue(_handle, value);
}

esp_err_t LightDevice::_matterGetBrightness(node_t *node, endpoint_t *endpoint, esp_matter_attr_val_t *val)
{
    auto cluster = cluster::get(endpoint, LevelControl::Id);
    auto attribute = attribute::get(cluster, LevelControl::Attributes::CurrentLevel::Id);
    return attribute::get_val(attribute, val);
}

esp_err_t LightDevice::_matterGetColorMode(node_t *node, endpoint_t *endpoint, esp_matter_attr_val_t *val)
{
    auto cluster = cluster::get(endpoint, ColorControl::Id);
    auto attribute = attribute::get(cluster, ColorControl::Attributes::ColorMode::Id);
    return attribute::get_val(attribute, val);
}

esp_err_t LightDevice::_matterGetColorTemperature(node_t *node, endpoint_t *endpoint, esp_matter_attr_val_t *val)
{
    auto cluster = cluster::get(endpoint, ColorControl::Id);
    auto attribute = attribute::get(cluster, ColorControl::Attributes::ColorTemperatureMireds::Id);
    return attribute::get_val(attribute, val);
}
esp_err_t LightDevice::_matterSetColorTemperature(node_t *node, endpoint_t *endpoint, esp_matter_attr_val_t *val)
{
    auto cluster = cluster::get(endpoint, ColorControl::Id);
    auto attribute = attribute::get(cluster, ColorControl::Attributes::ColorTemperatureMireds::Id);
    return attribute::set_val(attribute, val);
}

esp_err_t LightDevice::_matterGetOnOff(node_t *node, endpoint_t *endpoint, esp_matter_attr_val_t *val)
{
    auto cluster = cluster::get(endpoint, OnOff::Id);
    auto attribute = attribute::get(cluster, OnOff::Attributes::OnOff::Id);
    return attribute::get_val(attribute, val);
}
esp_err_t LightDevice::_matterSetOnOff(node_t *node, endpoint_t *endpoint, esp_matter_attr_val_t *val)
{
    auto cluster = cluster::get(endpoint, OnOff::Id);
    auto attribute = attribute::get(cluster, OnOff::Attributes::OnOff::Id);
    return attribute::set_val(attribute, val);
}