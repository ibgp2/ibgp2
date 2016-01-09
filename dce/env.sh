#!/bin/bash
#
# This script set the environment variables in order to deploy and compile iBGP2.
# It is used by the installation script (install.sh) and can be useful for the developer.
#
# Usage:
#
#    ./env.sh

echo "Loading environment variables"
export NS3_DIR="$HOME/git/ns3"
export NS3_VERSION="3.23"
export NS3_DCE_DIR="$NS3_DIR/dce/source/ns-3-dce/"
export NS3_DCE_VERSION="1.6"
export IBGP2_GIT="$HOME/git/ibgp2"
export BAKE_HOME="$NS3_DIR/bake"
export PATH="$PATH:$BAKE_HOME"
export PYTHONPATH="$PYTHONPATH:$BAKE_HOME"
