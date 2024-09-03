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
#include <sys/socket.h>

#include "ArduProfFreeRTOS.h"
#include "./ThreadPanel.h"
#include "./TaskTcpClient.h"
#include "../AppContext.h"
#include "../model/LampModel.h"

static const char *TAG = "ThreadPanel";

////////////////////////////////////////////////////////////////////////////////////////////
ThreadPanel *ThreadPanel::_instance = nullptr;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadPanel *ThreadPanel::getInstance(void)
{
    if (!_instance)
    {
        static ThreadPanel instance;
        _instance = &instance;
    }
    return _instance;
}

#if defined ESP_PLATFORM
////////////////////////////////////////////////////////////////////////////////////////////
// Thread for ESP32
////////////////////////////////////////////////////////////////////////////////////////////

#define RUNNING_CORE 0 // dedicate core 0 for Thread
// #define RUNNING_CORE 1 // dedicate core 1 for Thread
// #define RUNNING_CORE ARDUINO_RUNNING_CORE

#define TASK_NAME "ThreadPanel"
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

#define TASK_INIT_NAME "taskDelayInit"
#define TASK_INIT_STACK_SIZE 4096
#define TASK_INIT_PRIORITY 0

static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
static StaticQueue_t xStaticQueue;

static StackType_t xStack[TASK_STACK_SIZE];
static StaticTask_t xTaskBuffer;

////////////////////////////////////////////////////////////////////////////////////////////
ThreadPanel::ThreadPanel() : ardufreertos::ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                             handlerMap(),
                             _timer1Hz("Timer 1Hz",
                                       pdMS_TO_TICKS(1000),
                                       [](TimerHandle_t xTimer)
                                       {
                                           auto instance = ThreadPanel::getInstance();
                                           instance->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                           //
                                       }),
                             _isNetworkAvailable(false),
                             _connectionState(ConnectionState::Disconnect),
                             _sock(-1),
                             _hTaskConnect(NULL)
{
    _instance = this;

    // setup event handlers
    handlerMap = {
        __EVENT_MAP(ThreadPanel, EventApp),
        __EVENT_MAP(ThreadPanel, EventSystem),
        __EVENT_MAP(ThreadPanel, EventNull), // {EventNull, &ThreadPanel::handlerEventNull},
    };
}

void ThreadPanel::start(void *ctx)
{
    ESP_LOGI(TAG, "%s: core%d: uxTaskPriorityGet(nullptr)=%d, xPortGetFreeHeapSize()=%u, minimum free stack=%u",
             __func__, uxTaskPriorityGet(nullptr), xPortGetCoreID(), xPortGetFreeHeapSize(), uxTaskGetStackHighWaterMark(nullptr));
    // LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
    ThreadBase::start(ctx);

    _taskHandle = xTaskCreateStaticPinnedToCore(
        [](void *instance)
        { static_cast<ThreadBase *>(instance)->run(); },
        TASK_NAME,
        TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
        this,
        TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        xStack,
        &xTaskBuffer,
        RUNNING_CORE);
}
#endif


void ThreadPanel::setup(void)
{
    ThreadBase::setup();
}

/////////////////////////////////////////////////////////////////////////////
void ThreadPanel::onMessage(const Message &msg)
{
    // ESP_LOGI(TAG, "%s: event=%d, iParam=%d, uParam=%u, lParam=%lu", __func__,, msg.event, msg.iParam, msg.uParam, msg.lParam);
    // LOG_TRACE("event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
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
__EVENT_FUNC_DEFINITION(ThreadPanel, EventApp, msg) // void ThreadPanel::handlerEventApp(const Message &msg)
{
    // ESP_LOGI(TAG, "%s: EventApp(%d), iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
    auto src = static_cast<enum AppTriggerSource>(msg.iParam);
    switch (src)
    {
    case AppDeviceUpdate:
        handlerUpdateDevice(msg);
        break;
    case AppTcpConnection:
        handlerTcpConnection(msg);
        break;
    case AppUserCommand:
        handlerUserCommand(msg);
        break;
    default:
        ESP_LOGW(TAG, "unsupported AppTriggerSource=%d", src);
        break;
    }
}
__EVENT_FUNC_DEFINITION(ThreadPanel, EventSystem, msg) // void ThreadPanel::handlerEventSystem(const Message &msg)
{
    // ESP_LOGI(TAG, "%s: EventSystem(%d), iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
    enum SystemTriggerSource src = static_cast<enum SystemTriggerSource>(msg.iParam);
    switch (src)
    {
    case SysSoftwareTimer:
        handlerSoftwareTimer((TimerHandle_t)(msg.lParam));
        break;
    case SysNetworkAvailable:
    {
        handlerNetworkAvailable(msg);
        break;
    }
    default:
        ESP_LOGW(TAG, "%s: unsupported SystemTriggerSource=%d", __func__, src);
        break;
    }
}

// define EventNull handler
__EVENT_FUNC_DEFINITION(ThreadPanel, EventNull, msg) // void ThreadPanel::handlerEventNull(const Message &msg)
{
    ESP_LOGI(TAG, "%s: EventNull(%d), iParam=%d, uParam=%u, lParam=%lu", __func__, msg.event, msg.iParam, msg.uParam, msg.lParam);
    // LOG_TRACE("EventNull(", msg.event, "), iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
}

/////////////////////////////////////////////////////////////////////////////
void ThreadPanel::handlerUpdateDevice(const Message &msg)
{
    auto device = msg.uParam;
    switch (device)
    {
    case DeviceLamp:
        {
            auto sock = _sock;
            if (sock < 0)
            {
                ESP_LOGW(TAG, "%s: invalid socket", __func__);
                return;
            }

            auto state = (int)msg.lParam;
            LampModel model;
            if (model.build(LampModel::NAME, LampModel::UPDATE, 0, state))
            {
                const char *str = model.stringnify();
                if (str)
                {
                    ESP_LOGI(TAG, "%s: model.stringnify() returns %s", __func__, str);

                    int err = send(sock, str, strlen(str), 0);
                    if (err < 0)
                    {
                        ESP_LOGW(TAG, "%s: send() failed", __func__);
                    }
                }
                else
                {
                    ESP_LOGI(TAG, "%s: model.stringnify() returns NULL", __func__);
                }
                model.stringDelete((void *)str);
            }
            else
            {
                ESP_LOGW(TAG, "%s: model.build() failed", __func__);
            }
            break;
        }
    default:
        ESP_LOGW(TAG, "%s: unsupported device=%d", __func__, device);
        break;
    }
}
void ThreadPanel::handlerUserCommand(const Message &msg)
{
    auto usrCmd = msg.uParam;
    switch (usrCmd)
    {
    case UsrReqUpdate:
    case UsrClick:
    {
        auto ctx = static_cast<AppContext *>(context());
        postEvent(ctx->queueMain, msg);
        break;
    }
    default:
        ESP_LOGW(TAG, "%s: unsupported user command=%d", __func__, usrCmd);
        break;
    }
}
void ThreadPanel::handlerTcpConnection(const Message &msg)
{
    bool isSuccess = (bool)msg.uParam;
    if (isSuccess)
    {
        ESP_LOGI(TAG, "%s: Connect success", __func__);
        _connectionState = ConnectionState::Connect;
        // _timer1Hz.start();
    }
    else
    {
        ESP_LOGI(TAG, "%s: %s", __func__, msg.lParam == ErrDisconnect ? "Disconnect" : "Connect failed");
        _connectionState = ConnectionState::Disconnect;
        // _timer1Hz.stop();

        closeSocket();

        TaskHandle_t handle = _hTaskConnect;
        _hTaskConnect = NULL;
        if (handle)
        {
            // ESP_LOGI(TAG, "%s: delete taskTcpClient", __func__);
            vTaskDelete(handle);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
        if (_isNetworkAvailable)
        {
            _connectionState = ConnectionState::Connecting;
            TaskTcpClient::start(this);
        }
    }
}
void ThreadPanel::handlerNetworkAvailable(const Message &msg)
{
    bool isAvailable = (bool)msg.uParam;
    ESP_LOGI(TAG, "%s: isAvailable=%d, _connectionState=%d", __func__, isAvailable, _connectionState);
    if (isAvailable && _connectionState == ConnectionState::Disconnect)
    {

        _connectionState = ConnectionState::Connecting;
        TaskTcpClient::start(this);
    }
    else
    {
        closeSocket();
    }
    _isNetworkAvailable = isAvailable;
}

void ThreadPanel::handlerSoftwareTimer(TimerHandle_t xTimer)
{
    if (xTimer == _timer1Hz.timer())
    {
        ESP_LOGI(TAG, "%s: _timer1Hz", __func__);
    }
    else
    {
        ESP_LOGI(TAG, "%s: unsupported timer handle=0x%08lx", __func__, (uint32_t)(xTimer));
    }
}

void ThreadPanel::closeSocket(void)
{
    int sock = _sock;
    _sock = -1;
    if (sock != -1)
    {
        // ESP_LOGI(TAG, "%s: shutdown and close socket: sock=%d", __func__, sock);
        shutdown(sock, 0);
        close(sock);
    }
}