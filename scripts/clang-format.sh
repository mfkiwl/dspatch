#!/bin/bash

DIR=$(cd `dirname $0` && pwd)
cd $DIR/..

find . -iname *.h -o -iname *.cpp | grep -v subprojects | xargs clang-format --style=file --verbose -i
