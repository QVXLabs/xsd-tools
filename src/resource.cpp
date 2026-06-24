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
#include <dirent.h>
#include <algorithm>
#include <string>
#include <vector>
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
	/* source-tree convenience: ./templates/<name> */
	string localPath("templates/" + templateName);
	if (0 == access(localPath.c_str(), R_OK))
		return localPath;
	throw ResourceException("Could not open template " + templateName);
	return std::string("");
}

string
Resource::TemplatesDir() noexcept {
	const char* val = getenv("XSDTOOLS_DATA");
	if (nullptr != val && 0 == access(val, R_OK))
		return string(val);
	struct passwd* pw = getpwuid(getuid());
	if (nullptr != pw) {
		string homeDir(pw->pw_dir);
		homeDir += "/" + gscHOMEPATH.substr(1);
		if (0 == access(homeDir.c_str(), R_OK))
			return homeDir;
	}
	if (0 == access(gscGLOBALPATH.c_str(), R_OK))
		return gscGLOBALPATH;
	/* source-tree convenience: ./templates */
	if (0 == access("templates", R_OK))
		return string("templates");
	return string("");
}

vector<string>
Resource::ListTemplates() noexcept {
	vector<string> names;
	string dir = TemplatesDir();
	if (dir.empty())
		return names;
	DIR* pDir = opendir(dir.c_str());
	if (nullptr == pDir)
		return names;
	struct dirent* pEnt = nullptr;
	while (nullptr != (pEnt = readdir(pDir))) {
		string name(pEnt->d_name);
		if ("." == name || ".." == name)
			continue;
		if (0 == access((dir + "/" + name).c_str(), R_OK))
			names.push_back(name);
	}
	closedir(pDir);
	sort(names.begin(), names.end());
	return names;
}
