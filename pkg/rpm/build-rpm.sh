#!/bin/sh
tar -czvf SOURCES/xsd-tools.tgz -C ../../.. \
    xsd-tools/CMakeLists.txt xsd-tools/templates xsd-tools/src \
    xsd-tools/third-party
rpmbuild -v -bb --clean SPECS/xsd-tools.spec