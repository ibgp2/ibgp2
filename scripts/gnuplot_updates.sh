#!/bin/sh
# Ok this script is totally crappy and hardcoded, sorry...

dataset="article"
result_dir="$HOME/git/ibgp2/results/$dataset"

num_routers=8
x2tics='("8" 1,"28" 2,"56" 3,"70" 4,"56" 5,"28" 6,"8" 7,"1" 8)'

for mode in rr fm ibgp2
do
    echo -ne "" > "$result_dir"/updates_${mode}.dat
done

for i in $(seq $num_routers)
do
    for mode in rr fm ibgp2
    do
        filename_dat="$result_dir/updates_${mode}.dat"
        echo "Writting $filename_dat"
        # We don't care about measurements done on our extern BGP peer (nh)
        # IP 2.x.x.x correspond to our eBGP peer (nh)
#       num_rcvd=$(grep -nr UPDATE ../results/article/$mode/prefix${i}.csv | grep -v files-nh | grep rcvd | grep -v "BGP: 2\." | grep -vc DENIED)
        num_sent=$(grep -nr UPDATE "$result_dir"/"$mode"/prefix${i}.csv | grep -v files-nh | grep send | grep -vc "BGP: 2\.")
        echo "$i\t$num_sent" >> "$filename_dat"
    done
done

filename_wext="$result_dir/updates"
filename_gnu="${filename_wext}.gnu"
filename_eps="${filename_wext}.eps"

echo "
# terminal
set term postscript color eps 18

# Line style
set border linewidth 1.5
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 2 lc rgb '#00ad60' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 3 lc rgb '#ad6000' lt 1 lw 2 pt 7 pi -1 ps 1.5
set pointintervalbox 3

# Caption
set key on outside right

# settings
set out \"$filename_eps\"

# Title
#set title \"CPU usage.\"

# X-axis
set xlabel \"|C|\"
set xrange [0:]
set xtics 1
set xtics out

# X2-axis
set x2label \"\#BGP prefixes involved in the run.\"
set x2tics 1
num_routers = $num_routers
set x2range [0:num_routers + 1]
set x2tics $x2tics
set x2tics out

# Y-axis
set ylabel \"\#iBGP updates sent\"
set yrange [0:]
set ytics 100

# Curves
set style fill solid 0.25 border
set style histogram errorbars gap 2 lw 1
set style data histogram
set grid ytics
set xrange [0:]

box_size = 0.2
plot \\
\"$result_dir/updates_fm.dat\" using (-1 * box_size + \$1):2:(box_size) ls 1 with boxes title \"FM\",\\
\"$result_dir/updates_ibgp2.dat\" using (0 * box_size + \$1):2:(box_size) ls 2 with boxes title \"iBGP2\",\\
\"$result_dir/updates_rr.dat\" using (1 * box_size + \$1):2:(box_size) ls 3 with boxes title \"RR\"

" > "$filename_gnu"

echo "Writting $filename_eps"
gnuplot $filename_gnu
exit 0
