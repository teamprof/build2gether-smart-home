# Copyright 2024 teamprof.net@gmail.com
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
# PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# import bluetooth
# import json
import os
import logging
import asyncio
from multiprocessing import Queue
from enum import Enum

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

from app_event import AppEvent
from const import AppConfig, EventValue, ConnectionType, ConnectionState, UserButton


LAMP_DEVICE_NAME = 'nrf52840-lamp'
BT_UUID_LED_SERV_VAL = "00001523-1212-efde-1523-785feabcd123"
BT_UUID_LED_CHRC_VAL = "00001525-1212-efde-1523-785feabcd123"

BLE_SCAN_RETRY_INTERVAL = 5.0


class State(Enum):
    Unknown = -1
    Off = 0
    On = 1

    def __int__(self):
        return self.value

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_


class AppClientBle():
    def __init__(self):
        super().__init__()
        log_level = logging.DEBUG   # log_level = logging.INFO
        logging.basicConfig(
            level=log_level, format="%(levelname)s: %(message)s",)
        self.logger = logging.getLogger(__name__)

        self.logger.debug(f'{__name__}: __init__: pid={os.getpid()}')

        self.appContext = None
        self.queue = Queue()

        self.isConnected = False
        self.client = None
        self.state = State.Off
        self.eventDisconnected = asyncio.Event()

        self.handlers = {
            EventValue.UserInput: self.handleEventUser,
            EventValue.DeviceReqUpdate: self.handleEventReqUpdate,
            EventValue.DataBle: self.handleEventData,
        }

    async def onMessage(self, msg):
        if isinstance(msg, AppEvent):
            self.logger.debug(
                f'{__name__}: onMessage: event={msg.event}, arg0={msg.arg0}, arg1={msg.arg1}, obj={msg.obj}')
            if msg.event in self.handlers:
                await self.handlers[msg.event](msg)
            else:
                self.logger.debug(
                    f'{__name__}: unsupported event={msg.event}, arg0={msg.arg0}, arg1={msg.arg1}, obj={msg.obj}')
        else:
            self.logger.debug(f'{__name__}: onMessage: msg={msg}')

    async def messageLoopForever(self):
        while True:
            if not self.queue.empty():
                await self.onMessage(self.queue.get())
            else:
                await asyncio.sleep(1)

    async def connectLoopForever(self):
        while True:
            device = await self.scan()
            if device is not None:
                await self.connect(device)
            else:
                self.logger.debug(
                    f'{__name__}: connectLoopForever: retry after {BLE_SCAN_RETRY_INTERVAL} seconds')
                await asyncio.sleep(BLE_SCAN_RETRY_INTERVAL)

    async def loopForever(self):
        await asyncio.gather(
            self.messageLoopForever(),
            self.connectLoopForever()
        )

    def start(self, ctx):
        self.appContext = ctx
        asyncio.run(self.loopForever())

    async def connect(self, device):
        self.logger.debug(f'{__name__}: connect: device = {device}')
        async with BleakClient(
            device,
            disconnected_callback=self.onDisconnected
        ) as client:
            self.client = client
            self.isConnected = True

            self.postMainEvent(
                EventValue.ConnectionUpdate, ConnectionType.Ble, ConnectionState.Connected)

            # BlueZ doesn't have a proper way to get the MTU, so we have this hack.
            # If this doesn't work for you, you can set the client._mtu_size attribute
            # to override the value instead.
            if client._backend.__class__.__name__ == "BleakClientBlueZDBus":
                await client._backend._acquire_mtu()

            await self.client.start_notify(BT_UUID_LED_CHRC_VAL, self.onNotification)

            # self.eventDisconnected.clear()
            # await self.eventDisconnected.wait()

            await self.client.stop_notify(BT_UUID_LED_CHRC_VAL)

            self.client = None
            self.isConnected = False
            self.postMainEvent(EventValue.ConnectionUpdate,
                               ConnectionType.Ble, ConnectionState.Disconnected)

    async def scan(self):
        self.logger.debug(f'{__name__}: scanning ...')
        device = await BleakScanner.find_device_by_name(
            LAMP_DEVICE_NAME, cb=dict(use_bdaddr=False)
        )
        self.logger.debug(f'{__name__}: scan done: device={device}')
        return device

    def onDisconnected(self, client):
        self.logger.debug(f'{__name__}: onDisconnected')
        self.isConnected = False
        self.eventDisconnected.set()

    def onNotification(self, characteristic: BleakGATTCharacteristic, data: bytearray):
        self.logger.debug(
            f'{__name__} onNotification: {characteristic.uuid}: {data.hex()}')
        self.postEvent(EventValue.DataBle, obj=data)

    async def read(self):
        self.logger.debug(f'{__name__}: read()')
        if self.client and self.client.is_connected:
            return await self.client.read_gatt_char(BT_UUID_LED_CHRC_VAL)
        else:
            self.logger.warn(f'{__name__}: read() - client is not available')
            return None

    async def write(self, data):
        self.logger.debug(f'{__name__}: write() - data = {data}')
        if self.client and self.client.is_connected:
            await self.client.write_gatt_char(BT_UUID_LED_CHRC_VAL, data, response=False)
        else:
            self.logger.warn(f'{__name__}: write() - client is not available')

    async def handleEventData(self, event):
        self.logger.debug(
            f'{__name__}: handleEventData: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        data = event.obj
        if data and isinstance(data, bytearray) and len(data) > 0:
            state = data[0]
            self.state = State(state)
            self.postMainState(state)
        else:
            self.logger.warn(
                f'{__name__}: handleEventData() - unsupported data')

    async def handleEventReqUpdate(self, event):
        self.logger.debug(
            f'{__name__}: handleEventReqUpdate: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        data = await self.read()
        if isinstance(data, bytearray) and len(data) > 0:
            state = data[0]
            self.state = State(state)
            self.logger.debug(
                f'{__name__}: handleEventReqUpdate: state={state}')
            self.postMainState(int(self.state))
        else:
            self.state = State.Unknown
            self.logger.warn(
                f'{__name__}: handleEventReqUpdate: unsuported data: type(data)={type(data)}, data={data}')

    async def handleEventUser(self, event):
        self.logger.debug(
            f'{__name__}: handleEventUser: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        button = UserButton(event.arg0)
        if button == UserButton.LampNrfOnOff:
            self.state = State.On if self.state == State.Off else State.Off

            # write state to characteristic
            data = bytearray(1)
            data[0] = self.state.value
            await self.write(data)
            # await asyncio.sleep(0.1)

        else:
            self.logger.warn(
                f'{__name__}: handleEventUser() - unsupported button={button}')

    def postEvent(self, event, arg0=0, arg1=0, obj=None):
        self.queue.put(AppEvent(event, arg0, arg1, obj))

    def postMainEvent(self, event, arg0, arg1):
        self.appContext.queueMain.put(AppEvent(event, arg0, arg1))

    def postMainDataEvent(self, event, arg0, arg1):
        jsonObj = {"device": "lamp-nrf", "event": event,
                   "arg0": arg0, "arg1": arg1}
        # msg = BleMessage.from_bytes(jsonObj)
        self.appContext.queueMain.put(
            AppEvent(EventValue.DataBle, obj=jsonObj))

    def postMainState(self, state):
        self.postMainDataEvent("update", 0, state)
