#!/bin/bash
# parse options
outputdir="./target/generated-sources/xsdb"
package="com.unknown.generated"
while getopts x:d:p: var; do
    case $var in
	x) xsd=$OPTARG;;
	d) outputdir=$OPTARG;;
	p) package=$OPTARG;;
    esac
done

# handle option
if [ -z $xsd ]; then
    echo "xsd-java-wrapper (c)2015 QVXLabs LLC"
    echo "Parsers output from xsdb and seperates into files."
    echo "Syntax: xsdb--wrapper -x <xsd-file> -d <output directory> -p <java-package>"
    echo "        -d option is optional and defaults to"
    echo "           ./target/generated-sources/xsdb"
    echo "        -p option is optional and defaults to"
    echo "           com.unknown.generated"
else
    XSDB_OUT=$(mktemp -t "xsdb.XXXX")
    xsdb java-json.org $xsd -package $package > $XSDB_OUT
    awk -f java-split-file.awk -v outputdir=$outputdir $XSDB_OUT | sh
fi
