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
#include <esp_err.h>
#include <esp_system.h>
#include <esp_chip_info.h>
#include <esp_flash.h>
#include <esp_idf_version.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <platform/ESP32/CHIPDevicePlatformEvent.h>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#include "./QueueMain.h"
#include "../AppContext.h"
#include "../ButtonID.h"
#include "../inc/ConnectivityManagerImpl.h"

static const char *TAG = "QueueMain";

using namespace esp_matter;

constexpr auto k_timeout_seconds = 300;

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
#ifndef ABORT_APP_ON_FAILURE
#define ABORT_APP_ON_FAILURE(x, ...)         \
    do                                       \
    {                                        \
        if (!(unlikely(x)))                  \
        {                                    \
            __VA_ARGS__;                     \
            vTaskDelay(pdMS_TO_TICKS(5000)); \
            abort();                         \
        }                                    \
    } while (0)
#endif

////////////////////////////////////////////////////////////////////////////////////////////
QueueMain *QueueMain::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
QueueMain *QueueMain::getInstance(void)
{
    if (!_instance)
    {
        static QueueMain instance;
        _instance = &instance;
    }
    return _instance;
}

#if defined ESP_PLATFORM
////////////////////////////////////////////////////////////////////////////////////////////
// Thread for ESP32
////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_QUEUE_SIZE 128 // message queue size for app task
static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

////////////////////////////////////////////////////////////////////////////////////////////
void QueueMain::printAppInfo(void)
{
#if ARDUINO
    PRINTLN("===============================================================================");
    PRINTLN("ESP.getChipModel()=", ESP.getChipModel(), ", getChipRevision()=", ESP.getChipRevision(), ", getFlashChipSize()=", ESP.getFlashChipSize(),
            "\r\nNumber of cores=", ESP.getChipCores(), ", SDK version=", ESP.getSdkVersion());
    PRINTLN("ArduProf version: ", ARDUPROF_VER);
    PRINTLN("===============================================================================");
#else
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK)
    {
        flash_size /= ((uint32_t)(1024 * 1024));
    }
    else
    {
        ESP_LOGI(TAG, "%s: esp_flash_get_size() failed", __func__);
    }

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;

    char chipFeatures[64];
    strcpy(chipFeatures, (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "");
    strcat(chipFeatures, (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "");
    if (chip_info.features & CHIP_FEATURE_BLE)
    {
        strcat(chipFeatures, "BLE");
    }
    if (chip_info.features & CHIP_FEATURE_IEEE802154)
    {
        strcat(chipFeatures, ", 802.15.4 (Zigbee/Thread)");
    }

    ESP_LOGI(TAG, "===============================================================================");
    ESP_LOGI(TAG, "ESP.getChipModel()=%s, getChipRevision()=%d.%d, getFlashChipSize()=%lu MB, features=%s, Number of cores=%d, getSdkVersion()=%s",
             CONFIG_IDF_TARGET, major_rev, minor_rev, flash_size, chipFeatures, chip_info.cores, esp_get_idf_version());
    ESP_LOGI(TAG, "ArduProf version: " ARDUPROF_VER);
    ESP_LOGI(TAG, "===============================================================================");
#endif
}

QueueMain::QueueMain() : ardufreertos::MessageBus(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                         handlerMap(),
                         //  _fanDevice(),
                         _lightDevice(),
                         _buttonBoot(this)
{
    _instance = this;

    handlerMap = {
        __EVENT_MAP(QueueMain, EventApp),
        __EVENT_MAP(QueueMain, EventSystem),
        __EVENT_MAP(QueueMain, EventNull), // {EventNull, &QueueMain::handlerEventNull},
    };
}
#endif

void QueueMain::start(void *ctx)
{
    ESP_LOGI(TAG, "%s: core%d: uxTaskPriorityGet(nullptr)=%d, xPortGetFreeHeapSize()=%u, minimum free stack=%u",
             __func__, uxTaskPriorityGet(nullptr), xPortGetCoreID(), xPortGetFreeHeapSize(), uxTaskGetStackHighWaterMark(nullptr));
    // LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
    MessageBus::start(ctx);

    printAppInfo();

    _buttonBoot.init();

    // Create a Matter node and add the mandatory Root Node device type on endpoint 0
    node::config_t node_config;
    node_t *node = node::create(&node_config, onAttributeUpdate, onIdentification);
    if (!node)
    {
        ESP_LOGE(TAG, "%s: Matter node creation failed", __func__);
    }

    endpoint::extended_color_light::config_t lightConfig;
    LightDevice::getDefaultConfig(lightConfig);
    endpoint_t *lightEndpoint = endpoint::extended_color_light::create(node, &lightConfig, ENDPOINT_FLAG_NONE, &_lightDevice);
    ABORT_APP_ON_FAILURE(lightEndpoint != nullptr, ESP_LOGE(TAG, "Failed to create extended color light endpoint"));
    uint16_t lightEndpointID = endpoint::get_id(lightEndpoint);
    ESP_LOGI(TAG, "%s: Light created with endpoint_id %d", __func__, lightEndpointID);
    _lightDevice.init(lightEndpointID);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    // Set OpenThread platform config
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

    // start Matter
    esp_err_t err = esp_matter::start(onMatterEvent);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Matter start failed: %d", __func__, err);
    }

#if CONFIG_ENABLE_ENCRYPTED_OTA
    err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "%s: Failed to initialized the encrypted OTA, err: %d", __func__, err);
    }
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::init();
#endif
}

void QueueMain::onMessage(const Message &msg)
{
    auto func = handlerMap[msg.event];
    if (func)
    {
        (this->*func)(msg);
    }
    else
    {
        ESP_LOGW(TAG, "%s: Unsupported event=%d, iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
        // LOG_TRACE("Unsupported event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
    }
}

/////////////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(QueueMain, EventApp, msg) // void QueueMain::handlerEventApp(const Message &msg)
{
    ESP_LOGI(TAG, "%s: EventApp(%d), iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
    auto src = static_cast<enum AppTriggerSource>(msg.iParam);
    switch (src)
    {
    case AppUserCommand:
        handlerUserCommand(msg);
        break;
    default:
        ESP_LOGW(TAG, "unsupported AppTriggerSource=%d", src);
        break;
    }
}

__EVENT_FUNC_DEFINITION(QueueMain, EventSystem, msg) // void QueueMain::handlerEventSystem(const Message &msg)
{
    // ESP_LOGI(TAG, "%s: EventSystem(%d), iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
    enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysButtonClick:
    {
        handlerButtonClick(msg);
        break;
    }
    default:
        ESP_LOGW(TAG, "%s: unsupported SystemTriggerSource=%d", __func__, src);
        break;
    }
}

// define EventNull handler
__EVENT_FUNC_DEFINITION(QueueMain, EventNull, msg) // void QueueMain::handlerEventNull(const Message &msg)
{
    ESP_LOGI(TAG, "%s: EventNull(%d), iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
    // LOG_TRACE("EventNull(", msg.event, "), iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
}
/////////////////////////////////////////////////////////////////////////////
void QueueMain::handlerUserCommand(const Message &msg)
{
    auto usrCmd = msg.uParam;
    switch (usrCmd)
    {
    case UsrReqUpdate:
    {
        // ESP_LOGW(TAG, "%s: UsrReqUpdate", __func__);
        auto ctx = static_cast<AppContext *>(context());
        auto state = (uint32_t)(_lightDevice.getState());
        postEvent(ctx->threadPanel, EventApp, AppDeviceUpdate, DeviceLamp, state);
        break;
    }
    case UsrClick:
    {
        auto buttonID = msg.lParam;
        // ESP_LOGI(TAG, "%s: UsrClick: buttonID=%lu", __func__, buttonID);
        if (buttonID == ButtonLampEspOn)
        {
            _lightDevice.clickButtonOn();
            auto ctx = static_cast<AppContext *>(context());
            auto state = (uint32_t)(_lightDevice.getState());
            postEvent(ctx->threadPanel, EventApp, AppDeviceUpdate, DeviceLamp, state);
        }
        else
        {
            ESP_LOGW(TAG, "%s: UsrClick: unsupported buttonID=%lu", __func__, buttonID);
        }
        break;
    }
    default:
        ESP_LOGW(TAG, "%s: unsupported user command=%d", __func__, usrCmd);
        break;
    }
}

void QueueMain::handlerButtonClick(const Message &msg)
{
    auto pin = msg.uParam;
    if (pin == _buttonBoot.getPin())
    {
        ESP_LOGI(TAG, "%s: ButtonClick: buttonBoot", __func__);
        auto ctx = static_cast<AppContext *>(context());
        postEvent(ctx->queueMain, EventApp, AppUserCommand, UsrClick, ButtonLampEspOn);
    }
    else
    {
        ESP_LOGI(TAG, "%s: unsupported pin=%d", __func__, pin);
    }
}

void QueueMain::onPublicEvent(const ChipDeviceEvent *event, intptr_t arg)
{
    ESP_LOGI(TAG, "%s: event->Type=0x%04x (%u)", __func__, event->Type, event->Type);
}
void QueueMain::onPlatformSpecificEvent(const ChipDeviceEvent *event, intptr_t arg)
{
    auto platform = (chip::DeviceLayer::ChipDevicePlatformEvent *)(&event->Platform);
    // ESP_LOGI(TAG, "platform->ESPSystemEvent.Id=%ld, Base=%s", platform->ESPSystemEvent.Id, platform->ESPSystemEvent.Base);

    if (platform->ESPSystemEvent.Base == IP_EVENT)
    {
        switch (platform->ESPSystemEvent.Id)
        {
        case IP_EVENT_STA_GOT_IP: // station got IP from connected AP
        {
            ESP_LOGI(TAG, "%s: IP_EVENT_STA_GOT_IP", __func__);
            auto ctx = static_cast<AppContext *>(context());
            postEvent(ctx->threadPanel, EventSystem, SysNetworkAvailable, true);
            break;
        }

        case IP_EVENT_GOT_IP6: // station or ap or ethernet interface v6IP addr is preferred
            ESP_LOGI(TAG, "%s: IP_EVENT_GOT_IP6", __func__);
            break;

        case IP_EVENT_STA_LOST_IP: // station lost IP and the IP is reset to 0
        {
            ESP_LOGI(TAG, "%s: IP_EVENT_STA_LOST_IP", __func__);
            auto ctx = static_cast<AppContext *>(context());
            postEvent(ctx->threadPanel, EventSystem, SysNetworkAvailable, false);
            break;
        }

        default:
            ESP_LOGW(TAG, "%s: unsupported platform->ESPSystemEvent.Base=%s, .Id=0x%04lx (%ld)", __func__, platform->ESPSystemEvent.Base, platform->ESPSystemEvent.Id, platform->ESPSystemEvent.Id);
            break;
        }
    }
    else if (platform->ESPSystemEvent.Base == WIFI_EVENT)
    {
        // ESP_LOGI(TAG, "%s: WIFI_EVENT: platform->ESPSystemEvent.Id=0x%04lx (%ld), event->Type=0x%04x (%d)", __func__, platform->ESPSystemEvent.Id, platform->ESPSystemEvent.Id, event->Type, event->Type);
        switch (platform->ESPSystemEvent.Id)
        {
        case WIFI_EVENT_SCAN_DONE:
        {
            ESP_LOGI(TAG, "%s: WIFI_EVENT_SCAN_DONE: platform->ESPSystemEvent.Data.WiFiStaScanDone.status=%ld, number=%u, scan_id=%u",
                     __func__, platform->ESPSystemEvent.Data.WiFiStaScanDone.status, platform->ESPSystemEvent.Data.WiFiStaScanDone.number, platform->ESPSystemEvent.Data.WiFiStaScanDone.scan_id);
            switch ((chip::DeviceLayer::ConnectivityChange)(platform->ESPSystemEvent.Data.WiFiStaScanDone.status))
            {
            case chip::DeviceLayer::ConnectivityChange::kConnectivity_NoChange:
                ESP_LOGI(TAG, "%s: kConnectivity_NoChange", __func__);
                break;
            case chip::DeviceLayer::ConnectivityChange::kConnectivity_Established:
                ESP_LOGI(TAG, "%s: kConnectivity_Established", __func__);
                break;
            case chip::DeviceLayer::ConnectivityChange::kConnectivity_Lost:
                ESP_LOGI(TAG, "%s: kConnectivity_Lost", __func__);
                break;
            }
            break;
        }
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "%s: WIFI_EVENT_STA_START (%d)", __func__, WIFI_EVENT_STA_START);
            break;
        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(TAG, "%s: WIFI_EVENT_STA_CONNECTED", __func__);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            ESP_LOGI(TAG, "%s: WIFI_EVENT_STA_DISCONNECTED", __func__);
            break;
        }
        default:
            ESP_LOGW(TAG, "%s: unsupported platform->ESPSystemEvent.Base=%s, .Id=0x%04lx (%ld)", __func__, platform->ESPSystemEvent.Base, platform->ESPSystemEvent.Id, platform->ESPSystemEvent.Id);
            break;
        }
    }
    else
    {
        ESP_LOGW(TAG, "%s: unsupported platform->ESPSystemEvent.Base=%s, event->Type=0x%04x (%d)", __func__, platform->ESPSystemEvent.Base, event->Type, event->Type);
    }
}

void QueueMain::onMatterEvent(const ChipDeviceEvent *event, intptr_t arg)
{
    // ESP_LOGI(TAG, "event->Type=0x%04x (%d)", event->Type, event->Type);
    switch (event->Type)
    {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "%s: kInterfaceIpAddressChanged", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "%s: kCommissioningComplete", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "%s: kFailSafeTimerExpired", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "%s: kCommissioningSessionStarted", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "%s: kCommissioningSessionStopped", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "%s: kCommissioningWindowOpened", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "%s: kCommissioningWindowClosed", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
    {
        ESP_LOGI(TAG, "%s: kFabricRemoved", __func__);
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
        {
            chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
            if (!commissionMgr.IsCommissioningWindowOpen())
            {
                // After removing last fabric, this example does not remove the Wi-Fi credentials
                // and still has IP connectivity so, only advertising on DNS-SD.
                CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds, chip::CommissioningWindowAdvertisement::kDnssdOnly);
                if (err != CHIP_NO_ERROR)
                {
                    ESP_LOGE(TAG, "%s: Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, __func__, err.Format());
                }
            }
        }
        break;
    }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "%s: kFabricWillBeRemoved", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "%s: kFabricUpdated", __func__);
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "%s: kFabricCommitted", __func__);
        break;

    default:
        if (chip::DeviceLayer::DeviceEventType::IsPlatformSpecific(event->Type))
        {
            // ESP_LOGI(TAG, "IsPlatformSpecific() returns true)");
            getInstance()->onPlatformSpecificEvent(event, arg);
        }
        else if (chip::DeviceLayer::DeviceEventType::IsPublic(event->Type))
        {
            // ESP_LOGI(TAG, "%s: IsPublic() returns true: event->Type=0x%04x (%d)", __func__, event->Type, event->Type);
            getInstance()->onPublicEvent(event, arg);
        }
        else
        {
            ESP_LOGW(TAG, "%s: unsupported event->Type=0x%04x (%d)", __func__, event->Type, event->Type);
        }
        break;
    }
}

esp_err_t QueueMain::onAttributeUpdate(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                       uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    ESP_LOGI(TAG, "%s: type=%u, endpoint_id=%u, cluster_id=%lu, attribute_id=%lu, val=%p, priv_data=%p", __func__, type, endpoint_id, cluster_id, attribute_id, val, priv_data);

    esp_err_t err = ESP_OK;
    auto instance = QueueMain::getInstance();
    switch (type)
    {
    case attribute::PRE_UPDATE:
    {
        if (endpoint_id == instance->_lightDevice.getEndpointID())
        {
            err = instance->onLightPreUpdate(cluster_id, attribute_id, val, priv_data);
        }
        else
        {
            ESP_LOGW(TAG, "%s: unsupported PRE_UPDATE on endpoint_id %d", __func__, endpoint_id);
        }
        break;
    }
    case attribute::POST_UPDATE:
        // nothing to do for POST_UPDATE
        // ESP_LOGI(TAG, "%s: nothing to do for POST_UPDATE on endpoint_id %d", __func__, endpoint_id);
        break;
    default:
        ESP_LOGW(TAG, "%s: unsupported type %d", __func__, type);
        break;
    }
    return err;
}

esp_err_t QueueMain::onIdentification(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                      uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "%s: type=%u, endpoint_id=%u, effect=%u, variant=%u, priv_data=%p", __func__, type, endpoint_id, effect_id, effect_variant, priv_data);
    // ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

esp_err_t QueueMain::onLightPreUpdate(uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    ESP_LOGI(TAG, "%s: cluster_id=%lu, attribute_id=%lu, val=%p, priv_data=%p", __func__, cluster_id, attribute_id, val, priv_data);

    esp_err_t err = ESP_OK;
    configASSERT(priv_data == &_lightDevice);
    err = _lightDevice.onAttributeUpdate(cluster_id, attribute_id, val);
    return err;
}