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
#include "ArduProfFreeRTOS.h"
#include "./AppEvent.h"

class TaskTcpClient;

class ThreadPanel : public ardufreertos::ThreadBase
{
public:
    typedef enum _ConnectionState
    {
        Undefine = 0,
        Disconnect,
        Connecting,
        Connect
    } ConnectionState;

    ThreadPanel();
    static ThreadPanel *getInstance(void);

    virtual void start(void *);
    virtual void onMessage(const Message &msg);

protected:
    typedef void (ThreadPanel::*handlerFunc)(const Message &);
    std::map<int16_t, handlerFunc> handlerMap;

private:
    friend TaskTcpClient;

    static ThreadPanel *_instance;
    ardufreertos::PeriodicTimer _timer1Hz;
    bool _isNetworkAvailable;
    ConnectionState _connectionState;
    int _sock;
    TaskHandle_t _hTaskConnect;

    virtual void setup(void);
    void handlerUpdateDevice(const Message &msg);
    void handlerTcpConnection(const Message &msg);
    void handlerUserCommand(const Message &msg);
    void handlerNetworkAvailable(const Message &msg);
    void handlerSoftwareTimer(TimerHandle_t xTimer);

    void closeSocket(void);

    ///////////////////////////////////////////////////////////////////////
    // declare event handler
    ///////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DECLARATION(EventApp)
    __EVENT_FUNC_DECLARATION(EventSystem)
    __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
};