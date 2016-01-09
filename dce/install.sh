#!/bin/bash
# Install ns3 and iBGP2
#
# Usage:
#   ./install.sh
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
set -e

. ./env.sh

#GIT_URL_IBGP2="npafi@git.npafi.org:ibgp2.git"
GIT_URL_IBGP2="https://github.com/ibgp2/ibgp2.git"

print_sep() {
    echo "------------------------------------------------------------------------------------"
}

install_dependencies() {
    print_sep
    echo "Installing dependencies"
    print_sep
    sudo apt-get update

    # Those dependencies are required to compile ns3 (g++ indent libsysfs-dev libssl-dev),
    # and to compile our stuff (libboost-dev libpstreams-dev)
    sudo apt-get -y install g++ gawk indent mercurial libsysfs-dev libssl-dev libboost-dev pkg-config tcpdump libpstreams-dev
}

install_bake() {
    print_sep
    echo "Downloading bake"
    print_sep
    mkdir -p "$NS3_DIR"
    cd $NS3_DIR
    hg clone http://code.nsnam.org/bake bake
    cd -
}

configure_bake() {
    print_sep
    echo "Configuring bake"
    print_sep
    mkdir -p "$NS3_DIR/dce"
    cd "$NS3_DIR/dce"

    # This generates a first configuration template in $NS3_DIR/dce/bakefile.xml
    bake.py configure -e dce-ns3-$NS3_DCE_VERSION -e dce-quagga-$NS3_DCE_VERSION
}

download_compile_ns3() {
    print_sep
    echo "Compiling ns3 in $NS3_DIR/dce"
    print_sep

    cd "$NS3_DIR/dce"
    bake.py download
    bake.py build
    cd -
}

download_ibgp2() {
    cd "$NS3_DIR"

    # Our git'ed files will be taken from the iBGP2 repository
    if [ ! -f .git/config ]  || [ "$(grep -c "$GIT_URL_IBGP2" .git/config)" -eq 0 ] ; then

        print_sep
        echo "Downloading iBGP2"
        print_sep

        git init
        git remote add origin "$GIT_URL_IBGP2"

        # We only merge the contents of "dce" (on the git repository) into $NS3_DIR/dce
        # (the directory containing currently the ns3 sources).
        # Based on: http://jasonkarns.com/blog/subdirectory-checkouts-with-git-sparse-checkout/
        git config core.sparsecheckout true
        echo "dce" >> .git/info/sparse-checkout
        git fetch origin
        git checkout -tf origin/master

        # The following will hide all the ns3 files when running git status, but do not
        # forget to git add the new files you might create for ibgp2 as they won't appear!
        git config status.showuntrackedfiles no
    fi
    cd -
}

compile_ns3_ibgp2() {
    ns3_dir="$NS3_DIR/dce/source/ns-$NS3_VERSION"
    ns3_dce_dir="$NS3_DIR/dce/source/ns-3-dce"

    # Enable C++11 flags
    # Do not use --disable-examples, otherwise the iBGPv2 simulator won't be compiled.
    cd $ns3_dce_dir
    CXXFLAGS="-std=c++0x" ./waf configure --disable-tests --with-ns3="$NS3_DIR"/dce/build
    ./waf clean build
    cd -
}

usage() {
    args="--igp=$IBGP2_GIT/datasets/test/igp.csv --ebgp=$IBGP2_GIT/datasets/test/prefix.csv --ibpgMode=2 --verbose --routeInterval=5 --stopTime=75"

    print_sep
    echo "
You can now run a simulation:

    cd $NS3_DCE_DIR
    ./waf --run="dce-ibgpv2-simu $args"

To (re)compile the project:

    ns-$NS3_VERSION module:

        cd $NS3_DIR/dce/source/ns-$NS3_VERSION
        ./waf

    ns-3-dce module:

        cd $NS3_DCE_DIR
        ./waf

    ... or simpler:

        cd $NS3_DIR/dce/
        ./build.sh

    Note: to customize your compilation profile, see $NS3_DIR/dce/bakefile.xml

To debug the project:

    cd $NS3_DCE_DIR
    ./waf --run \"dce-ibgpv2-simu\" --command-template=\"gdb %s --args %s $args\"
"

}

install_dependencies
install_bake
configure_bake
download_compile_ns3
download_ibgp2
compile_ns3_ibgp2
usage
exit 0
