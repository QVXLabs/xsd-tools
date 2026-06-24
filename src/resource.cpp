/*
 * resource.cpp
 *
 *  Created on: 01/29/12
 *      Author: QVXLabs LLC
 *   Copyright: (c)2012 QVXLabs LLC
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
#include <string>
#include <cstdlib>
#if defined(_WIN32)
#	include <io.h>
#	include <windows.h>
#elif defined(__APPLE__)
#	include <unistd.h>
#	include <sys/types.h>
#	include <pwd.h>
#	include <mach-o/dyld.h>
#else
#	include <unistd.h>
#	include <sys/types.h>
#	include <pwd.h>
#endif
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

/* True if `path` is readable. (POSIX access / Windows _access.) */
static bool
_readable(const string& path) noexcept {
#if defined(_WIN32)
	return 0 == _access(path.c_str(), 4 /* R_OK */);
#else
	return 0 == access(path.c_str(), R_OK);
#endif
}

/* The current user's home directory, or "" if unknown. */
static string
_homeDir() noexcept {
#if defined(_WIN32)
	const char* home = getenv("USERPROFILE");
	return home ? string(home) : string();
#else
	struct passwd* pw = getpwuid(getuid());
	return pw ? string(pw->pw_dir) : string();
#endif
}

/* The directory containing the running executable, or "" if it cannot be
 * resolved. Lets a relocatable install (tarball) find its templates without
 * env setup, independent of the prefix baked in at build time. */
static string
_exeDir() noexcept {
	char buf[4096];
#if defined(_WIN32)
	DWORD n = GetModuleFileNameA(NULL, buf, sizeof(buf));
	if (0 == n || n >= sizeof(buf))
		return string();
	string path(buf, n);
	size_t slash = path.find_last_of("\\/");
#elif defined(__APPLE__)
	uint32_t size = sizeof(buf);
	if (0 != _NSGetExecutablePath(buf, &size))
		return string();
	string path(buf);
	size_t slash = path.find_last_of('/');
#else
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n <= 0)
		return string();
	buf[n] = '\0';
	string path(buf);
	size_t slash = path.find_last_of('/');
#endif
	return (string::npos == slash) ? string() : path.substr(0, slash);
}

string
Resource::GetTemplatePath(const std::string& templateName) noexcept(false) {
	/* test if the template name exists */
	if (_readable(templateName))
		return templateName;
	/* check environment variable for template file */
	const char * val = getenv("XSDTOOLS_DATA");
	if (nullptr != val) {
		string envFilePath(val);
		envFilePath += "/" + templateName;
		if (_readable(envFilePath))
			return envFilePath;
	}
	/* check the home directory for template file */
	string homeDir(_homeDir());
	if (!homeDir.empty()) {
		string homeFilePath(homeDir);
		homeFilePath += "/" + gscHOMEPATH.substr(1) + templateName;
		if (_readable(homeFilePath))
			return homeFilePath;
	}
	/* check relative to the executable (relocatable install / tarball) */
	string exeDir(_exeDir());
	if (!exeDir.empty()) {
		const string relPaths[] = {
			exeDir + "/../share/xsdtools/templates/" + templateName,
			exeDir + "/templates/" + templateName,
		};
		for (const string& relPath : relPaths) {
			if (_readable(relPath))
				return relPath;
		}
	}
	/* check the global directory for template file */
	string globalFilePath(gscGLOBALPATH);
	globalFilePath += "/" + templateName;
	if (_readable(globalFilePath))
		return globalFilePath;
	else {
		string errorMessage("Could not open template ");
		throw ResourceException(errorMessage + templateName);
	}
	return std::string("");
}
