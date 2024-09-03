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
from enum import Enum


class AppConfig():
    ServerIP = '0.0.0.0'
    ServerPort = 8080


class EventValue(Enum):
    Null = 0
    Timer = 1

    # arg0=wifi/ble, arg1=connected/disconnected, obj=remote ip:port, ...
    ConnectionUpdate = 10
    DeviceUpdate = 11       # matterDevice: obj=...
    DeviceReqUpdate = 12

    DataTcp = 20
    DataBle = 21

    # ScanResult = 30
    # ConnectResult = 31

    UserInput = 40          # arg0=<UserButton>


class ConnectionType(Enum):
    Tcp = 1
    Ble = 2


class ConnectionState(Enum):
    Connected = 1
    Disconnected = 2


class UserButton(Enum):
    LampNrfOnOff = 1
    LampEspOn = 2

    def __int__(self):
        return self.value
