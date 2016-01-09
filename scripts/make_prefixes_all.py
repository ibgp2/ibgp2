#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Usage:
#   ./make_all_prefixes.py filename_asbr num_asbrs > ebgp.txt
#
#   with:
#      filename_asbr: The absolute path of the file containing
#        one ASBR name per line.
#      num_asbrs: The number of eBGP prefixes generated.
#
# asbr.txt contains one ASBR router name per line.

from __future__     import print_function
from __future__     import with_statement # Required in 2.5
from itertools      import combinations

import sys, traceback

def error(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("ERROR:", *objs, file = sys.stderr)

def parse_asbr(filename_asbr):
    """
    Parse a text file having one router name per line.
    Args:
        filename_asbr: The absolute path of the file.
    Returns:
        A set gathering all the corresponding router names.
    """
    asbrs = set()
    with open(filename_asbr) as f:
        for line in f:
            asbr = line.rstrip("\r\n")
            asbrs.add(asbr)
    return asbrs

def make_ipv4_prefix_24(i, n):
    """
    Bijection from ([1, 255], [1, 255]) to prefix space.
    """
    assert i > 0 and i < 256, "i = %s" % i
    assert n > 0 and n < 256, "n = %s" % n
    return "%s.%s.0.0/24" % (n, i)

def main():
    # Check arguments
    argc = len(sys.argv)

    if argc != 3:
        error("usage: %s filename_asbr num_asbrs" % sys.argv[0])
        return 1

    try:
        # Get arguments
        filename_asbr = sys.argv[1]
        num_asbrs = int(sys.argv[2])
        asbrs = parse_asbr(filename_asbr)

        # Make outputs
        i = 0
        for asbrs_subset in combinations(asbrs, num_asbrs):
            prefix = make_ipv4_prefix_24(i + 1, num_asbrs + 100)
            i += 1
            for asbr in asbrs_subset:
                print("%s\t%s" % (asbr, prefix))
            print()

    except Exception, e:
        traceback.print_exc()
        error(e)
        return 2

    return 0

if __name__ == '__main__':
    sys.exit(main())

