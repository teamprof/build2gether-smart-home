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
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>

#include "./main.h"
#include "./ble_task.h"
#include "./AppEvent.h"
#include "./BleConst.h"
#include "./LedConst.h"
#include "./led_service.h"

#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main, LOG_LEVEL);

#define USER_LED DK_LED1
#define CON_STATUS_LED DK_LED4

#define USER_BUTTON DK_BTN1_MSK

static void onLedChrcWrite(bool led_state)
{
	postMainEvent(EventBleLed, ActionSetLed, 0, led_state);
}

static bool onLedChrcRead(void)
{
	return MainTask::getInstance()->getLedState();
}

static struct led_service_cb led_callbacks = {
	.write = onLedChrcWrite,
	.read = onLedChrcRead,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	LOG_DBG("button_state=%u, has_changed=%u", button_state, has_changed);
	if (!button_state && has_changed & USER_BUTTON)
	{
		MainTask::getInstance()->postEvent(EventUserInput, ActionToggleLed);
	}
}

///////////////////////////////////////////////////////////////////////
#define TASK_QUEUE_SIZE 32 // message queue size for app task
K_MSGQ_DEFINE(taskQueue, sizeof(Message), TASK_QUEUE_SIZE, alignof(uint32_t));

MainTask *MainTask::_instance = NULL;

///////////////////////////////////////////////////////////////////////
MainTask::MainTask() : MessageBus(&taskQueue), _ledState(false)
{
	handlerMap = {
		__EVENT_MAP(MainTask, EventUserInput),
		__EVENT_MAP(MainTask, EventBleConnection),
		__EVENT_MAP(MainTask, EventBleLed),
		__EVENT_MAP(MainTask, EventNull), // {EventNull, &MainTask::handlerEventNull},
	};
}

MainTask *MainTask::getInstance(void)
{
	if (!_instance)
	{
		static MainTask mainTask;
		_instance = &mainTask;
	}
	return _instance;
}

void MainTask::start(void *ctx)
{
	MessageBus::start(ctx);

	dk_set_led(USER_LED, _ledState);
	dk_set_led_off(CON_STATUS_LED);
}

void MainTask::onMessage(const Message &msg)
{
	auto func = handlerMap[msg.event];
	if (func)
	{
		(this->*func)(msg);
	}
	else
	{
		LOG_DBG("Unsupported event=%hd, iParam=%hd, uParam=%hu, lParam=%u", msg.event, msg.iParam, msg.uParam, msg.lParam);
	}
}

///////////////////////////////////////////////////////////////////////
bool MainTask::getLedState(void)
{
	return _ledState;
}
void MainTask::setLedState(bool ledState)
{
	_ledState = ledState;
	dk_set_led(USER_LED, ledState);
	ble_led_notify(ledState);
}

void MainTask::toggleLedState(void)
{
	setLedState(!_ledState);
}

///////////////////////////////////////////////////////////////////////
__EVENT_FUNC_DEFINITION(MainTask, EventUserInput, msg) // void MainTask::handlerEventUserInput(const Message &msg)
{
	LOG_DBG("EventUserInput(%hd), iParam=%hd, uParam=%hu, lParam=%u", msg.event, msg.iParam, msg.uParam, msg.lParam);
	auto userAction = msg.iParam;
	switch (userAction)
	{
	case ActionToggleLed:
		toggleLedState();
		break;
	default:
		LOG_DBG("Unsupported userAction=%hd", userAction);
		break;
	}
}
__EVENT_FUNC_DEFINITION(MainTask, EventBleConnection, msg) // void MainTask::handlerEventBleConnection(const Message &msg)
{
	LOG_DBG("EventBleConnection(%hd), iParam=%hd, uParam=%hu, lParam=%u", msg.event, msg.iParam, msg.uParam, msg.lParam);
	auto state = msg.iParam;
	switch (state)
	{
	case BleDisconnected:
		dk_set_led_off(CON_STATUS_LED);
		break;

	case BleConnected:
		dk_set_led_on(CON_STATUS_LED);
		break;

	default:
		LOG_DBG("unsupported state=%hd", state);
		break;
	}
}
__EVENT_FUNC_DEFINITION(MainTask, EventBleLed, msg) // void MainTask::handlerEventBleLed(const Message &msg)
{
	LOG_DBG("EventBleLed(%hd), iParam=%hd, uParam=%hu, lParam=%u", msg.event, msg.iParam, msg.uParam, msg.lParam);
	auto action = msg.iParam;
	bool state = (bool)(msg.lParam);
	switch (action)
	{
	case ActionSetLed:
		setLedState(state);
		break;
	default:
		LOG_DBG("unsupported action=%hd, state=%u", action, state);
		break;
	}
}
__EVENT_FUNC_DEFINITION(MainTask, EventNull, msg) // void MainTask::handlerEventNull(const Message &msg)
{
	LOG_DBG("EventNull(%hd), iParam=%hd, uParam=%hu, lParam=%u", msg.event, msg.iParam, msg.uParam, msg.lParam);
}

///////////////////////////////////////////////////////////////////////
void postMainEvent(int16_t event, int16_t iParam, uint16_t uParam, uint32_t lParam)
{
	LOG_DBG("event=%hd, iParam=%hd, uParam=%hu, lParam=%u", event, iParam, uParam, lParam);
	MainTask::getInstance()->postEvent(event, iParam, uParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
int main(void)
{
	int err = dk_leds_init();
	if (err)
	{
		LOG_ERR("dk_leds_init() returns %d", err);
		return -1;
	}

	err = dk_buttons_init(button_changed);
	if (err)
	{
		LOG_WRN("dk_buttons_init() returns %d", err);
	}

	bool bleInit = ble_init(&led_callbacks);
	LOG_DBG("ble_init() returns %d", bleInit);

	auto mainTask = MainTask::getInstance();
	mainTask->start(nullptr);
	// mainTask->postEvent(EventNull);
	mainTask->messageLoopForever();

	return 0;
}
