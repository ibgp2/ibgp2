#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author:
#   Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Usage:
#   ./check_inputs.py filename_igp filename_ibgp filename_prefixes
#
#   with:
#     * filename_igp: A file describing the IGP graph, containing for each line:
#
#           router_src router_dst metric
#
#          with router_src: a router name (string)
#               router_dst: a router name (string)
#               metric: IGP metric (unsigned int)
#
#     * filename_ibgp: A file describing the iBGP graph containing for each line:
#
#           router_src router_dst ibgp_label
#
#          with router_src: a router name (string)
#               router_dst: a router name (string)
#               ibgp_label: a value among UP, OVER, DOWN (string)
#                   UP   : router_src is a client of the RR router_dst
#                   OVER : router_src and router_dst are peers
#                   DOWN : router_src is a RR of router_dst (you could also pass OVER)
#
#     * filename_asbr: A file containing for each line
#
#             asbr prefix
#
#           with asbr  : a router name (string) (AS border router)
#                prefix: an IPv4 prefix
#
# Overview:
#   This script checks:
#
#   1) whether the IGP metrics and iBGP label are set in the both directions.
#   - If an arc is defined several time a warning is issued.
#   - If an arc is defined in only one direction, a warning is issued.
#
#   2) whether the ASBR routers set is included in the iBGP routers set which is also included in the IGP router set.

from __future__     import print_function
from __future__     import with_statement # Required in 2.5
from random         import randint

import re, sys, traceback

PATTERN_SPACE         = "\s+"
PATTERN_IPV4          = "(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
PATTERN_PREFIXV4      = "(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/\d{1,2})"
PATTERN_INT           = "(\d+)"
PATTERN_HOSTNAME      = "(\w+)"
PATTERN_IBGP          = "(OVER|UP|DOWN)"

PATTERN_IGP_LINE      = PATTERN_SPACE.join([PATTERN_HOSTNAME, PATTERN_HOSTNAME, PATTERN_INT])
PATTERN_IBGP_LINE     = PATTERN_SPACE.join([PATTERN_HOSTNAME, PATTERN_HOSTNAME, PATTERN_IBGP])
PATTERN_PREFIXES_LINE = PATTERN_SPACE.join([PATTERN_HOSTNAME, PATTERN_PREFIXV4])

REGEXP_IGP_LINE       = re.compile(PATTERN_IGP_LINE)
REGEXP_IBGP_LINE      = re.compile(PATTERN_IBGP_LINE)
REGEXP_PREFIXES_LINE  = re.compile(PATTERN_PREFIXES_LINE)

def info(*objs):
    """
    Print an info.
    Args:
        objs: The objects to print.
    """
    print("INFO:", *objs, file = sys.stdout) #sys.stderr)


def warning(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("WARNING:", *objs, file = sys.stdout) #sys.stderr)

def error(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("ERROR:", *objs, file = sys.stdout) #sys.stderr)

INFINITY = 999999999
DEFAULT_REVERSE_METRIC = 666666666

def parse_igp(filename_igp, reverse_igp, default_metric = DEFAULT_REVERSE_METRIC):
    """
    Parse a file storing for each line a pair of routers and
    the corresponding IGP metric.
    Args:
        filename_igp: The absolute path of the file.
        reverse_igp: Pass True to automatically set up the link in the both directions.
        default_metric:
            Pass the default IGP metric (ignored if reverse_igp = False).
            If you pass DEFAULT_REVERSE_METRIC the forward metric is used.
    Returns:
        A dict mapping for each pair of router (s, t) such as s < t the corresponding
        pair of IGP metrics.
    """
    igp = dict()
    with open(filename_igp) as f:
        for line in f:
            line = line.rstrip("\r\n")
            if len(line) == 0:
                continue
            m = REGEXP_IGP_LINE.match(line)
            if m:
                router_src = m.group(1)
                router_dst = m.group(2)
                metric     = int(m.group(3))

                if router_src == router_dst:
                    warning("In IGP file [%(filename_igp)s]: invalid link (%(router_src)s, %(router_dst)s)" % locals())
                    continue

                swap = router_src > router_dst
                link = (router_dst, router_src) if swap else (router_src, router_dst)

                # Define the reverse metric
                reverse_metric = INFINITY
                if reverse_igp == True:
                    if default_metric == DEFAULT_REVERSE_METRIC:
                        reverse_metric = default_metric
                    else:
                        assert(default_metric > 0)
                        reverse_metric = default_metric

                # Define new metric couple
                (new_metric1, new_metric2) = (reverse_metric, metric) if swap else (metric, reverse_metric)

                # Merge old metric couple and new metric couple
                try:
                    (old_metric1, old_metric2) = igp[link]
                    if old_metric1 != INFINITY and old_metric1 != default_metric and new_metric1 != INFINITY and new_metric1 != old_metric1:
                        warning("In IGP file [%(filename_igp)s]: line [%(line)s]: several distinct metrics are set: %(old_metric1)s %(new_metric)1s ..." % locals())
                    if old_metric2 != INFINITY and old_metric2 != default_metric and new_metric2 != INFINITY and new_metric2 != old_metric2:
                        warning("In IGP file [%(filename_igp)s]: line [%(line)s]: several distinct metrics are set: %(old_metric2)s %(new_metric2)s ..." % locals())
                    new_metric1 = min(new_metric1, old_metric1)
                    new_metric2 = min(new_metric2, old_metric2)
                    igp[link] = (new_metric1, new_metric2)
                except KeyError:
                    igp[link] = (new_metric1, new_metric2)
            else:
                warning("In IGP file [%(filename_igp)s]: ignored line [%(line)s]" % locals())

    for link, metrics in igp.items():
        (metric1, metric2) = metrics
        if metric1 == INFINITY or metric2 == INFINITY:
            warning("In IGP file [%(filename_igp)s]: asymmetric IGP link %(link)s: %(metrics)s" % locals())
        #elif metric1 != INFINITY and metric2 != INFINITY:
        #    info("[ok] IGP link %(link)s with metrics %(metrics)s" % locals())

    return igp

UP = 1
OVER = 2
DOWN = 3
UNSET = 4

def int_to_ibgp(i):
    """
    Convert an integer related to an iBGP session label to the corresponding String.
    Args:
        i: A value among UP, OVER, DOWN, UNSET.
    Returns:
        The corresponding string if i is in {UP, OVER, DOWN}, None otherwise.
    """
    if   i == 1: return "UP"
    elif i == 2: return "OVER"
    elif i == 3: return "DOWN"
    return None

def select_ibgp(ibgp1, ibgp2):
    """
    Select among two iBGP label the one the less constraining the iBGP route spread.
    Args:
        ibgp1: A value among UP, OVER, DOWN, UNSET
        ibgp2: A value among UP, OVER, DOWN, UNSET
    Returns:
        The corresponding label.
    """
    if ibgp1 == DOWN : ibgp1 = OVER
    if ibgp2 == DOWN : ibgp2 = OVER
    return min(ibgp1, ibgp2)

def ibgp_sym(ibgp):
    """
    Args:
        ibgp: A value among {UP, OVER, DOWN}
    Returns:
        The symmetric iBGP label, UNSET if invalid.
    """
    if   ibgp == UP:   return DOWN
    elif ibgp == OVER: return OVER
    elif ibgp == DOWN: return UP
    return UNSET

def parse_ibgp(filename_ibgp, reverse_ibgp):
    """
    Parse a file storing for each line a pair of routers and
    the corresponding iBGP session
    Args:
        filename_ibgp: The absolute path of the file.
        reverse_ibgp: Pass True to automatically set up the link in the both directions.
    Returns:
        A set gathering all the corresponding iBGP sessions.
    """
    ibgp_sessions = dict()
    with open(filename_ibgp) as f:
        for line in f:
            line = line.rstrip("\r\n")
            if len(line) == 0:
                continue
            m = REGEXP_IBGP_LINE.match(line)
            if m:
                router_src = m.group(1)
                router_dst = m.group(2)

                # Parse iBGP label
                ibgp       = m.group(3)
                if   ibgp == "OVER" : ibgp = OVER
                elif ibgp == "UP"   : ibgp = UP
                elif ibgp == "DOWN" : ibgp = DOWN
                else:
                    warning("In iBGP file [%(filename_ibgp)s]: line [%(line)s]: invalid session type)" % locals())
                    continue

                if router_src == router_dst:
                    warning("In iBGP file [%(filename_ibgp)s]: invalid link (%(router_src)s, %(router_dst)s)" % locals())
                    continue

                swap = router_src > router_dst
                link = (router_dst, router_src) if swap else (router_src, router_dst)
                reverse_label = ibgp_sym(ibgp) if reverse_ibgp == True else UNSET
                (new_ibgp1, new_ibgp2) = (reverse_label, ibgp) if swap else (ibgp, reverse_label)

                # Merge new session couple
                try:
                    (old_ibgp1, old_ibgp2) = ibgp_sessions[link]
                    if old_ibgp1 != UNSET and new_ibgp1 != UNSET and old_ibgp1 != new_ibgp1:
                        warning("In iBGP file [%(filename_ibgp)s]: line [%(line)s]: conflicting session)" % locals())
                    if old_ibgp2 != UNSET and new_ibgp2 != UNSET and old_ibgp2 != new_ibgp2:
                        warning("In iBGP file [%(filename_ibgp)s]: line [%(line)s]: conflicting session)" % locals())
                    new_ibgp1 = select_ibgp(new_ibgp1, old_ibgp1)
                    new_ibgp2 = select_ibgp(new_ibgp2, old_ibgp2)
                    ibgp_sessions[link] = (new_ibgp1, new_ibgp2)
                except KeyError:
                    ibgp_sessions[link] = (new_ibgp1, new_ibgp2)

            else:
                warning("In iBGP file [%(filename_ibgp)s]: ignored line [%(line)s]" % locals())

    for link, ibgp in ibgp_sessions.items():
        (ibgp1, ibgp2) = ibgp

        if ibgp1 == UNSET or ibgp2 == UNSET:
            warning("In iBGP file [%(filename_ibgp)s]: asymmetric iBGP link %(link)s: %(ibgp)s" % locals())
        elif ibgp1 == UP and ibgp2 == UP:
            warning("In iBGP file [%(filename_ibgp)s]: iBGP2-like link %(link)s" % locals())
        #elif (ibgp1 == UP or ibgp1 == OVER) and (ibgp2 == UP or ibgp2 == OVER):
        #    info("[ok] iBGP link %(link)s %(ibgp)s" % locals())

    return ibgp_sessions

def parse_prefixes(filename_prefixes):
    """
    Parse a file storing for each line an ASBR router name and an input eBGP prefix.
    Args:
        filename_prefixes: The absolute path of the file.
    Returns:
        A dict which maps for each ASBR the corresponding input prefixes.
    """
    prefixes = dict()
    with open(filename_prefixes) as f:
        for line in f:
            line = line.rstrip("\r\n")
            if len(line) == 0:
                continue
            m = REGEXP_PREFIXES_LINE.match(line)
            if m:
                asbr   = m.group(1)
                prefix = m.group(2)
                if asbr not in prefixes.keys(): prefixes[asbr] = set()
                prefixes[asbr].add(prefix)
            else:
                warning("In iBGP file [%(filename_ibgp)s]: ignored line [%(line)s]" % locals())

    return prefixes

def check_consistency(igp, ibgp, prefixes):
    """
    Check whether data are consistent or not.
    Args:
        igp: see parse_igp()
        ibgp: see parse_ibgp()
        prefixes: see parse_prefixes()
    Returns:
        Three sets are returned:
        ibgp_not_igp: contains iBGP routers which are not in the IGP topology.
        igp_not_ibgp: contains IGP routers which are not in the iBGP topology.
        asbr_not_bgp: contains ASBR that are not IGP and/or not iBGP.
    """
    ret = True

    routers_igp = set()
    for link in igp.keys():
        (router_src, router_dst) = link
        routers_igp.add(router_src)
        routers_igp.add(router_dst)

    routers_ibgp = set()
    for link in ibgp.keys():
        (router_src, router_dst) = link
        routers_ibgp.add(router_src)
        routers_ibgp.add(router_dst)

    asbrs = set(prefixes.keys())

    ibgp_not_igp = routers_ibgp - routers_igp
    if len(ibgp_not_igp) > 0:
        error("The following routers are BGP but not in the IGP topology: %s" % sorted(ibgp_not_igp))
        ret = False

    igp_not_ibgp = routers_igp - routers_ibgp
    if len(igp_not_ibgp) > 0:
        warning("The following routers are in the IGP topology and do not run BGP: %s" % sorted(igp_not_ibgp))
        ret = False

    asbr_not_bgp = asbrs - (routers_ibgp & routers_igp)
    if len (asbr_not_bgp) > 0:
        error("The following routers are ASBR but do not run BGP: %s" % sorted(asbr_not_bgp))
        ret = False

    return (ibgp_not_igp, igp_not_ibgp, asbr_not_bgp)

def main():
    # Check arguments
    argc = len(sys.argv)

    if argc < 4 or argc > 6:
        error("usage: %s filename_igp filename_ibgp filename_prefixes [reverse_igp] [reverse_ibgp]" % sys.argv[0])
        return 1

    # Get arguments
    filename_igp      = sys.argv[1]
    filename_ibgp     = sys.argv[2]
    filename_prefixes = sys.argv[3]
    reverse_igp       = int(sys.argv[4]) != 0 if argc >= 5 else False
    reverse_ibgp      = int(sys.argv[5]) != 0 if argc >= 6 else False

    if reverse_igp:
        info("Reverse IGP link will be automatically set.")
    if reverse_ibgp:
        info("Reverse iBGP session will be automatically set.")

    ibgp_not_igp = set()
    igp_not_ibgp = set()
    asbr_not_bgp = set()
    try:

        igp = parse_igp(filename_igp, reverse_igp, default_metric = DEFAULT_REVERSE_METRIC)
        ibgp = parse_ibgp(filename_ibgp, reverse_ibgp)
        prefixes = parse_prefixes(filename_prefixes)
        (ibgp_not_igp, igp_not_ibgp, asbr_not_bgp) = check_consistency(igp, ibgp, prefixes)

    except Exception, e:
        traceback.print_exc()
        error(e)
        return 2

    make_fixed_files = True

    if make_fixed_files:
        suffix_fixed = ".fixed"

        # Print fixed IGP file
        filename_igp_fixed  = filename_igp + suffix_fixed
        with open(filename_igp_fixed, 'w') as f_igp:
            info("Writting %s" % filename_igp_fixed)
            for link, metrics in igp.items():
                (router_src, router_dst) = link
                (metric1, metric2) = metrics
                if metric1 != INFINITY:
                    print("%(router_src)s\t%(router_dst)s\t%(metric1)s" % locals(), file = f_igp)
                if metric2 != INFINITY:
                    print("%(router_dst)s\t%(router_src)s\t%(metric2)s" % locals(), file = f_igp)

        # Print fixed iBGP file
        filename_ibgp_fixed = filename_ibgp + suffix_fixed
        with open(filename_ibgp_fixed, 'w') as f_ibgp:
            info("Writting %s" % filename_ibgp_fixed)
            for link, ibgp in ibgp.items():

                (router_src, router_dst) = link
                if (router_src in ibgp_not_igp) or (router_dst in ibgp_not_igp):
                    info("Removing iBGP session %(router_src)s -- %(router_dst)s (not in IGP topology)" % locals())
                    continue

                (ibgp1, ibgp2) = ibgp
                ibgp1 = int_to_ibgp(ibgp1)
                ibgp2 = int_to_ibgp(ibgp2)
                if ibgp1 != None and ibgp2 != None:
                    print("%(router_src)s\t%(router_dst)s\t%(ibgp1)s" % locals(), file = f_ibgp)
                    print("%(router_dst)s\t%(router_src)s\t%(ibgp2)s" % locals(), file = f_ibgp)

        # Print fixed prefixes file
        filename_prefixes_fixed = filename_prefixes + suffix_fixed
        with open(filename_prefixes_fixed, 'w') as f_prefixes:
            info("Writting %s" % filename_prefixes_fixed)
            for asbr, prefs in prefixes.items():
                if asbr in asbr_not_bgp:
                    info("Removing eBGP prefix for ASBR %(asbr)s (not in IGP and BGP topologies)" % locals())
                    continue
                for prefix in prefs:
                    print("%(asbr)s\t%(prefix)s" % locals(), file = f_prefixes)

    return 0

if __name__ == '__main__':
    sys.exit(main())

