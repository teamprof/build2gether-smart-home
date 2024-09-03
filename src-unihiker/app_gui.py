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
import sys
import os
from unihiker import GUI
from circuits import Component, handler, Event, Timer
from circuits.core.events import started
from circuits.io import File
from info_widget import InfoWidget
from lamp_nrf_widget import LampNrfWidget
from lamp_esp_widget import LampEspWidget
from const import EventValue, ConnectionType, ConnectionState, UserButton
from pyprof import PyProfRoot


class timerEvent(Event):
    """timerEvent"""


class AppGui(PyProfRoot):
    channel = 'gui'

    def __init__(self):
        super().__init__()
        self.logger.debug(f'{self.name} init: pid={os.getpid()}')
        self.handlers = {
            EventValue.Timer: self.handleEventTimer,
            EventValue.UserInput: self.handleEventUser,
            EventValue.DeviceReqUpdate: self.handleEventReqUpdate,
            EventValue.ConnectionUpdate: self.handleEventConnection,
            EventValue.DeviceUpdate: self.handleEventDevice,
        }
        self.switchConnection = {
            ConnectionType.Tcp: self.tcpStateUpdate,
            ConnectionType.Ble: self.bleStateUpdate,
        }

        gui = GUI()
        self.info = InfoWidget(gui).register(self)
        self.lampEsp = LampEspWidget(gui).register(self)
        self.lampNrf = LampNrfWidget(gui).register(self)

        File(sys.stdin, channel="stdin").register(self)

        gui.on_a_click(lambda: self.postEvent(
            EventValue.UserInput, UserButton.LampEspOn, dest=self.parent))
        gui.on_b_click(lambda: self.postEvent(
            EventValue.UserInput, UserButton.LampNrfOnOff, dest=self.parent))

    def timerEvent(self):
        self.logger.debug(f'{self.name}: timerEvent: pid={os.getpid()}')

    def started(self, *args):
        self.logger.debug(f'{self.name} started: pid={os.getpid()}')
        self.fire(started(*args), self.info)
        self.fire(started(*args), self.lampEsp)
        self.fire(started(*args), self.lampNrf)

    def handleEventTimer(self, event):
        self.logger.debug(
            f'{self.name}: handleEventTimer: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')

    def handleEventReqUpdate(self, event):
        self.postEvent(event.event, event.arg0, event.arg1,
                       event.obj, dest=self.parent)

    def handleEventUser(self, event):
        self.logger.debug(
            f'{self.name}: handleEventUser: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        self.postEvent(event.event, event.arg0, event.arg1,
                       event.obj, dest=self.parent)

    def handleEventConnection(self, event):
        connectionType = event.arg0
        self.switchConnection.get(
            connectionType, self.unsupportedHandler)(event)

    def handleEventDevice(self, event):
        self.logger.debug(
            f'{self.name}: handleEventDevice: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        json = event.obj
        device = json['device']
        if device is None:
            return
        if device == 'lamp-esp':
            self.postEvent(EventValue.DeviceUpdate, 0,
                           0, json, dest=self.lampEsp)
        elif device == 'lamp-nrf':
            self.postEvent(EventValue.DeviceUpdate, 0,
                           0, json, dest=self.lampNrf)

    def tcpStateUpdate(self, event):
        self.logger.debug(
            f'{self.name}: ConnectionStateUpdate: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        connectionState = event.arg1
        if connectionState == ConnectionState.Connected:
            self.logger.debug(
                f'{self.name}: tcpStateUpdate: ConnectionState.Connected')
            self.postEvent(event.event, event.arg0, event.arg1,
                           event.obj, dest=self.lampEsp)
        elif ConnectionState.Disconnected:
            self.logger.debug(
                f'{self.name}: tcpStateUpdate: ConnectionState.Disconnected')
            self.postEvent(event.event, event.arg0, event.arg1,
                           event.obj, dest=self.lampEsp)
        else:
            self.logger.debug(
                f'{self.name}: tcpStateUpdate: unsupported state={connectionState}')

    def bleStateUpdate(self, event):
        self.logger.debug(
            f'{self.name}: ConnectionStateUpdate: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        connectionState = event.arg1
        if connectionState == ConnectionState.Connected:
            self.logger.debug(
                f'{self.name}: bleStateUpdate: ConnectionState.Connected')
            self.postEvent(event.event, event.arg0, event.arg1,
                           event.obj, dest=self.lampNrf)
        elif ConnectionState.Disconnected:
            self.logger.debug(
                f'{self.name}: bleStateUpdate: ConnectionState.Disconnected')
            self.postEvent(event.event, event.arg0, event.arg1,
                           event.obj, dest=self.lampNrf)
        else:
            self.logger.debug(
                f'{self.name}: bleStateUpdate: unsupported state={connectionState}')

    @handler("read", channel="stdin")
    def on_read_stdin(self, data):
        """mock gui click by typing lamp<ENTER> / fanon<ENTER> / fanoff<ENTER> on keyboard"""
        string_data = data.decode('utf-8').rstrip('\n')
        if string_data == "lamp-nrf":
            self.postEvent(EventValue.UserInput,
                           UserButton.LampNrfOnOff, dest=self.parent)
        elif string_data == "lamp-esp":
            self.postEvent(EventValue.UserInput,
                           UserButton.LampEspOn, dest=self.parent)
