#!/bin/bash

SCRIPT=${BASH_SOURCE[0]}
SCRIPTPATH=$(realpath $(dirname $SCRIPT))
PROJECT_DIR=code
REPORT_FILE=rapport.pdf
ARCHIVE=rendu.tar.gz

cd $SCRIPTPATH

if [ ! -d "${PROJECT_DIR}" ]
then
    echo "Could not find project code directory : $(realpath $PROJECT_DIR)" >&2
    exit 1
fi

if [ ! -f "$REPORT_FILE" ]
then
    echo "Could not find $REPORT_FILE directory in $SCRIPTPATH" >&2
    exit 1
fi

echo "The following files are archived in $ARCHIVE : "
tar --exclude="$ARCHIVE" --exclude='*.o' -czvf $ARCHIVE ${PROJECT_DIR} ${REPORT_FILE}
