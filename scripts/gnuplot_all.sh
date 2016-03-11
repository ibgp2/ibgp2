#!/bin/sh
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Overview:
#   This script is used to plot all the curves related to a given
#   dataset. The duration of all the simulation must be the same
#   otherwise the results related to the convergence time will
#   be wrong.
#
# Usage:
#   ./gnuplot_all dataset bgpd_start_date simulation_time
#
# Examples:
#   ./gnuplot_all article 30 75

# Workspace
SCRIPT_DIR="`dirname $0`"
RESULTS_DIR="$SCRIPT_DIR/../results"

# Script paths
GNUPLOT1="$SCRIPT_DIR/gnuplot1_curves.py"
GNUPLOT2="$SCRIPT_DIR/gnuplot2_summaries.py"
GNUPLOT3="$SCRIPT_DIR/gnuplot3_benchmark.py"
GNUPLOT4="$SCRIPT_DIR/gnuplot4_convergence.py"
GNUPLOT5="$SCRIPT_DIR/gnuplot5_updates.sh"

usage() {
    echo "usage: $0 dataset bgpd_start_date simulation_duration
        dataset: a directory contained in $RESULTS_DIR containing simulation results (ex: article)
        bgpd_start_date: starting date of bgpd (ex: 30)
        simulation_duration: simulation duration (ex: 75)
    " >&2
}

check_arguments() {
    dataset=$1

    results_dataset_dir="$RESULTS_DIR/$dataset"
    if [ ! -d "$results_dataset_dir" ] ; then
        echo "Dataset [$dataset] not found in [$RESULTS_DIR]. Possible values are:" >&2
        for x in `find "$RESULTS_DIR" -maxdepth 1 -type d` ; do
            if [ "$x" != "$RESULTS_DIR" ] ; then
                echo "    `basename $x`" >&2
            fi
        done
        exit 1
    fi
}

run() {
    cmd=$1
    echo "Running $cmd"
    eval $cmd
}

gnuplot_dataset() {
    # Results paths
    dataset=$1
    results_dataset_dir="$RESULTS_DIR/$dataset"
    bgpd_start_date=$2
    simulation_duration=$3

    for directory in `find "$results_dataset_dir" -maxdepth 1 -type d` ; do
        mode=`basename "$directory"`
        if [ "$mode" = "rr" ] || [ "$mode" = "fm" ] || [ "$mode" = "ibgp2" ] ; then
            results_simu_dir="$results_dataset_dir/$mode"
            run "'$GNUPLOT1' '$results_simu_dir' '$bgpd_start_date' '$simulation_duration'"

            results_gnuplot_dir="$results_simu_dir/gnuplot"
            run "'$GNUPLOT2' '$results_gnuplot_dir'"
        fi
    done

    run "'$GNUPLOT3' '$results_dataset_dir'"
    run "'$GNUPLOT4' '$results_dataset_dir' '$bgpd_start_date'"
    run "'$GNUPLOT5' '$results_dataset_dir'"
}

#-----------------------------------------------
# Main program
#-----------------------------------------------

# Handle program arguments

if [ $# -ne 3 ] ; then
    usage
    exit 1
fi

dataset=$1
bgpd_start_date=$2
simulation_duration=$3

check_arguments $dataset
gnuplot_dataset $dataset $bgpd_start_date $simulation_duration

exit 0
