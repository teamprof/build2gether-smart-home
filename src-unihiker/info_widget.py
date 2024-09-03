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
import platform
from enum import Enum
from unihiker import GUI
from pyprof import PyProfRoot


class InfoWidget(PyProfRoot):
    channel = 'info'

    def __init__(self, gui: GUI):
        super().__init__()
        self.logger.debug(f'{self.name} init: pid={os.getpid()}')

        hostname = platform.node()
        self.text = gui.draw_text(
            x=12, y=20, text=f'host: {hostname}', color='blue', origin='left')

    def started(self, *args):
        self.logger.debug(f'{self.name} started: pid={os.getpid()}')

    def ready(self, *args):
        self.logger.debug(f'{self.name} ready: pid={os.getpid()}, args={args}')
