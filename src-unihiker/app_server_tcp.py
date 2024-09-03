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
import json
import os
import logging
from circuits import Component, handler, Event, ipc
from circuits.net.sockets import TCPServer
from app_event import AppEvent
from const import AppConfig, EventValue, ConnectionType, ConnectionState, UserButton


class AppServerTcp(TCPServer):
    channel = 'tcp'

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.clients = set()

        log_level = logging.DEBUG
        logging.basicConfig(
            level=log_level, format="%(levelname)s: %(message)s",)
        self.logger = logging.getLogger(__name__)
        self.logger.debug(f'{self.name} init: pid={os.getpid()}')

        self.handlers = {
            EventValue.UserInput: self.handleEventUser,
            EventValue.DeviceReqUpdate: self.handleEventReqUpdate,
        }

    def started(self, *args):
        self.logger.debug(f'{self.name} started: pid={os.getpid()}')

    @handler('connect')
    def on_connect(self, sock, host, port):
        self.logger.debug(f'on_connect: sock={sock}, host={host}, port={port}')
        self.clients.add(sock)
        self.fire(AppEvent(EventValue.ConnectionUpdate,
                  ConnectionType.Tcp, ConnectionState.Connected), self.parent)

    @handler('disconnect')
    def on_disconnect(self, sock):
        self.logger.debug(f'on_disconnect: sock={sock}')
        self.clients.remove(sock)
        self.fire(AppEvent(EventValue.ConnectionUpdate, ConnectionType.Tcp,
                  ConnectionState.Disconnected), self.parent)

    @handler('read')
    def on_read(self, sock, data):
        """
        Read Event Handler

        This is fired by the underlying Socket Component when there has been
        new data read from the connected client.

        ..note :: By simply returning, client/server socket components listen
                  to ValueChagned events (feedback) to determine if a handler
                  returned some data and fires a subsequent Write event with
                  the value returned.
        """

        try:
            json_obj = json.loads(data)
            self.fire(AppEvent(EventValue.DataTcp, obj=json_obj), self.parent)
        except json.JSONDecodeError as e:
            self.logger.debug(f"JSON data is not well-formed: data={data}")
        except TypeError as e:
            self.logger.debug(
                f"Invalid input type for json.loads(): type(data)={type(data)}")
            # print("Invalid input type for json.loads:", e)

    @handler('AppEvent')
    def onAppEvent(self, event):
        if event.event in self.handlers:
            self.handlers[event.event](event)
        else:
            self.unsupportedHandler(event)

    def unsupportedHandler(self, event):
        self.logger.debug(
            f'{self.name}: unsupported event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')

    def handleEventReqUpdate(self, event):
        jsonObj = {"device": "lamp-esp",
                   "event": "req-update", "arg0": 0, "arg1": 0}
        josnStr = json.dumps(jsonObj)
        data = bytes(josnStr, encoding="utf-8")
        for client in self.clients:
            client.send(data)

    def handleEventUser(self, event):
        self.logger.debug(
            f'{self.name}: handleEventUser: event={event.event}, arg0={event.arg0}, arg1={event.arg1}, obj={event.obj}')

        arg0 = int(event.arg0)  # buttonId
        jsonObj = {"device": "lamp-esp",
                   "event": "user-click", "arg0": arg0, "arg1": 0}
        self.logger.debug(f'jsonObj={jsonObj}')
        # jsonObj = { "device":"fan", "event": "user-click", "arg0": 0, "arg1": arg1 }
        josnStr = json.dumps(jsonObj)
        data = bytes(josnStr, encoding="utf-8")
        for client in self.clients:
            client.send(data)
