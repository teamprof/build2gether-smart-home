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
#include <map>
#include <esp_err.h>
#include <esp_matter.h>
#include <esp_matter_identify.h>
#include "../ArduProfFreeRTOS.h"
#include "../device/ButtonBoot.h"
#include "../device/LightDevice.h"
#include "../AppEvent.h"

using namespace esp_matter;

struct ChipDeviceEvent;

class QueueMain final : public ardufreertos::MessageBus
{
public:
    QueueMain();
    static QueueMain *getInstance(void);

    virtual void start(void *);
    virtual void onMessage(const Message &msg) override;

    static void printAppInfo(void);

protected:
    typedef void (QueueMain::*funcPtr)(const Message &);
    std::map<int16_t, funcPtr> handlerMap;

private:
    static QueueMain *_instance;
    LightDevice _lightDevice;
    ButtonBoot _buttonBoot;

    void handlerUserCommand(const Message &msg);
    void handlerButtonClick(const Message &msg);
    void onPlatformSpecificEvent(const ChipDeviceEvent *event, intptr_t arg);
    void onPublicEvent(const ChipDeviceEvent *event, intptr_t arg);
    esp_err_t onFanPreUpdate(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);
    esp_err_t onLightPreUpdate(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);

    static void onMatterEvent(const ChipDeviceEvent *event, intptr_t arg);
    static esp_err_t onIdentification(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                      uint8_t effect_variant, void *priv_data);
    static esp_err_t onAttributeUpdate(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                       uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data);

    ///////////////////////////////////////////////////////////////////////
    // declare event handler
    ///////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventApp)
    __EVENT_FUNC_DECLARATION(EventSystem)
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};
