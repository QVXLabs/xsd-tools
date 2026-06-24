#!/usr/bin/python3

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
import test_framework


def main(arg):
    op = arg[1] if len(arg) > 1 and "clean" == arg[1] else ""
    test_framework.runTest("xml_",
                           "xsd-positive/",
                           "../templates/c-xml-expat-dom.template",
                           op)


if __name__ == "__main__":
    main(sys.argv)
