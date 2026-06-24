/*
 * main.cpp
 *
 *  Created on: May 18, 2011
 *      Author: QVXLabs LLC
 *   Copyright: (c)2011 QVXLabs LLC
 *
 *  This file is part of xsd-tools.
 *
 *  xsd-tools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  xsd-tools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with xsd-tools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <exception>
#include <string>
#include <iostream>
#include <vector>
#include "./src/xsdtools.hpp"
#include "./src/luaScriptAdapter.hpp"
#include "./src/resource.hpp"
#include "./src/XSDParser/Parser.hpp"
#include "resource_config.hpp"

using namespace std;

int main(int argc, const char* argv[]) {
	if (argc >= 2) {
		const string firstArg(argv[1]);
		if ("--version" == firstArg || "-v" == firstArg) {
			cout << "xsdb " << XSDTOOLS_VERSION << endl;
			return 0;
		}
	}
	if (argc < 3) {
		cout << "xsdb " << XSDTOOLS_VERSION << " (c) QVXLabs LLC" << endl;
		cout << "Syntax xsdb <template> <input xsd file> <unique template paramters>" << endl;
		cout << "To retrieve list of template paramters invoke with option \"-h info\"" << endl;
		cout << "For example:" << endl;
		cout << "\t #./xsdb templates/test test/xsd-positive/testA001.xsd -h all" << endl;
		cout << "Please note that not all templates have optional paramters in which" << endl;
		cout << "case they will not return anything as optional paramters." << endl;
		return 0;
	}
	/* additional argv beyond <template> <xsd> are template key/value pairs */
	std::vector<std::string> templateArgs;
	for (int ndx = 3; ndx < argc; ++ndx)
		templateArgs.push_back(argv[ndx]);
	try {
		XsdTools::Generate(argv[1], argv[2], std::cout, templateArgs);
	} catch (XSD::XMLException& e) {
		cerr << "XSD Parsing Error: " << e.what() << endl;
		return 1;
	} catch (Core::LuaException& e) {
		cerr << "Lua Error: " << e.what() << endl;
		return 1;
	} catch (Core::ResourceException& e) {
		cerr << e.what() << endl;
		return 1;
	} catch (std::exception& e) {
		cerr << "General error: " << e.what() << endl;
		return 1;
	}
	return 0;
}
