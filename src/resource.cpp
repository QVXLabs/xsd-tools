/*
 * resource.cpp
 *
 *  Created on: 01/29/12
 *      Author: Ardavon Falls
 *   Copyright: (c)2012 Ardavon Falls
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
 *  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string>
#define INCBIN_SILENCE_BITCODE_WARNING
#define INCBIN_PREFIX _binary_
#include <incbin.h>
#include "src/resource.hpp"
#include "resource_config.hpp"

using namespace std;
using namespace Core;

INCBIN(luascript_luac, "luascript.luac");

static const unsigned char * _binary_luascript_luac = &_binary_luascript_luacData[0];
static const size_t _binary_luascript_luac_size = _binary_luascript_luacSize;

static const string gscHOMEPATH("~/.xsdtools/templates");
static const string gscGLOBALPATH(XSDTOOLS_GLOBAL_TEMPLATE_PATH);

ResourceException::ResourceException(const string& rMsg)
	: m_errorMsg(rMsg)
{ }

/* virtual */
ResourceException::~ResourceException() noexcept
{ }

/* virtual */ const char*
ResourceException::what() const noexcept {
	return m_errorMsg.c_str();
}

Resource::Resource() 
{ }

/* virtual */
Resource::~Resource() noexcept
{ }

const uint8_t *
Resource::GetEngineScript(size_t* pRetSz) noexcept {
	*pRetSz = _binary_luascript_luac_size;
	return reinterpret_cast<const uint8_t*> (_binary_luascript_luac);
}

string
Resource::GetTemplatePath(const std::string& templateName) noexcept(false) {
	/* test if the template name exists */
	if (0 == access(templateName.c_str(), R_OK))
		return templateName;
	/* check environment variable for template file */
	const char * val = getenv("XSDTOOLS_DATA");
	if (nullptr != val) {
		string envFilePath(val);
		envFilePath += "/" + templateName;
		if (0 == access(envFilePath.c_str(), R_OK))
			return envFilePath;
	}
	/* check the home directory for template file */
	struct passwd* pw = getpwuid(getuid());
	string homeFilePath(pw->pw_dir);
	homeFilePath += "/" + gscHOMEPATH.substr(1) + templateName;
	if (0 == access(homeFilePath.c_str(), R_OK))
		return homeFilePath;
	/* check the global directory for template file */
	string globalFilePath(gscGLOBALPATH);
	globalFilePath += "/" + templateName;
	if (0 == access(globalFilePath.c_str(), R_OK))
		return globalFilePath;
	else {
		string errorMessage("Could not open template ");
		throw ResourceException(errorMessage + templateName);
	}
	return std::string("");
}
