#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Usage:
#   ./gnuplot3_benchmark.py dataset
#
# Example:
#   ./gnuplot3_benchmark.py article
#
# Outputs:
#   $gnuplot_dir/convergence_time_sec_benchmark.{gnu, eps}
#   $gnuplot_dir/diversity_benchmark.{gnu, eps}
#   $gnuplot_dir/mem_usage_kib_benchmark.{gnu, eps}
#   $gnuplot_dir/num_recv_message_benchmark.{gnu, eps}


from __future__     import print_function
from __future__     import with_statement # Required in 2.5

import os, subprocess, sys

#------------------------------------------------------------
# Constants
#------------------------------------------------------------

GNU_COMMON = """
# terminal
set term postscript color eps 25

# Line style
set border linewidth 1.5
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 2 lc rgb '#00ad60' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 3 lc rgb '#ad6000' lt 1 lw 2 pt 7 pi -1 ps 1.5
set pointintervalbox 3

# Caption
#set key on inside top left
set key on outside right
"""

GNU_SETTINGS = """
# Title
#set title "%(title)s"

# X-axis
set xlabel "%(xlabel)s"
set xrange %(xrange)s
set xtics %(xtics)s
set xtics out

# X2-axis
set x2label "#BGP prefixes involved in the run."
set x2tics 1
num_routers = %(num_routers)s
set x2range [0:num_routers + 1]
set x2tics %(x2tics)s
set x2tics out

# Y-axis
set ylabel "%(ylabel)s"
set yrange %(yrange)s
set ytics %(ytics)s

# Curves
set style fill solid 0.25 border
set style histogram errorbars gap 2 lw 1
set style data histogram
set grid ytics
set xrange [0:]
"""

# .dat containing min/avg/max
SETTINGS = {
    "convergence_time_sec_" : {
        "title"  : "iBGP convergence.",
        "xlabel" : "|C|",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "ylabel" : "Convergence time (in s)",
        "yrange" : "[0:]",
        "ytics"  : 5
    },
    "diversity_"            : {
        "title"  : "BGP route diversity.",
        "xlabel" : "|C|",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "ylabel" : "#Routes",
        "yrange" : "[0:]",
        "ytics"  : 1
    },
    "mem_usage_kib_"        : {
        "title"  : "Memory usage.",
        "ylabel" : "RAM used (only RIB entries) (in kb)",
        "xlabel" : "|C|",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "ylabel" : "RAM used (in kb)",
        "yrange" : "[0:]",
        "ytics"  : 10
    },
    "ram_all_kib_"        : {
        "prefix" : "ram_all_kib_",
        "title"  : "Memory usage.",
        "xlabel" : "|C|",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "ylabel" : "RAM used (in kb)",
        "yrange" : "[0:]",
        "ytics"  : 10
    },
    "num_recv_message_"     : {
        "title"  : "CPU usage.",
        "xlabel" : "|C|",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "ylabel" : "#BGP updates received",
        "yrange" : "[0:]",
        "ytics"  : 10
    },
    "diversity_"            : {
        "title"  : "BGP route diversity.",
        "xlabel" : "|C|",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "ylabel" : "Avg #Routes / prefix",
        "yrange" : "[0:]",
        "ytics"  : 1
    },
    "optimality_"            : {
        "title"  : "Fm-optimality.",
        "ylabel" : "%pair fm-optimal",
        "xrange" : "[0:]",
        "xtics"  : 1,
        "xlabel" : "|C|",
        "yrange" : "[0:]",
        "ytics"  : 10
    }
}

GNU_FS = """
# settings
data_dir="%(gnuplot_dir)s"
set out data_dir."%(filename_eps)s"
"""

INF_USAGE = """
usage: %(prog_name)s output_dataset_dir
    Args:
        output_dataset_dir: the directory containing all the outputs
            related to a given dataset.
            Example:
                %(prog_name)s ~/git/ibgp2/results/article/
    Outputs:
        \$gnuplot_dir/convergence_time_sec_benchmark.{gnu, eps}
        \$gnuplot_dir/diversity_benchmark.{gnu, eps}
        \$gnuplot_dir/mem_usage_kib_benchmark.{gnu, eps}
        \$gnuplot_dir/num_recv_message_benchmark.{gnu, eps}
"""

SUFFIX_BENCHMARK = "benchmark"
SUFFIX_MEAN = "mean"

#------------------------------------------------------------
# Log
#------------------------------------------------------------

def info(*objs):
    """
    Print an info.
    Args:
        objs: The objects to print.
    """
    print("INFO:", *objs, file = sys.stdout)

def error(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("ERROR:", *objs, file = sys.stderr)

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


#------------------------------------------------------------
# Gnuplot
#
# Each curves (eps file) is produced according to a
# (.dat, .gnu) pair of input files.
#
# The following function generates the .gnu files. This
# work is split in several print_* functions.
#------------------------------------------------------------

def print_common_def(ofs_gnu):
    """
    Print gnuplot settings shared by all the file written
    by this script.
    Args:
        ofs_gnu: Output stream to the gnuplot file.
    """
    print(GNU_COMMON, file = ofs_gnu)

def print_fs(ofs_gnu, gnuplot_dir, filename_eps):
    """
    Write in a gnuplot file the information related to the
    filesystem (location of the *.dat files containing the
    points, path of the *.eps file, etc.).
    Args:
        ofs_gnu: Output stream to the gnuplot file.
        gnuplot_dir: Directory containing the *.dat files.
        filename_eps: Path of the resulting eps file.
    """
    print(GNU_FS % {
        "gnuplot_dir"  : os.path.abspath(gnuplot_dir) + os.path.sep,
        "filename_eps" : os.path.basename(filename_eps)
    }, file = ofs_gnu)

def print_settings(ofs_gnu, settings, filenames_dat, prefix):
    """
    Prints gnuplot settings.
    Args:
        ofs_gnu: Output stream to the gnuplot file.
        settings: A dict instance. Example: setting["diversity_"]
        filenames_dat: A list of absolute paths corresponding to the input dat files.
    """
    # Patch to add number of prefixes
    num_routers = 8
    settings["num_routers"] = num_routers
    settings["x2tics"] = make_x2_tics(num_routers)
    # end patch

    print(GNU_SETTINGS % settings, file = ofs_gnu)
    print("box_size = 0.2", file = ofs_gnu)
    print("plot\\", file = ofs_gnu)

    imax = len(filenames_dat)
    offset = - (imax - 1) / 2
    i = 0

    # Sort filenames_dat to always use the same style for each ibgp mode on each curves.
    for filename_dat in sorted(filenames_dat):
        # title
        title = None
        filename_dat = os.path.abspath(filename_dat)
        if   "/rr/gnuplot"    in filename_dat: title = "RR"
        elif "/fm/gnuplot"    in filename_dat: title = "FM"
        elif "/ibgp2/gnuplot" in filename_dat: title = "iBGP2"

        if prefix != "optimality_":
            # box
            print(
                '"%(filename_dat)s" using (%(offset)s * box_size + $1):3:(box_size) ls %(ls)s with boxes %(title)s,\\' % {
                    "filename_dat" : filename_dat,
                    "offset"       : i + offset,
                    "ls"           : i + 1,
                    "title"        : ("title \"%s\"" % title) if title != None else "notitle",
                },
                file = ofs_gnu
            )
            # yerror
            print(
                '"%(filename_dat)s" using (%(offset)s * box_size + $1):3:2:4 ls %(ls)s with error notitle%(eol)s' % {
                    "filename_dat" : filename_dat,
                    "offset"       : i + offset,
                    "ls"           : i + 1,
                    "eol"    : ",\\" if i + 1 < imax else ""
                },
                file = ofs_gnu
            )
        else:
            # box
            print(
                '"%(filename_dat)s" using (%(offset)s * box_size + $1):2:(box_size) ls %(ls)s with boxes %(title)s,\\' % {
                    "filename_dat" : filename_dat,
                    "offset"       : i + offset,
                    "ls"           : i + 1,
                    "title"        : ("title \"%s\"" % title) if title != None else "notitle",
                },
                file = ofs_gnu
            )

        i += 1

def make_gnu_all(ofs_gnu, gnuplot_dir, filenames_dat, filename_eps, settings, prefix):
    """
    Write the gnuplot file related to a curve.
    Args:
        ofs_gnu: Output stream to the gnuplot file.
        gnuplot_dir: Directory containing the *.dat files.
        filenames_dat: The list of input *.dat files.
        filename_eps: Path of the resulting eps file.
        settings: Settings related to this curve.
    """
    print_common_def(ofs_gnu)
    print_fs(ofs_gnu, gnuplot_dir, filename_eps)
    print_settings(ofs_gnu, settings, filenames_dat, prefix)

def call_gnuplot(filename_gnu):
    """
    Process a gnu file with gnuplot.
    Args:
        filename_gnu: The absolute path of the gnuplot file.
    """
    with open(os.devnull, "w") as fnull:
        subprocess.Popen(["gnuplot", filename_gnu], stderr = fnull)

#------------------------------------------------------------
# Main program
#------------------------------------------------------------

def print_usage(output_file = sys.stderr):
    """
    Print the usage of this program.
    Args:
        output_file: The output file.
    """
    print(INF_USAGE % {"prog_name" : sys.argv[0]}, file = output_file)

def main():
    """
    Main program.
    """
    # Check arguments
    argc = len(sys.argv)
    if argc != 2:
        print_usage()
        return 1

    gnuplot_dir = sys.argv[1]

    for prefix in SETTINGS.keys():
        # DEBUG
        if prefix != "convergence_time_sec_":
            print("g3: DEBUG: skip")

        # Find files matching the prefix
        filenames_dat = set()
        basename_expected = "%s%s.dat" % (prefix, SUFFIX_MEAN)
        for root, directories, filenames in os.walk(gnuplot_dir):
            for filename in filenames:
                if os.path.basename(filename) == basename_expected:
                    filename_dat = os.path.join(root, filename)
                    filenames_dat.add(filename_dat)

        filename_wext = os.path.abspath(os.path.join(gnuplot_dir, "%s%s" % (prefix, SUFFIX_BENCHMARK)))
        filename_gnu = "%s.gnu" % filename_wext
        filename_eps = "%s.eps" % filename_wext

        if len(filenames_dat) > 0:
            with open(filename_gnu, "w") as ofs_gnu:
                info("Writting %s" % filename_gnu)
                make_gnu_all(ofs_gnu, gnuplot_dir, filenames_dat, filename_eps, SETTINGS[prefix], prefix)
                info("Writting %s" % filename_eps)
                call_gnuplot(filename_gnu)
        else:
            error("No file found in [%s] matching [%s]" % (gnuplot_dir, basename_expected))


    return 0

if __name__ == '__main__':
    sys.exit(main())
