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
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#include <map>
#include "./lib/arduprof/ArduProf.h"

class MainTask : public MessageBus
{
public:
    static MainTask *getInstance(void);
    virtual void start(void *);
    virtual void onMessage(const Message &msg);
    bool getLedState(void);

protected:
    typedef void (MainTask::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

private:
    MainTask();
    static MainTask *_instance;

    bool _ledState;
    void setLedState(bool ledState);
    void toggleLedState(void);

    ///////////////////////////////////////////////////////////////////////////
    // event handler
    ///////////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventUserInput)
    __EVENT_FUNC_DECLARATION(EventBleConnection)
    __EVENT_FUNC_DECLARATION(EventBleLed)
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    extern void postMainEvent(int16_t event, int16_t iParam, uint16_t uParam, uint32_t lParam);

#ifdef __cplusplus
}
#endif
