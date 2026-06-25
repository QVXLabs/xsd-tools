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
import conio
import re

consoleIO = conio.conio()

_logSuccess = lambda msg: consoleIO.stdout(consoleIO.OKGREEN, msg)
_logFailure = lambda msg: consoleIO.stdout(consoleIO.FAIL, msg)
_logMessage = lambda msg: consoleIO.stdout(consoleIO.ENDC, msg)

_extractName = lambda name: os.path.splitext(os.path.split(name)[1])[0]


def _logCondition(rslt, msg_tbl=("Fail\n", "Pass\n")):
    (_logFailure, _logSuccess)[rslt](msg_tbl[rslt])
    return rslt


def _handleError(stdErr, errCode):
    return _logCondition(not (errCode and 0 != len(stdErr)), (stdErr, ""))


def _csplit(input, pattern, output_path):
    out_file = None
    regex = re.compile(pattern)
    for line in input:
        result = regex.search(line)
        if result:
            if out_file:
                out_file.close()
            filename = os.path.join(output_path, result.group(1))
            out_file = open(filename, "w")
        elif out_file:
            out_file.write(line)
    return True


def _execu(command, working_dir=None):
    out, err, retCode = consoleIO.call(command, working_dir)
    return _handleError(err, retCode)


def genBinding(srcPrefix, xsdfile, rsltPath, template):
    schemaName = _extractName(xsdfile)
    rsltPath = '{0}{1}-xsdb'.format(rsltPath, schemaName)
    # move generated code to desired location
    if not os.path.exists(rsltPath):
        os.makedirs(rsltPath)
    # use tool to generate marshalling code
    cmd_fmt = '../xsdb {0} {1} > /tmp/c-xml-expat.tmp'
    rslt = _execu(cmd_fmt.format(template, xsdfile))
    rslt = rslt and _csplit(open('/tmp/c-xml-expat.tmp', 'r'),
                            "/\* FILE:\s(.*.[h|c])",
                            rsltPath)
    return rslt


def genBindingTest(xsdfile, rsltPath, template):
    schemaName = _extractName(xsdfile)
    rsltTstFile = '{0}{1}-bin.c'.format(rsltPath, schemaName);
    # use tool to generate test code
    cmd_fmt = '../xsdb {0} {1} > {2}'
    return _execu(cmd_fmt.format(template, xsdfile, rsltTstFile))


def genMakefile(xsdfiles, rsltPath):
    # open and create makefile
    makefile = open(rsltPath + 'makefile', "w")
    # output 'all' section
    makefile.write('all: ')
    for target in xsdfiles:
        target = _extractName(target)
        makefile.write(' ' + target)
    makefile.write('\n\n')
    # output each target section
    for target in xsdfiles:
        target = _extractName(target)
        makefile.write(target + ':\n')
        makefile.write('\t$(MAKE) -f test.mk TEST=' + target + '\n')
    makefile.write('\n')
    # output clean section
    makefile.write('clean:\n')
    for target in xsdfiles:
        target = _extractName(target)
        makefile.write('\t$(MAKE) -f test.mk TEST=' + target + ' clean\n')
    makefile.write('\n')
    # close file and return
    makefile.close()
    return True


def buildTests(rsltPath):
    _logMessage('compiling test cases: ')
    return _logCondition(_execu('make', rsltPath))


def execTests(xsdfiles, rsltPath):
    rslt = True;
    for exe in xsdfiles:
        exefile = _extractName(exe)
        _logMessage('executing test {0}: '.format(exefile))
        rslt = rslt and _execu(rsltPath + exefile)
        _logCondition(rslt)
    return rslt


def genPreconditionList(preconditionPath):
    return glob.glob(preconditionPath + "*.xsd")


def genTestCases(srcPrefix,
                 preconditionPath,
                 postConditionPath,
                 bindingTemplate,
                 testTemplate):
    msg_fmt = "generating test code from {0}: "
    preconditionFileLst = genPreconditionList(preconditionPath)
    for preconditionFile in preconditionFileLst:
        _logMessage(msg_fmt.format(preconditionFile))
        rslt = genBinding(srcPrefix,
                          preconditionFile,
                          postConditionPath,
                          bindingTemplate)
        rslt = rslt and genBindingTest(preconditionFile,
                                       postConditionPath,
                                       testTemplate)
        _logCondition(rslt)
    return rslt


def cleanTestDir(rsltPath):
    shutil.move(rsltPath + 'test.mk', '/tmp/test.mk')
    shutil.rmtree(rsltPath)
    os.makedirs(rsltPath)
    shutil.move('/tmp/test.mk', rsltPath + 'test.mk')


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
    rslt = genTestCases(srcPrefix,
                        testPath,
                        dstPath,
                        bindingTemplate,
                        testTemplate)
    rslt = rslt and genMakefile(xsdLst, dstPath)
    # make tests
    rslt = rslt and buildTests(dstPath)
    # run tests
    rslt = rslt and execTests(xsdLst, dstPath)
    return
