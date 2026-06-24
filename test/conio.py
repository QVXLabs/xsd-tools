# Copyright: (c)2012 QVXLabs LLC
#
# This file is part of xsd-tools.
#
# xsd-tools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# xsd-tools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.


import sys
import signal
import subprocess


class conio:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    SIGNALS_TO_NAMES_DICT = dict((getattr(signal, n), n) \
                                 for n in dir(signal) if n.startswith('SIG') and '_' not in n)

    def __init__(self):
        return

    def enableColors(self):
        self.HEADER = '\033[95m'
        self.OKBLUE = '\033[94m'
        self.OKGREEN = '\033[92m'
        self.WARNING = '\033[93m'
        self.FAIL = '\033[91m'
        self.ENDC = '\033[0m'

    def disableColors(self):
        self.HEADER = ''
        self.OKBLUE = ''
        self.OKGREEN = ''
        self.WARNING = ''
        self.FAIL = ''
        self.ENDC = ''

    def stdout(self, color, message):
        sys.stdout.write(color + str(message) + self.ENDC)

    def call(self, cmdLine, workDir=None):
        if None == workDir:
            prcssStrm = subprocess.Popen(cmdLine,
                                         shell=True,
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
        else:
            prcssStrm = subprocess.Popen(cmdLine,
                                         shell=True,
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE,
                                         cwd=workDir)
        rslt, error = prcssStrm.communicate()
        # handle a signal error
        if 0 > prcssStrm.returncode:
            return rslt, self.SIGNALS_TO_NAMES_DICT[-prcssStrm.returncode] + "\n" + error, prcssStrm.returncode
        return rslt, error, prcssStrm.returncode
