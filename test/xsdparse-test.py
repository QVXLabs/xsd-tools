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
import conio

consoleIO = conio.conio()

def runTest(xsdfile, rsltFile):
	consoleIO.stdout(consoleIO.ENDC, "Running test:" + xsdfile + ": ")
	# run tool and capture output
	cmd = "../xsdb ../templates/test " + xsdfile
	std, err, errCode = consoleIO.call(cmd)
	test_result = std.decode('utf-8')
	if 0 != len(err):
		consoleIO.stdout(consoleIO.FAIL, "Fail\n")
		consoleIO.stdout(consoleIO.FAIL, err)
		return
	try:
		postcondition = open(rsltFile).read()
		if postcondition == test_result:
			consoleIO.stdout(consoleIO.OKGREEN, "Pass\n")
		else:
			consoleIO.stdout( consoleIO.FAIL, "Fail\n" )
			for diffs in difflib.unified_diff(test_result, postcondition, "xsdb:output", rsltFile ):
				consoleIO.stdout( consoleIO.FAIL, diffs )
	except IOError as e:
		consoleIO.stdout(consoleIO.FAIL, "Fail\n")
		if e.errno == os.errno.ENOENT:
			consoleIO.stdout(consoleIO.FAIL, "\ttest postcondition file not found\n")
		else:
			consoleIO.stdout(consoleIO.FAIL, "\texception: " + e)
	return

def testLoop(preconditionPath, postConditionPath):
	preconditionFileLst = glob.glob(preconditionPath + "*.xsd")
	for preconditionFile in preconditionFileLst:
		postconditionFile = postConditionPath + \
			(os.path.split(preconditionFile[:-3] + 'out')[1])
		runTest(preconditionFile, postconditionFile)

def main():
	consoleIO.stdout(consoleIO.ENDC, "relative path arguments\n")
	testLoop("xsd-positive/", "xsdparse/")
	consoleIO.stdout(consoleIO.ENDC, "full path arguments\n")
	testLoop(os.getcwd() + '/xsd-positive/', "xsdparse/")

if __name__ == "__main__":
	main()
