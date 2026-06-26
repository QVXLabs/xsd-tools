#!/bin/bash
# parse options
while getopts x:d: var; do
    case $var in
	x) xsd=$OPTARG;;
	d) outputdir=$OPTARG;;
    esac
done

# handle option
if [ -z $xsd ]; then
    echo "xsd-c-wrapper (c)2015 QVXLabs LLC"
    echo "Parsers output from xsdb and seperates into files."
    echo "Syntax: xsdb-c-wrapper -x <xsd-file> -d <output directory>"
    echo "        -d option is optional and defaults to ./"
else
    XSDB_OUT=$(mktemp -t "xsdb.XXXX")
    xsdb c-xml-expat-dom $xsd > $XSDB_OUT
    awk -f c-split-file.awk -v outputdir=$outputdir $XSDB_OUT | sh
    rm $XSDB_OUT
fi
