%define _topdir		%(echo $PWD)
%define name		xsd-tools
%define	release		0
%define version		0.1.0
%define buildroot	%{_topdir}/%{name}-%{version}-root

BuildRoot:		%{buildroot}
Summary:		xsd-tools is a tool to generate bindings in multiple languages from xsd schema documents
License:		GNU GPL v3
Name:			%{name}
Version:		%{version}
Release:		%{release}
Source:			%{name}.tgz
Prefix:			/usr
Group:			Development/Tools

BuildRequires:		gcc-c++ >= 4.4.7
BuildRequires:		cmake >= 3.16
BuildRequires:		tinyxml-devel >= 2.6.1
BuildRequires:		boost-devel >= 1.41.0
BuildRequires:		lua-devel >= 5.1.4
Requires:		tinyxml >= 2.6.1
Requires:		boost-filesystem >= 1.41.0
Requires:		boost-system >= 1.41.0
Requires:		lua >= 5.1.4

%description
A tool to generate data bindings from XML XSD schema for multiple languages including C, Python & Java.

%prep
%setup -q -n xsd-tools

%build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
      -DBUILD_TESTING=OFF
cmake --build build -j2

%install
DESTDIR=$RPM_BUILD_ROOT cmake --install build

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/xsdb
/usr/share/xsdtools/templates/


%changelog
