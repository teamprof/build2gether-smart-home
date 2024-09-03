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
#include <nvs_flash.h>

#include "./thread/QueueMain.h"
#include "./thread/ThreadPanel.h"
#include "AppContext.h"

///////////////////////////////////////////////////////////////////////////////
static const char *TAG = "app_main";

///////////////////////////////////////////////////////////////////////////////
// define variable "context" and initialize pointers to "threadPanel"
static AppContext context = {0};

AppContext *getContext(void)
{
    return &context;
}

static void createTask(void)
{
    // define variable "queueMain" for Arduino thred to interact with ArduProf framework.
    static QueueMain queueMain;

    // define variable "threadPanel" for application thread. (Define other thread as you need)
    static ThreadPanel threadPanel;

    // initialize application context
    context.queueMain = &queueMain;
    context.threadPanel = &threadPanel;

    // start threadPanel
    ESP_LOGI(TAG, "%s: threadPanel.start()", __func__);
    threadPanel.start(&context);

    // start queueMain
    ESP_LOGI(TAG, "%s: queueMain.start()", __func__);
    queueMain.start(&context);
}

extern "C" void app_main()
{
    // Initialize the ESP NVS layer
    nvs_flash_init();

    createTask();

    auto ctx = getContext();
    auto pQueueMain = static_cast<QueueMain *>(ctx->queueMain);
    assert(pQueueMain);

    // process message in queueMain if available
    pQueueMain->messageLoopForever(); // never return
    // pQueueMain->messageLoop(0); // non-blocking
    // pQueueMain->messageLoop(); // blocking until event received and proceed
}
