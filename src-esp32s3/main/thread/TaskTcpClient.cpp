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
#include <lwip/netdb.h>
#include <lwip/dns.h>

#include "ArduProfFreeRTOS.h"
#include "./ThreadPanel.h"
#include "./TaskTcpClient.h"
#include "./AppEvent.h"
#include "../model/LampModel.h"

static const char *TAG = "TaskTcpClient";

////////////////////////////////////////////////////////////////////////////////////////////
#define SERVER_NAME "unihiker.local"
#define SERVER_PORT 8080

#define RX_BUF_SIZE 128

#define TASK_NAME "TaskTcpClient"
#define TASK_STACK_SIZE 4096
#define TASK_PRIORITY 3

////////////////////////////////////////////////////////////////////////////////////////////
char TaskTcpClient::_rxBuf[RX_BUF_SIZE];

////////////////////////////////////////////////////////////////////////////////////////////
void TaskTcpClient::start(void *threadParent)
{
    auto parent = static_cast<ThreadPanel *>(threadParent);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "%s: Unable to create socket: errno %d", __func__, errno);
        parent->postEvent(EventApp, AppTcpConnection, false, ErrCreateSocket);
        return;
    }
    parent->_sock = sock;
    ESP_LOGI(TAG, "%s: Socket created: sock=%d", __func__, sock);

    BaseType_t rst = xTaskCreate(
        [](void *param)
        {
            run(param);

            //  do not return, let parent to delete this task
            while (true)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        },
        TASK_NAME,
        TASK_STACK_SIZE,
        parent,
        TASK_PRIORITY,
        &parent->_hTaskConnect);

    if (rst != pdPASS)
    {
        ESP_LOGE(TAG, "%s: xTaskCreate() failed", __func__);
        parent->postEvent(EventApp, AppTcpConnection, false, ErrTaskCreate);
    }
}

void TaskTcpClient::run(void *threadParent)
{
    auto parent = static_cast<ThreadPanel *>(threadParent);
    int sock = parent->_sock;

    if (sock < 0)
    {
        ESP_LOGW(TAG, "%s: invalid socket %d", __func__, sock);
        parent->postEvent(EventApp, AppTcpConnection, false, ErrInvalidSocket);
        return;
    }

    ///////////////////////////////////////////////////////////////////////////
    // connect to server
    // support both IPV6 and IPV4
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;

    int err = getaddrinfo(SERVER_NAME, STR(SERVER_PORT), &hints, &res);
    if (err != 0 || res == NULL)
    {
        ESP_LOGE(TAG, "%s: DNS lookup failed err=%d res=%p", __func__, err, res);
        parent->postEvent(EventApp, AppTcpConnection, false, ErrDnsLookup);
        return;
    }
    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "%s: DNS lookup succeeded. IP=%s", __func__, inet_ntoa(*addr));

    err = connect(sock, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (!err)
    {
        // ESP_LOGI(TAG, "connected %s:%d", inet_ntoa(*addr), SERVER_PORT);
        ESP_LOGI(TAG, "%s: connected %s:%d", __func__, SERVER_NAME, SERVER_PORT);
        parent->postEvent(EventApp, AppTcpConnection, true);
    }
    else
    {
        ESP_LOGE(TAG, "%s: Socket unable to connect: errno 0x%04x (%d)", __func__, errno, errno);
        parent->postEvent(EventApp, AppTcpConnection, false, ErrConnect);
        return;
    }
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // receiver
    int len = recv(sock, _rxBuf, sizeof(_rxBuf) - 1 - 1, 0);
    while (len > 0)
    {
        _rxBuf[len] = '\0';
        ESP_LOGI(TAG, "%s: recv() returns %d, _rxBuf: %s", __func__, len, _rxBuf);

        LampModel jsonModel;
        // jsonModel.reset();
        bool rst = jsonModel.parse(_rxBuf);
        if (rst)
        {
            // ESP_LOGI(TAG, "%s: device=%s, event=%s, arg0=%d, arg1=%d", __func__, fanModel.device(), fanModel.event(), fanModel.arg0(), fanModel.arg1());
            processJsonData(parent, jsonModel);
        }
        else
        {
            ESP_LOGW(TAG, "%s: fanModel.parse() failed", __func__);
        }

        len = recv(sock, _rxBuf, sizeof(_rxBuf) - 1 - 1, 0);
    }
    ///////////////////////////////////////////////////////////////////////////

    // auto parent = static_cast<ThreadPanel *>(param);
    parent->postEvent(EventApp, AppTcpConnection, false, ErrDisconnect);
}

void TaskTcpClient::processJsonData(ThreadPanel *parent, LampModel &jsonModel)
// void TaskTcpClient::processJsonData(ThreadPanel *parent, FanModel &jsonModel)
{
    ESP_LOGI(TAG, "%s: device=%s, event=%s, arg0=%d, arg1=%d", __func__, jsonModel.device(), jsonModel.event(), jsonModel.arg0(), jsonModel.arg1());
    auto device = jsonModel.device();
    if (strcmp(device, LampModel::NAME))
    {
        ESP_LOGW(TAG, "%s: unsupported device=%s, event=%s, arg0=%d, arg1=%d", __func__, jsonModel.device(), jsonModel.event(), jsonModel.arg0(), jsonModel.arg1());
    }

    // process fan device event
    auto event = jsonModel.event();
    if (!strcmp(event, LampModel::REQ_UPDATE))
    {
        ESP_LOGI(TAG, "%s: req-update event", __func__);
        parent->postEvent(EventApp, AppUserCommand, UsrReqUpdate);
    }
    else if (!strcmp(event, LampModel::USER_CLICK))
    {
        auto buttonID = jsonModel.arg0();
        ESP_LOGI(TAG, "%s: user-click event: buttonID=%d", __func__, buttonID);
        parent->postEvent(EventApp, AppUserCommand, UsrClick, buttonID);
    }
    else
    {
        ESP_LOGW(TAG, "%s: unsupported fan event=%s, arg0=%d, arg1=%d", __func__, jsonModel.event(), jsonModel.arg0(), jsonModel.arg1());
    }
}