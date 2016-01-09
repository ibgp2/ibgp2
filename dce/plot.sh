#/bin/bash

. env.sh

OUTPUT_DIR="$IBGP2_GIT/results"

date="$(date +%Y-%m-%d_%H:%M:%S)"

dataset="test"
mode="ibgp2"
dataset_dir="$IBGP2_GIT/datasets/$dataset"
input_igp="$dataset_dir/igp.csv"
input_prefixes="$dataset_dir/prefix.csv"
result_dir="$OUTPUT_DIR/$dataset/$mode/$(basename $input_prefixes)" #$date"

graph_dir=$result_dir/out
graph_maker="python $HOME/git/ibgp2/stage/scripts/graph_maker/graph_maker.py"

# Save the results
mkdir -p "$result_dir"
cp -r "$NS3_DCE_DIR"/files-* "$result_dir"
cp -r "$NS3_DCE_DIR"/routes*.log "$result_dir"

# Plot the results
$graph_maker $graph_dir $(find $result_dir/files-* | grep "stdout$" | xargs)

pushd "$graph_dir"
# Gnuplot the results
for x in $(ls -1 *.gnu); do gnuplot $x; done
# Graphviz the results
#for x in $(ls -1 *.dot); do dot     $x; done
popd
    
exit 0
