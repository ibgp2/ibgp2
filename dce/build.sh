#!/bin/bash
#
# Build ns-$NS3_VERSION and ns-3-dce
#
# Author:
#    Marc-Olivier Buob <marcolivier.buob@orange.fr>

. env.sh

# We altered the following wscript:
#
# ./ns-$NS3_VERSION/src/applications/wscript
# ./ns-$NS3_VERSION/src/internet/wscript
# ./ns-3-dce/myscripts/ns-3-dce-quagga/wscript
#
# The corresponding waf scripts are:
#
# ./ns-$NS3_VERSION/waf
# ./ns-3-dce/waf
#
# Since ns-3-dce depends on ns-$NS3_VERSION, we must recompile ns-$NS3_VERSION before ns-$NS3_VERSION.
# - To be sure that ns-$NS3_VERSION libs are properly recompiled and always up-to-date
# we enforce binaries reinstall.
# - To avoid link error we also reinstall ns-3-dce and rebuild final binaries.

# If linking error occurs due to quagga-helper, run:
# pushd $NS3_DIR/dce/source/ns-3-dce/myscripts/ns-3-dce-quagga ; ./waf uninstall build install ; popd
# pushd $NS3_DIR/dce/source/ns-3-dce/ ; ./waf ; popd

#for dir in ns-$NS3_VERSION ns-3-dce
#do
#    pushd $NS3_DIR/dce/source/$dir > /dev/null
#    # For ns-3-dce : if you get an error due to libtest.so, skip install target
#    ./waf clean build install
#    if [ $? -ne 0 ]
#    then
#        exit 1
#    fi
#    popd > /dev/null
#done

pushd $NS3_DIR/dce/source/ns-$NS3_VERSION > /dev/null ; ./waf ; popd > /dev/null
pushd $NS3_DIR/dce/source/ns-3-dce > /dev/null ; ./waf ; popd > /dev/null

exit 0
