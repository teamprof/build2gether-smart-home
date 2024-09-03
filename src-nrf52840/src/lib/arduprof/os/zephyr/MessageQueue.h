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
#include <stdint.h>
#include <zephyr/kernel.h>

#include "../../type/Message.h"

#ifdef __ZEPHYR__

class MessageQueue
{
public:
    MessageQueue(k_msgq *queue) : _queue(queue)
    {
    }
    //     // MessageQueue(uint16_t queueLength,
    //     //              uint8_t *pucQueueStorageBuffer = nullptr,
    //     //              StaticQueue_t *pxQueueBuffer = nullptr)
    //     // {
    //     //     if (pucQueueStorageBuffer != nullptr && pxQueueBuffer != nullptr)
    //     //     {
    //     //         _queue = xQueueCreateStatic(queueLength, sizeof(Message), pucQueueStorageBuffer, pxQueueBuffer);
    //     //     }
    //     //     else
    //     //     {
    //     //         _queue = xQueueCreate(queueLength, sizeof(Message));
    //     //     }
    //     //     configASSERT(_queue != NULL);
    //     // }

    //     // ~MessageQueue()
    //     // {
    //     //     vQueueDelete(_queue);
    //     //     _queue = nullptr;
    //     // }

    void postEvent(k_msgq *queue, int16_t event, int16_t iParam = 0, uint16_t uParam = 0, uint32_t lParam = 0L, k_timeout_t timeout = K_NO_WAIT)
    {
        Message msg = {
            .event = event,
            .iParam = iParam,
            .uParam = uParam,
            .lParam = lParam,
        };
        postEvent(queue, msg, timeout);
    }
    inline void postEvent(k_msgq *queue, const Message &msg, k_timeout_t timeout = K_NO_WAIT)
    {
        if (!queue)
        {
            return;
        }

        if (k_msgq_put(queue, &msg, timeout) != 0)
        {
            // LOG_INF("Failed to post event to app task event queue");
        }
    }

    inline void postEvent(int16_t event, int16_t iParam = 0, uint16_t uParam = 0, uint32_t lParam = 0L, k_timeout_t timeout = K_NO_WAIT)
    {
        postEvent(queue(), event, iParam, uParam, lParam, timeout);
    }
    inline void postEvent(const Message &msg, k_timeout_t timeout = K_NO_WAIT)
    {
        postEvent(queue(), msg, timeout);
    }

    inline k_msgq *queue(void)
    {
        return _queue;
    }

protected:
    k_msgq *_queue;
};

/////////////////////////////////////////////////////////////////////////////
#define __EVENT_MAP(class, event)      \
    {                                  \
        event, &class ::handler##event \
    }
#define __EVENT_FUNC_DEFINITION(class, event, msg) void class ::handler##event(const Message &msg)
#define __EVENT_FUNC_DECLARATION(event) void handler##event(const Message &msg);
/////////////////////////////////////////////////////////////////////////////

#endif