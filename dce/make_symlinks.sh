#!/bin/bash
#
# Usage:
#
#   ./symlink
#
# This build a symlink toward each files provided by iBGPv2 in
# the ns3 source root directory.
. ./env.sh

pushd $IBGP2_GIT/dce 1>/dev/null
for x in $(find source | egrep "\.(cc|h)$")
do
    pushd $NS3_DIR/dce 1>/dev/null
    if [ ! -f $x ]
    then
        echo "ln -s $x"
        ln -s $x
    fi
    popd 1>/dev/null
done
popd 1>/dev/null
echo "Symlinks built"
exit 0
