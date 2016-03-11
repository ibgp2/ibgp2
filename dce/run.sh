#!/bin/bash
#
# Running script for iBGP simulations.
#
# Note: you don't need to run this script as root. Be sure to choose a large
# enough stopTime value.
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>

#-----------------------------------------------------------------------------
# Simulation parameters
#-----------------------------------------------------------------------------

# The period (in seconds) used to dump the FIB of each routers.
# Set this value to 0 to disable this dump. Value suggested: 5s since this
# is the default of MRAI in iBGP.

routesInterval=5

# The start date of bgpd.
# You should choose a value enough large to guarantee that the IGP has
# converges before starting bgpd daaemons. Otherwise some artifacts may occurs.
# For instance a BGP router may select temporarily a suboptimal egress point
# thinking this is the optimal one in terms of IGP cost. The router will detects
# its mistake only after having perform a rescan of its FIB (every 60s). As a
# sequel, some BGPd routers may converge significantly slower only.

bgpdStartTime=30

# The date (in seconds) to stop the simulation.
# If you observe setsockopt error in the BGPd's and OSPFd's log, this is
# probably because stopTime is too small.

stopTime=75

# The input dataset: test|article

dataset="article"
#dataset="test"

# The iBGP topology type: valid values are: ibgp2|rr|fm|all

mode="all"
#mode="ibgp2"
#mode="fm"
#mode="rr"

# Set which debug message must be printed.
# Example: "--verbose --debug"

debug="--verbose"

# Type of sampling used to generate input prefixes
# Valid values: "all" "random". But something else to debug.

sampling="all"
#sampling="random"
#sampling="debug"

#-----------------------------------------------------------------------------
# Script
#-----------------------------------------------------------------------------

. ./env.sh

OUTPUT_DIR="$IBGP2_GIT/results"
dataset_dir="$IBGP2_GIT/datasets/$dataset"
input_igp="$dataset_dir/igp.csv"
input_ibgp="$dataset_dir/ibgp.csv"

if [ "$sampling" == "all" ] || [ "$sampling" == "random" ] ; then
    inputs_ebgp="$dataset_dir/prefixes_$sampling/prefix*.csv"
else
    # debug: use a specific dataset of eBGP prefixes
    inputs_ebgp="$dataset_dir/prefixes_all/prefix2.csv"
fi

date="$(date +%Y-%m-%d_%H:%M:%S)"

clean_previous_run() {
	# Remove outputs of previous run
	echo "Cleaning previous filesystems ($NS3_DCE_DIR/files-*)"

    rm  -f "$NS3_DCE_DIR"/*_ibgpv2_bgp.txt
	rm -rf "$NS3_DCE_DIR"/files-*
	rm  -f "$NS3_DCE_DIR"/routes*.log
	rm  -f "$NS3_DCE_DIR"/*.csv
}

print_sep() {
    echo "======================================================"
}

print_bgp_updates() {
    print_sep
    echo "BGP updates:"
    echo "$result_dir | grep 'bgpd.log$' | xargs grep ' UPDATE '"
    print_sep

    find $result_dir | grep 'bgpd.log$' | xargs grep ' UPDATE '
}

print_bgp_routes_external_prefixes() {
    result_dir=$1
    input_ebgp=$2

	echo grep -v "^#" "$input_ebgp"

	grep -v "^#" "$input_ebgp" | while read line
    do
        # Get the eBGP prefix and the ASBR hostname.
        asbr=$(echo $line | awk '{print $1}')
		ebgp_prefix=$(echo $line | awk '{print $2}')

		# Extract the IP of the prefix,
		ip="$(echo "$ebgp_prefix" | awk '{print $1}' | cut -d"/" -f1 | uniq)"

		print_sep
        echo "Routes toward eBGP prefix [$ebgp_prefix] entering the AS via [$asbr]"
        echo "grep "$ip" "$result_dir"/routes*.log | uniq"
        print_sep

        pushd "$result_dir" > /dev/null
        grep "$ip" routes*.log | uniq
        popd > /dev/null
    done
}

run_ns3() {
    input_igp="$1"
    input_ebgp="$2"
    mode="$3"
    debug="$4"
    routesInterval="$5"
    bgpdStartTime="$6"
    stopTime="$7"

	# iBGP mode

	case "$mode" in
		fm)
			mode_option="--ibgpMode=0"
			;;
		rr)
			mode_option="--ibgpMode=1 --ibgp=$input_ibgp"
			;;
		ibgp2)
			mode_option="--ibgpMode=2"
			;;
	esac

	# Run NS
	# You can add export NS_LOG="MyObject" before running ./waf to turn on NS_LOG
    #export NS_LOG="Ibgp2d"

	waf_cmd="dce-ibgpv2-simu --igp=$input_igp --ebgp=$input_ebgp $mode_option $debug --routesInterval=$routesInterval --bgpdStartTime=$bgpdStartTime --stopTime=$stopTime"
	#waf_cmd="dce-rr-simu --igp=$input_igp --ebgp=$input_ebgp $mode_option $debug --routesInterval=$routesInterval --stopTime=$stopTime"
	echo "$date Running ./waf --run=\"$waf_cmd\" in [$NS3_DCE_DIR]"
	pushd "$NS3_DCE_DIR" 1>/dev/null

	# Note: it seems useless to be root since ns3-dce will never build raw socket
	./waf --run="$waf_cmd"
	ret=$?
	popd 1>/dev/null

	if [ $ret != 0 ]
	then
		exit $ret
	fi

    return $ret
}

save_results() {
    result_dir=$1

	# Save the results
	echo "Saving results in $result_dir"
	rm -rf "$result_dir"
	mkdir -p "$result_dir"
	mv "$NS3_DCE_DIR"/*.csv "$result_dir"
	mv "$NS3_DCE_DIR"/routes*.log "$result_dir"

	# Move node's filesystems in $result_dir
	old_IFS=$IFS
	IFS=$'\n'
	for x in $(awk '/^[^#]/{print $1, $2}' "$result_dir"/ifconfig.csv | uniq)
	do
		# Rename them according to their hostname
		node_id=$(echo $x | cut -d" " -f1)
		node_name=$(echo $x | cut -d" " -f2)
		source_dir="$NS3_DCE_DIR"/files-"$node_id"
		target_dir="$result_dir"/files-"$node_name"
		mv "$source_dir" "$target_dir"

		# Remove useless files
		find "$target_dir" | grep "$target_dir/var/log/[0-9]" | xargs rm -rf
		find "$target_dir" | grep "$target_dir/home"          | xargs rm -rf
		find "$target_dir" | grep "\.pid$"                    | xargs rm -rf
	done
	IFS=$old_IFS
	mv "$NS3_DCE_DIR"/bgpd_*.txt "$result_dir"
	mv "$NS3_DCE_DIR"/zebra_*.txt "$result_dir"
	if [ "$mode" == "ibgp2" ]
    then
        mv "$NS3_DCE_DIR"/*_ibgpv2_bgp.txt "$result_dir"
    fi
}

print_bgp_updates() {
    print_sep
    echo "BGP updates:"
    echo "$result_dir | grep 'bgpd.log$' | xargs grep ' UPDATE '"
    print_sep

    find $result_dir | grep 'bgpd.log$' | xargs grep ' UPDATE '
}

print_bgp_routes_external_prefixes() {
    result_dir=$1
    input_ebgp=$2

    echo "--"
    echo $input_ebgp
    echo "--"

    grep -v "^#" "$input_ebgp" | while read line
    do
        asbr=$(echo $line | awk '{print $1}')
        ebgp_prefix=$(echo $line | awk '{print $2}')
        ip="$(echo "$ebgp_prefix" | awk '{print $1}' | cut -d"/" -f1 | uniq)"

        if [ -z "$asbr" ] || [ -z "$ebgp_prefix" ] || [ -z "$ip" ] ; then
            continue
        fi

        print_sep
        echo "Routes toward eBGP prefix: [$ebgp_prefix] via egress point [$asbr]"

        pushd "$result_dir" > /dev/null
        grep "$ip" routes*.log | uniq
        popd > /dev/null
     done
}

if [ $mode == "all" ] ; then
    modes="rr fm ibgp2"
else
    modes=$mode
fi

for mode in $modes ; do
    mode_dir="$OUTPUT_DIR/$dataset/$mode"
    echo $mode_dir
    # For each set of input prefixes
    for input_ebgp in $(ls -1 $inputs_ebgp) ; do
        print_sep
        echo "input_igp      = $input_igp"
        echo "input_ebgp     = $input_ebgp"
        echo "mode           = $mode"
        echo "debug          = $debug"
        echo "routesInterval = $routesInterval"
        echo "bgpdStartTime  = $bgpdStartTime"
        echo "stopTime       = $stopTime"
        print_sep
        result_dir="$mode_dir/$(basename $input_ebgp)/" #$date"

        # Prepare workspace to avoid side-effect
        clean_previous_run

        # Call ns3
        run_ns3 "$input_igp" "$input_ebgp" "$mode" "$debug" "$routesInterval" "$bgpdStartTime" "$stopTime"
        ret=$?
        if [ "$ret" -ne 0 ] ; then
            echo "run_ns3 returned $ret" >&2
            exit $ret
        fi

        # Backup results
        save_results "$result_dir"

        # Uncomment the following instructions to debug
        #print_bgp_updates "$result_dir"
        #print_bgp_routes_external_prefixes "$result_dir" "$input_ebgp"
        #break
    done
done

exit 0
