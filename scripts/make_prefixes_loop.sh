#!/bin/sh
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Overview:
#   This script is used to produces a set of prefix*.csv
#   files used in input of a simulation.
#
# Usage:
#   ./make_prefixes_loop {all|random} asbr_filename [num_prefixes]
#
# Examples:
#   ./make_prefixes_loop all ../datasets/article/asbr.csv
#   ./make_prefixes_loop random ../datasets/article/asbr.csv 5

SCRIPT_DIR="`dirname $0`"

usage() {
    echo "usage: $0 {all|random} asbr_filename [num_prefixes]
    Args:
        {all|random}:
            all: All the set of concurrent prefixes are generated.
            random: A sample of num_prefix concurrent prefixes is generated for each value of {1..\$num_asbr}.
        asbr_filename:
            Path to a file containing one ASBR name per line.
            Example: ~/git/ibgp2/datasets/article/asbr.csv
        num_prefixes:
            Only relevant if \$1 == "random".
            An integer > 0, indicating for each value of {1..\$num_asbr}, how many concurrent prefix sets are randomly picked.
            Suggested value: 5.
    Outputs:
        prefix*.csv files
    " >&2 
}

# Check number of arguments

if [ $# -ne 2 ] && [ $# -ne 3 ] ; then
    usage
    exit 1
fi

# Map each variable with the corresponding argument

sampling=$1
asbr_filename=$2

# Check parameters

if [ ! -e "$asbr_filename" ] || [ ! -f "$asbr_filename" ] ; then
    echo "Invalid path to ASBR file [$asbr_filename]"
    exit 1
fi

# Process arguments

case "$sampling" in
random)
    if [ $# -ne 3 ] ; then
        echo "Missing 3rd argument (integer value)" >&2
        usage
        exit 1
    fi
    num_prefixes=$3
    make_prefixes="$SCRIPT_DIR/make_prefixes_random.py $asbr_filename $num_prefixes"
    ;;
all)
    make_prefixes="$SCRIPT_DIR/make_prefixes_all.py $asbr_filename "
    ;;
*)
    echo "Invalid 2nd argument [$sampling], should be either 'all' or 'random'" >&2
    usage
    exit 1
    ;;
esac

# Loop for each value of {1..\$num_asbr}

max_num_asbr=`seq 1 $(wc -l $asbr_filename | cut -d" " -f1)`
for num_asbr in $max_num_asbr ; do
    output_file="prefix${num_asbr}.csv"
    echo "Writting $output_file"
    eval "$make_prefixes" "$num_asbr" > "$output_file"
done

exit 0
