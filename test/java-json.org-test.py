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
import shutil
import sys
import os
import conio
import re
import string

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
        for dirs in glob.glob(rsltPath + '/*/'):
            shutil.rmtree(dirs)


def _extractSchemaName(filename):
    return os.path.splitext(os.path.split(filename)[1])[0]


def _splitFile(filename):
    """Splits files starting where the regex finds anything that resembles
    W+.java where W is one or words.
    """
    if os.path.exists('target/'):
        shutil.rmtree('target/')
    os.makedirs('target/')
    if not os.path.exists('target/'):
        sys.exit()

    splitFile = 0
    with open(filename, 'r') as schemaOut:
        for line in schemaOut:
            match = re.search(r'\w+\.java', line)
            if match:
                if splitFile != 0:
                    splitFile.close()
                splitFile = open('target/' + match.group(), 'w')
            elif splitFile != 0:
                splitFile.write(line)


def genBinding(xsdfile, rsltPath, template):
    schemaName = _extractSchemaName(xsdfile)
    rsltPath = rsltPath + schemaName + '/'

    # generate directory for code
    if not os.path.exists(rsltPath):
        os.makedirs(rsltPath)

    # use tool to generate marshalling code
    cmd = '../xsdb ' + template + ' ' + xsdfile + ' >' + '/tmp/schema-out'
    out, err, retCode = consoleIO.call(cmd)
    if _handleError(err, retCode):
        return False

    _splitFile('/tmp/schema-out')

    cmd = 'cp java-json.org/pom.xml ' + rsltPath
    out, err, retCode = consoleIO.call(cmd)
    if _handleError(err, retCode):
        return False

    cmd = 'mkdir -p ' + rsltPath + 'src/main/java/com/mobitv/app/'
    out, err, retCode = consoleIO.call(cmd)
    if _handleError(err, retCode):
        return False

    cmd = 'mv target/* ' + rsltPath + 'src/main/java/com/mobitv/app/'
    out, err, retCode = consoleIO.call(cmd)
    if _handleError(err, retCode):
        return False
    return True


def genBindingTest(xsdfile, rsltPath, template):
    schemaName = _extractSchemaName(xsdfile)
    rsltPath = rsltPath + schemaName + '/'

    # use tool to generate test code
    cmd = '../xsdb ' + template + ' ' + xsdfile + ' >' + \
          'java-json.org/' + schemaName + \
          '/src/main/java/com/mobitv/app/RunTest.java'
    out, err, retCode = consoleIO.call(cmd)

    if _handleError(err, retCode):
        return False
    return True


def genTestCases(preconditionPath, postConditionPath, bindingTemplate, testTemplate):
    # Get *.xsd
    preconditionFileList = genPreconditionList(preconditionPath)

    for preconditionFile in preconditionFileList:
        consoleIO.stdout(consoleIO.ENDC, "generating test code from " + preconditionFile + ": ")

        if not genBinding(preconditionFile, postConditionPath, bindingTemplate):
            consoleIO.stdout(consoleIO.FAIL, " Fail\n")
            return False

        if not genBindingTest(preconditionFile, postConditionPath, testTemplate):
            consoleIO.stdout(consoleIO.FAIL, " Fail\n")
            return False
        else:
            consoleIO.stdout(consoleIO.OKGREEN, "Pass\n")
    return True


def execTests(xsdfiles, rsltPath):
    savedPath = os.getcwd()

    for exe in xsdfiles:
        schemaName = _extractSchemaName(exe)
        consoleIO.stdout(consoleIO.ENDC, "compiling test " + schemaName + ": ")

        os.chdir(rsltPath + "/" + schemaName)

        # TODO: how to check if failed build?
        cmd = 'mvn package'
        out, err, retCode = consoleIO.call(cmd)
        if _handleError(err, retCode):
            return False

        cmd = 'java -cp target/my-app-1.0-SNAPSHOT-jar-with-dependencies.jar com.mobitv.app.RunTest'
        out, err, retCode = consoleIO.call(cmd)

        NOT_FOUND = -1
        test_output = out.decode('utf-8')
        if len(test_output) == 0:
            consoleIO.stdout(consoleIO.WARNING, " Warning empty\n")
        elif test_output.find("true") == NOT_FOUND:
            consoleIO.stdout(consoleIO.FAIL, " Fail\n")
        elif test_output.find("true") > NOT_FOUND:  # found
            consoleIO.stdout(consoleIO.OKGREEN, "Pass\n")

        os.chdir(savedPath)

    return True


def runTest(testPath, bindingTemplate, cmdlineArguments):
    templateName = os.path.basename(bindingTemplate)
    destinationPath = templateName + "/"
    testTemplate = bindingTemplate + "-test"

    # clean test case build directory
    cleanTestDir(destinationPath)
    if "clean" == cmdlineArguments:
        return

    # aquire list of test cases
    xsdList = genPreconditionList(testPath)

    # generate tests
    if not genTestCases(testPath, destinationPath, bindingTemplate, testTemplate):
        consoleIO.stdout(consoleIO.FAIL, "Generating Tests Failed!")
        return

    # run tests
    if not execTests(xsdList, destinationPath):
        consoleIO.stdout(consoleIO.FAIL, "Fail\n")


def main(args):
    cmdlineArguments = args[1] if len(args) > 1 and "clean" == args[1] else ""
    runTest("xsd-positive/", "../templates/java-json.org", cmdlineArguments)


if __name__ == "__main__":
    main(sys.argv)
