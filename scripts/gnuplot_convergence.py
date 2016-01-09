#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__     import print_function
from __future__     import with_statement # Required in 2.5

import os, re, subprocess, sys

GNU_SCRIPT = """
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
set out "%(filename_eps)s"

# Title
#set title "iBGP convergence"

# X-axis
set xlabel "|C|"
set xrange [0:]
set xtics 1
set xtics out

# X2-axis
set x2label "#BGP prefixes involved in the run."
set x2tics 1
num_routers = %(num_routers)s
set x2range [0:num_routers + 1]
set x2tics %(x2tics)s
set x2tics out

# Y-axis
set ylabel "Convergence time (s)"
set yrange [0:20]
set ytics 1

# Curves
set style fill solid 0.25 border
set style histogram errorbars gap 2 lw 1
set style data histogram
set grid ytics
set xrange [0:]

box_size = 0.2
plot \\
"%(filename_fm)s"    using (-1 * box_size + 1 + $0):3:(box_size) ls 1 with boxes title "FM",\\
"%(filename_fm)s"    using (-1 * box_size + 1 + $0):3:2:4 ls 1 with error notitle,\\
"%(filename_ibgp2)s" using ( 0 * box_size + 1 + $0):3:(box_size) ls 2 with boxes title "iBGP2",\\
"%(filename_ibgp2)s" using ( 0 * box_size + 1 + $0):3:2:4 ls 2 with error notitle,\\
"%(filename_rr)s"    using ( 1 * box_size + 1 + $0):3:(box_size) ls 3 with boxes title "RR",\\
"%(filename_rr)s"    using ( 1 * box_size + 1 + $0):3:2:4 ls 3 with error notitle
"""

def combin(n, k):
    # http://python.jpvweb.com/mesrecettespython/doku.php?id=combinaisons
    if k > n//2:
        k = n-k
    x = 1
    y = 1
    i = n-k+1
    while i <= n:
        x = (x*i)//y
        y += 1
        i += 1
    return x

def make_x2_tics(num_routers):
    """
    Make x2tics according to the number of router.
    We count how many prefixes was used in during for each
    run i (from 1 to |T|)
    Args:
        num_routers: The number of router in the simulated AS (|T|).
    Returns:
        The strings that can be passed to "set x2tics" in gnuplot.
    """
    s = '('
    for i in range(1, num_routers + 1):
        s += "\"%d\" %d%s" % (
            combin(num_routers, i),
            i,
            "," if i < num_routers else ""
        )
    s += ')'
    return s

def call_gnuplot(filename_gnu):
    """
    Process a gnu file with gnuplot.
    Args:
        filename_gnu: The absolute path of the gnuplot file.
    """
    with open(os.devnull, "w") as fnull:
        subprocess.Popen(["gnuplot", filename_gnu], stderr = fnull)

def info(*objs):
    """
    Print an info.
    Args:
        objs: The objects to print.
    """
    print("INFO:", *objs, file = sys.stdout)

MODES = ["ibgp2", "rr", "fm"]

PATTERN_SPACE       = "\s+"
PATTERN_IPV4        = "(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
PATTERN_PREFIXV4    = "(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/\d{1,2})"
PATTERN_DATE        = "[0-9]{4}/[0-9]{2}/[0-9]{2}"
PATTERN_HH          = "([0-2]?[0-9])"
PATTERN_MM          = "([0-5][0-9])"
PATTERN_SS          = "([0-5][0-9])"
PATTERN_HHMMSS      = ":".join([PATTERN_HH, PATTERN_MM, PATTERN_SS])
REGEXP_SEND = re.compile(PATTERN_SPACE.join([PATTERN_DATE, PATTERN_HHMMSS, "BGP:", PATTERN_IPV4, "send UPDATE", PATTERN_PREFIXV4]))

def main():
    dataset_dir = "../results/article"
    router_names = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'] # skip nh
    num_routers = len(router_names)

    map_points = dict()
    for mode in MODES:
        if mode not in map_points.keys(): map_points[mode] = dict()
        for i in range(1, num_routers + 1):
            if i not in map_points[mode].keys(): map_points[mode][i] = dict()

            run_dir = os.path.join(dataset_dir, mode, "prefix%s.csv" % i)
            map_run_cv = dict()
            for router_name in router_names:
                filename_log = os.path.join(run_dir, "files-%s" % router_name, "var", "log", "bgpd.log")
                with open(filename_log) as f:
                    for line in f:
                        m = REGEXP_SEND.match(line)
                        if m:
                            hh = int(m.group(1))
                            mm = int(m.group(2))
                            ss = int(m.group(3))
                            t = 3600 * hh + 60 * mm + ss
                            neighbor = m.group(4)
                            prefix = m.group(5)

                            if neighbor.startswith("2."):
                                continue
                            try:
                                t_cur = map_points[mode][i][prefix]
                                if t < t_cur:
                                    map_points[mode][i][prefix] = t
                            except KeyError:
                                if prefix not in map_points[mode][i].keys():
                                    map_points[mode][i][prefix] = dict()
                                map_points[mode][i][prefix] = t

    # Make dat files.
    filename_dat_pattern = os.path.abspath(os.path.join(dataset_dir, "cv_%s.dat"))
    for mode, dic1 in map_points.items():
        filename_dat = filename_dat_pattern % mode
        with open(filename_dat, "w") as ofs_dat:
            info("Writting %s" % filename_dat)
            for i, dic2 in dic1.items():
                avg = 0.0
                min = None
                max = None
                num = 0
                for prefix, value in dic2.items():
                    if min == None or value < min: min = value
                    if max == None or value > max: max = value
                    avg += value
                    num += 1
                avg /= num
                print("%s %s %s %s" % (i, min, avg, max), file = ofs_dat)

    # Make gnu and eps files.
    filename_wext = os.path.abspath(os.path.join(dataset_dir, "cv"))
    filename_gnu = "%s%s" % (filename_wext, ".gnu")
    filename_eps = "%s%s" % (filename_wext, ".eps")
    with open(filename_gnu, "w") as ofs_gnu:
        info("Writting %s" % filename_gnu)
        filename_ibgp2 = filename_dat_pattern % "ibgp2"
        filename_rr = filename_dat_pattern % "rr"
        filename_fm = filename_dat_pattern % "fm"
        x2tics = make_x2_tics(num_routers)
        print(GNU_SCRIPT % locals(), file = ofs_gnu)
        info("Writting %s" % filename_eps)
        call_gnuplot(filename_gnu)

if __name__ == '__main__':
    sys.exit(main())

