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
import os

from circuits import Timer, Event
from circuits.core.events import started
from circuits.io import File

from app_server_tcp import AppServerTcp
from app_gui import AppGui
from app_event import AppEvent
from const import AppConfig, EventValue, ConnectionType, ConnectionState, UserButton
from pyprof import PyProfRoot


class AppHome(PyProfRoot):
    channel = 'app'

    def __init__(self, ctx):
        super().__init__()
        self.logger.debug(f'{self.name} init: pid={os.getpid()}')

        self.appContext = ctx

        self.gui = AppGui().register(self)
        self.tcp = AppServerTcp(
            (AppConfig.ServerIP, AppConfig.ServerPort)).register(self)

        self.handlers = {
            EventValue.UserInput: self.handleEventUser,
            EventValue.ConnectionUpdate: self.handleEventConnection,
            EventValue.DeviceReqUpdate: self.handleEventReqUpdate,
            EventValue.DataTcp: self.handleEventDataTcp,
            EventValue.DataBle: self.handleEventDataBle,
        }
        self.switchButton = {
            UserButton.LampNrfOnOff: self.handleUserLampNrfOnOff,
            UserButton.LampEspOn: self.handleUserLampEspOn,
        }

        self.tcpState = ConnectionState.Disconnected
        self.bleState = ConnectionState.Disconnected

    def started(self, *args):
        self.logger.debug(f'{self.name} started: pid={os.getpid()}')
        self.fire(started(*args), self.gui)
        self.fire(started(*args), self.tcp)

        self.postEvent(EventValue.ConnectionUpdate,
                       ConnectionType.Tcp, self.tcpState, dest=self.gui)
        self.postEvent(EventValue.ConnectionUpdate,
                       ConnectionType.Ble, self.bleState, dest=self.gui)

        Timer(0.5, Event.create('pollIpcMessage', 1),
              persist=True).register(self)

    def ready(self, *args):
        self.logger.debug(f'{self.name} ready: pid={os.getpid()}')

    def handleEventConnection(self, event):
        self.logger.debug(
            f'{self.name}: handleEventConnection: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        connectionType = event.arg0
        connectionState = event.arg1
        if connectionType == ConnectionType.Tcp:
            self.tcpState = connectionState
        elif connectionType == ConnectionType.Ble:
            self.bleState = connectionState
        else:
            self.logger.debug(
                f'{self.name}: handleEventConnection: unsupported connection={connectionType}')
        self.postEvent(event.event, event.arg0, event.arg1, dest=self.gui)

    def handleEventDataTcp(self, event):
        self.logger.debug(
            f'{self.name}: handleEventDataTcp: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')

        json = event.obj
        if json is None:
            return

        self.postEvent(EventValue.DeviceUpdate, 0, 0, json, dest=self.gui)

    def handleEventDataBle(self, event):
        self.logger.debug(
            f'{self.name}: handleEventDataBle: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')

        json = event.obj
        if json is None:
            return

        self.postEvent(EventValue.DeviceUpdate, 0, 0, json, dest=self.gui)

    def handleEventReqUpdate(self, event):
        if event.arg0 == ConnectionType.Tcp:
            self.postEvent(event.event, event.arg0, event.arg1,
                           event.obj, dest=self.tcp)
        elif event.arg0 == ConnectionType.Ble:
            self.appContext.queueBle.put(
                AppEvent(event.event, event.arg0, event.arg1, event.obj))

    def handleEventUser(self, event):
        self.logger.debug(
            f'{self.name}: handleEventUser: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        self.switchButton.get(event.arg0, self.unsupportedHandler)(event)

    def handleUserLampEspOn(self, event):
        self.logger.debug(
            f'{self.name}: handleUserLampEspOn: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        if self.tcpState == ConnectionState.Connected:
            self.postEvent(event.event, event.arg0, event.arg1,
                           event.obj, dest=self.tcp)
        else:
            self.logger.debug(f'{self.name}: No TCP client connected!')

    def handleUserLampNrfOnOff(self, event):
        self.logger.debug(f'{self.name}: handleUserLampNrfOnOff: LampNrfOnOff')
        if self.bleState == ConnectionState.Connected:
            self.appContext.queueBle.put(
                AppEvent(event.event, event.arg0, event.arg1, event.obj))
        else:
            self.logger.debug(f'{self.name}: No BLE peripheral connected!')

    def pollIpcMessage(self, arg0):
        if not self.appContext.queueMain.empty():
            msg = self.appContext.queueMain.get()
            if isinstance(msg, AppEvent):
                self.fire(msg)
            else:
                self.logger.debug(
                    f'{self.name} pollIpcMessage: msg={msg}')
