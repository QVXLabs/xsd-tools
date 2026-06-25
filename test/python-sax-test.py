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

import glob
import difflib
import os
import shutil
import sys
import os
import conio

consoleIO = conio.conio()


def _handleError(stdErr, errCode):
    fail = (errCode and 0 != len(stdErr))
    if fail:
        consoleIO.stdout(consoleIO.FAIL, stdErr)
    return fail


def genPreconditionList(preconditionPath):
    return glob.glob(preconditionPath + "*.xsd")


def cleanTestDir(rsltPath):
    if os.path.exists(rsltPath):
        shutil.rmtree(rsltPath)
    os.makedirs(rsltPath)


def _extractSchemaName(filename):
    return os.path.splitext(os.path.split(filename)[1])[0]


def genBinding(srcPrefix, xsdfile, rsltPath, template):
    schemaName = _extractSchemaName(xsdfile)
    rsltPath = rsltPath + schemaName + '/'
    rsltFile = rsltPath + srcPrefix + schemaName + '.py'
    # generate directory for code
    if not os.path.exists(rsltPath):
        os.makedirs(rsltPath)
    # use tool to generate marshalling code
    cmd = '../xsdb ' + template + ' ' + xsdfile + ' >' + rsltFile
    out, err, retCode = consoleIO.call(cmd)
    if _handleError(err, retCode):
        return False
    return True


def genBindingTest(xsdfile, rsltPath, template):
    schemaName = _extractSchemaName(xsdfile)
    rsltPath = rsltPath + schemaName + '/'
    rsltTstFile = rsltPath + schemaName + 'test.py'
    # use tool to generate test code
    cmd = '../xsdb ' + template + ' ' + xsdfile + ' >' + rsltTstFile
    out, err, retCode = consoleIO.call(cmd)
    if _handleError(err, retCode):
        return False
    return True


def genTestCases(srcPrefix, preconditionPath, postConditionPath, bindingTemplate, testTemplate):
    preconditionFileLst = genPreconditionList(preconditionPath)
    for preconditionFile in preconditionFileLst:
        consoleIO.stdout(consoleIO.ENDC, "generating test code from " + preconditionFile + ": ")
        if not genBinding(srcPrefix, preconditionFile, postConditionPath, bindingTemplate):
            consoleIO.stdout(consoleIO.FAIL, " Fail\n")
            return False
        if not genBindingTest(preconditionFile, postConditionPath, testTemplate):
            consoleIO.stdout(consoleIO.FAIL, " Fail\n")
            return False
        else:
            consoleIO.stdout(consoleIO.OKGREEN, "Pass\n")
    return True


def execTests(xsdfiles, rsltPath):
    for exe in xsdfiles:
        schemaName = _extractSchemaName(exe)
        exefile = os.path.splitext(os.path.split(exe)[1])[0] + 'test.py'
        consoleIO.stdout(consoleIO.ENDC, "executing test " + exefile + ": ")
        cmd = 'python3 ' + rsltPath + schemaName + "/" + exefile
        std, err, retcode = consoleIO.call(cmd)
        if _handleError(err, retcode):
            return False
        else:
            consoleIO.stdout(consoleIO.OKGREEN, "Pass\n")
    return True


def runTest(srcPrefix, testPath, bindingTemplate, op):
    templateName = os.path.split(bindingTemplate)[1]
    dstPath = templateName + "/"
    testTemplate = bindingTemplate + "-test"
    # clean test case build directory
    cleanTestDir(dstPath)
    if "clean" == op:
        return
    # aquire list of test cases
    xsdLst = genPreconditionList(testPath)
    # generate tests
    if not genTestCases(srcPrefix, testPath, dstPath, bindingTemplate, testTemplate):
        return
    # run tests
    if not execTests(xsdLst, dstPath):
        consoleIO.stdout(consoleIO.FAIL, "Fail\n")


def main(args):
    op = args[1] if len(args) > 1 and "clean" == args[1] else ""
    runTest("", "xsd-positive/", "../templates/python-sax", op)


if __name__ == "__main__":
    main(sys.argv)
