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

#if defined(_WIN32)
#	include <io.h>
#else
#	include <unistd.h>
#endif
#include <exception>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include "./src/xsdtools.hpp"
#include "./src/luaScriptAdapter.hpp"
#include "./src/resource.hpp"
#include "./src/XSDParser/Parser.hpp"
#include "version.h"

using namespace std;

namespace {
	void printUsage(ostream& out) {
		out << "xsdb " << XSDTOOLS_VERSION
		    << " -- XSD-to-code generator (c) QVXLabs LLC\n\n"
		    << "Usage: xsdb [options] <template> <input.xsd> [k v ...]\n\n"
		    << "Generate code from <input.xsd> using <template> and print it\n"
		    << "to stdout. Trailing args are template key/value pairs exposed\n"
		    << "to the template as __CMD_ARGS__.\n\n"
		    << "Options:\n"
		    << "  --out-dir <dir>  Split multi-file output on '/* FILE: name'\n"
		    << "                   markers and write real files under <dir>.\n"
		    << "  --list           List available templates, one per line.\n"
		    << "  -h, --help       Show this help and exit.\n"
		    << "  --version        Print version and exit.\n\n"
		    << "Example:\n"
		    << "  xsdb c-xml-expat test/xsd-positive/testA001.xsd\n";
	}
}

int main(int argc, const char* argv[]) {
	std::string outDir;
	std::vector<std::string> positionals;
	for (int ndx = 1; ndx < argc; ++ndx) {
		std::string arg(argv[ndx]);
		if ("-h" == arg || "--help" == arg) {
			printUsage(cout);
			return 0;
		} else if ("--version" == arg) {
			cout << "xsdb " << XSDTOOLS_VERSION << endl;
			return 0;
		} else if ("--list" == arg) {
			Core::Resource resource;
			std::vector<std::string> names = resource.ListTemplates();
			if (names.empty()) {
				cerr << "No templates directory found (set XSDTOOLS_DATA, "
				     << "install templates, or run from a source tree)."
				     << endl;
				return 1;
			}
			for (size_t i = 0; i < names.size(); ++i)
				cout << names[i] << endl;
			return 0;
		} else if ("--out-dir" == arg) {
			if (ndx + 1 >= argc) {
				cerr << "--out-dir requires a directory argument." << endl;
				return 2;
			}
			outDir = argv[++ndx];
		} else {
			positionals.push_back(arg);
		}
	}

	if (positionals.size() < 2) {
		printUsage(cerr);
		return 2;
	}
	const std::string& templatePath = positionals[0];
	const std::string& xsdPath = positionals[1];

	/* Distinguish a missing/unreadable XSD up front: Generate() would
	 * otherwise surface this as a generic parse error. */
#if defined(_WIN32)
	if (0 != _access(xsdPath.c_str(), 4 /* R_OK */)) {
#else
	if (0 != access(xsdPath.c_str(), R_OK)) {
#endif
		cerr << "XSD file not found or unreadable: " << xsdPath << endl;
		return 3;
	}

	/* template key/value pairs follow the two positionals */
	std::vector<std::string> templateArgs(positionals.begin() + 2,
	                                       positionals.end());
	try {
		if (outDir.empty()) {
			XsdTools::Generate(templatePath, xsdPath, std::cout,
			                   templateArgs);
		} else {
			std::ostringstream blob;
			XsdTools::Generate(templatePath, xsdPath, blob, templateArgs);
			std::vector<std::string> files =
				XsdTools::SplitMarkedFiles(blob.str(), outDir);
			for (size_t i = 0; i < files.size(); ++i)
				cout << outDir << "/" << files[i] << endl;
		}
	} catch (XSD::XMLException& e) {
		cerr << "XSD parse error in " << xsdPath << ": " << e.what() << endl;
		return 4;
	} catch (Core::LuaException& e) {
		cerr << "Template error in " << templatePath << ": " << e.what()
		     << endl;
		return 5;
	} catch (Core::ResourceException& e) {
		/* template-not-found / output-write failures */
		cerr << e.what() << endl;
		return 6;
	} catch (std::exception& e) {
		cerr << "General error: " << e.what() << endl;
		return 1;
	}
	return 0;
}
