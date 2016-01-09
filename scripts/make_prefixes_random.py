#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Usage:
#   ./make_random_prefixes_from_asbrs.py filename_asbr num_prefixes num_asbr > prefix_num_asbr.csv
#
#   with:
#      filename_asbr: The absolute path of the file containing
#        one ASBR name per line.
#      num_prefixes: The number of eBGP prefixes generated.
#      n : The number of ASBR receiving a given prefix.
#
# asbr.txt contains one ASBR router name per line.

from __future__     import print_function
from __future__     import with_statement # Required in 2.5
from random         import randint

import sys
import traceback

def error(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("ERROR:", *objs, file = sys.stderr)

def swap(l, i , j):
    """
    Swap the i-th and the j-th element in a list.
    Args:
        l: The input list.
        i: Index of the first element (0 <= i < len(l)).
        j: Index of the first element (0 <= j < len(l)).
    """
    assert isinstance(l, list)
    assert isinstance(i, int)
    assert isinstance(j, int)
    tmp = l[i]
    l[i] = l[j]
    l[j] = tmp

def sampling_without_replacement(l, k):
    """
    Sample k distinct elements from a list of elements.
    Args:
        l: The input list, which will be reordered.
        k: The number of sampled elements (0 <= k < len(l)).
    Returns:
        None, but l is altered such as the sampling corresponds to l[-k:].
    """
    # Based on http://www.irem.univ-mrs.fr/IMG/pdf/tirage_sans_remise-2.pdf
    p = len(l)
    assert k <= p
    for i in range(k):
        d = randint(1, p)
        swap(l, d - 1, p - 1)
        p -= 1

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

    if argc != 4:
        error("usage: %s filename_asbr num_prefixes num_asbr" % sys.argv[0])
        return 1

    try:
        # Get arguments
        filename_asbr = sys.argv[1]
        num_prefixes = int(sys.argv[2])
        n = int(sys.argv[3])
        asbrs = parse_asbr(filename_asbr)

        # Make outputs
        for i in range(num_prefixes):
            l = list(asbrs)
            sampling_without_replacement(l, n)

            # Write one line per couple (prefix, asbr)
            # eBGP /24 prefixes starts from 100.*.*.*/24
            for asbr in l[-n:]:
                prefix = make_ipv4_prefix_24(i + 1, n + 100)
                print("%s\t%s" % (asbr, prefix))

            # Skip a line between each prefix
            if i != num_prefixes - 1:
                print()

    except Exception, e:
        traceback.print_exc()
        error(e)
        return 2

    return 0

if __name__ == '__main__':
    sys.exit(main())

