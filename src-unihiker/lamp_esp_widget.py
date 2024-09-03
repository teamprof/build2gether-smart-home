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
from enum import Enum
from unihiker import GUI
from const import EventValue, ConnectionType, ConnectionState, UserButton
from pyprof import PyProfRoot


class State(Enum):
    Off = 0
    ColorWhite = 1
    ColorYellow = 2
    ColorRed = 3
    On = 4

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_


class LampEspWidget(PyProfRoot):
    channel = 'lamp-esp'

    def __init__(self, gui: GUI):
        super().__init__()
        self.logger.debug(f'{self.name} init: pid={os.getpid()}')

        self.handlers = {
            EventValue.ConnectionUpdate: self.handleEventConnection,
            EventValue.DeviceUpdate: self.handleEventDevice,
        }

        self.isConnected = False
        self.state = State.Off
        self.group = gui.draw_round_rect(
            x=10, y=60, w=220, h=120, r=8, width=3, color=self.uiColor())
        self.icon = gui.draw_image(
            x=20, y=88, w=64, h=64, image='assets/lightbulb-off.png', onclick=self.uiClickIcon)
        self.title = gui.draw_text(
            x=88, y=88, text='SEEED ESP32S3', font_size=12)
        self.text = gui.draw_text(x=90, y=128, text='')

    def started(self, *args):
        self.logger.debug(f'{self.name} started: pid={os.getpid()}')

    def ready(self, *args):
        self.logger.debug(f'{self.name} ready: pid={os.getpid()}, args={args}')

    def handleEventConnection(self, event):
        connectionType = event.arg0
        connectionState = event.arg1
        if connectionType != ConnectionType.Tcp:
            return

        self.isConnected = connectionState == ConnectionState.Connected
        self.uiUpdateGroup()
        if self.isConnected:
            self.postEvent(EventValue.DeviceReqUpdate,
                           ConnectionType.Tcp, dest=self.parent)

    def handleEventDevice(self, event):
        self.logger.debug(
            f'{self.name}: handleEventDevice: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')
        jsonObj = event.obj
        device = jsonObj['device']
        if device is None or device != 'lamp-esp':
            return

        event = jsonObj['event']
        if event is None:
            return
        if event == 'update':
            state = jsonObj['arg1']
            if State.has_value(state):
                self.state = State(state)
                self.uiUpdateText()
                self.uiUpdateIcon()
            else:
                self.logger.debug(
                    f'{self.name}: unsupported lamp state: {state}')

    def uiUpdateIcon(self):
        icon = 'assets/lightbulb-on.png' if self.isConnected and self.state != State.Off else 'assets/lightbulb-off.png'
        self.icon.config(image=icon)

    def uiUpdateText(self):
        # Lamp: off / white / yellow / red / disconnect
        text = ''
        if not self.isConnected:
            text += 'disconnect'
        elif self.state == State.Off:
            text += 'off'
        elif self.state == State.ColorWhite:
            text += 'white'
        elif self.state == State.ColorYellow:
            text += 'yellow'
        elif self.state == State.ColorRed:
            text += 'red'
        elif self.state == State.On:
            text += 'on'
        else:
            text += 'unknown'
        self.title.config(color=self.uiColor())
        self.text.config(text=text, color=self.uiColor())

    def uiUpdateGroup(self):
        self.group.config(color=self.uiColor())
        self.uiUpdateIcon()
        self.uiUpdateText()

    def uiColor(self):
        return 'blue' if self.isConnected else 'grey'

    def uiClickIcon(self):
        self.logger.debug(
            f"{self.name}: uiOnClickIcon(): self.isConnected={self.isConnected}")
        if self.isConnected:
            self.postEvent(EventValue.UserInput,
                           UserButton.LampEspOn, dest=self.parent)
