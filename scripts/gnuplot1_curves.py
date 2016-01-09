#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Author: Marc-Olivier Buob <marcolivier.buob@orange.fr>
#
# Usage:
#   ./gnuplot1_curves.py simu_dir simulation_duration
#
# Example:
#   ./gnuplot1_curves.py ~/git/ibgp2/results/article/rr/ 75
#
# Outputs:
#   ./*.dat : points deduced from the zebra's outputs
#   ./*.gnu : gnuplot script allowing to produce ./*.eps according to ./*.dat
#   ./*.eps : corresponding curves

from __future__     import print_function
from __future__     import with_statement # Required in 2.5
from datetime       import timedelta
from netaddr        import IPAddress, IPNetwork
from pprint         import pprint

import errno, os, re, subprocess, sys, tempfile

#==========================================================================
# Useful regexps
#==========================================================================

PATTERN_SPACE       = "\s+"
PATTERN_IPV4        = "(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"
PATTERN_PREFIXV4    = "(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/\d{1,2})"
PATTERN_HH          = "([0-2]?[0-9])"
PATTERN_MM          = "([0-5][0-9])"
PATTERN_SS          = "([0-5][0-9])"
PATTERN_HHMMSS      = ":".join([PATTERN_HH, PATTERN_MM, PATTERN_SS])
PATTERN_INT         = "(\d+)"
PATTERN_RAM         = "(bytes?|KiB|MiB|GiB|TiB)"

def ram_to_multiplier(ram_unit):
    """
    Convert a RAM unit into the corresponding multiplier to get memory in bytes.
    Args:
        ram_unit: A String among "byte", "bytes", "KiB", "MiB", "GiB", "TiB".
    Raises:
        ValueError: if ram_unit is not valid.
    Returns:
        The corresponding int.
    """
    if   ram_unit == "byte" or ram_unit == "bytes": return 1
    elif ram_unit == "KiB": return 1024
    elif ram_unit == "MiB": return 1048576
    elif ram_unit == "GiB": return 1073741824
    elif ram_unit == "TiB": return 1099511627776
    else:
        raise ValueError("Invalid RAM unit '%s'" % ram_unit)

#==========================================================================
# Messages
#==========================================================================

INF_OPTIMAL = """
(%(router_name)s, %(network)s) is optimal
Selected (metric: %(selected_metric)d): %(selected_next_hop)s
Optimal (metric %(optimal_metric)s: %(optimal_nexthops)s
"""

WNG_SUBOPTIMAL = """
(%(router_name)s, %(network)s) is SUBOPTIMAL:
  Selected (metric: %(selected_metric)d): %(selected_next_hop)s
  Optimal (metric %(optimal_metric)s: %(optimal_nexthops)s
"""

WNG_DISTINCT_DIVERSITIES = "There are %d eBGP prefixes having distinct max diversities (%s) in this run, so the RAM(x,y,z) and CPU(x,y,z) points cannot be deduced."

ERR_NEXTHOP_UNREACHABLE = "%(router_name)s didn't learnt BGP route toward the next hop %(next_hop)s: try to increase --stopTime in the simulation"
ERR_PREFIX_UNREACHABLE = "%(router_name)s didn't learnt BGP route toward %(network)s: if the topology is iBGP valid, try to increase --stopTime in the simulation"

ERR_CANNOT_WRITE = "Cannot write %s: %s"

#==========================================================================
# Log
#==========================================================================

def info(*objs):
    """
    Print an info.
    Args:
        objs: The objects to print.
    """
    print("INFO:", *objs, file = sys.stdout)

def warning(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("WARNING:", *objs, file = sys.stderr)

def error(*objs):
    """
    Print an error.
    Args:
        objs: The objects to print.
    """
    print("ERROR:", *objs, file = sys.stderr)

#==========================================================================
# Filesystem
#==========================================================================

def check_readable_file(filename):
    """
    Test whether a file can be read.
    Args:
        filename: A String containing the absolute path of this file.
    Raises:
        RuntimeError: If the directory cannot be created.
    """
    if not os.path.isfile(filename):
        raise RuntimeError("%s is not a regular file" % filename)
    try:
        f = open(filename, "r")
        f.close()
    except:
        raise RuntimeError("%s cannot be read" % filename)

def check_writable_directory(directory):
    """
    Tests whether a directory is writable.
    Args:
        directory: A String containing an absolute path.
    Raises:
        RuntimeError: If the directory does not exists or isn't writable.
    """
    if not os.path.exists(directory):
        raise RuntimeError("Directory '%s' does not exists" % directory)
    if not os.access(directory, os.W_OK | os.X_OK):
        raise RuntimeError("Directory '%s' is not writable" % directory)
    try:
        with tempfile.TemporaryFile(dir = directory):
            pass
    except Exception, e:
        raise RuntimeError("Cannot write into directory '%s': %s" % (directory, e))

def mkdir(directory):
    """
    Create a directory (mkdir -p).
    Args:
        directory: A String containing an absolute path.
    Raises:
        OSError: If the directory cannot be created.
    """
    # http://stackoverflow.com/questions/600268/mkdir-p-functionality-in-python
    try:
        if not os.path.exists(directory):
            info("Creating '%s' directory" % directory)
        os.makedirs(directory)
    except OSError as e: # Python >2.5
        if e.errno == errno.EEXIST and os.path.isdir(directory):
            pass
        else:
            raise OSError("Cannot mkdir %s: %s" % (directory, e))

#--------------------------------------------------------------------------
# Simulation outputs filesystem
#--------------------------------------------------------------------------

# Patterns filesystem
PATTERN_NODE_DIRNAME = "files-(.+)"
REGEXP_NODE_DIRNAME = re.compile(PATTERN_NODE_DIRNAME)

def get_router_names(run_dir):
    """
    Create the list of router names according to the hierarchy
    stored in run_dir.
    Args:
        run_dir: The directory storing the outputs of the run.
    Returns:
        The list of router names.
    """
    router_names = list()

    for filename in os.listdir(run_dir):
        path = os.path.join(run_dir, filename)
        if not os.path.isdir(path):
            continue

        m = REGEXP_NODE_DIRNAME.match(filename)
        if m:
            router_name = m.group(1)
            router_names.append(router_name)

    return router_names

def make_output_filename(run_dir, daemon_name, router_name):
    """
    Craft the zebra's output filename.
    Args:
        run_dir: The directory storing the outputs of the run.
        daemon_name: The daemon name ("bgpd"|"zebra")
        router_name: The name of the router.
    Returns:
        The path related to zebra's output run on the router.
    """
    assert daemon_name in ["bgpd", "zebra"], "Invalid daemon_name = %s" % daemon_name
    return os.path.join(run_dir, "%s_%s.txt" % (daemon_name, router_name))

#==========================================================================
# Parsing zebra
#==========================================================================

#--------------------------------------------------------------------------
# Parsing "show ip route"
#--------------------------------------------------------------------------

class zebra_route_t:
    def __init__(self):
        """
        Constructor.
        """
        self.type        = None # K|C|S|R|O|I|B
        self.is_selected = None # >
        self.is_in_fib   = None # *
        self.destination = None # destination
        self.via         = None # via (BGP nexthop)
        self.up_time     = None # Uptime of the route

    def __str__(self):
        """
        Returns:
            The "%s" representation of this zebra_route_t.
        """
        return "%(type)s%(is_selected)s%(is_in_fib)s %(destination)s via %(via)s, %(up_time)s" % {
            "type"        : self.type,
            "is_selected" : ">" if self.is_selected else " ",
            "is_in_fib"   : "*" if self.is_in_fib   else " ",
            "destination" : self.destination,
            "via"         : self.via,
            "up_time"     : self.up_time
        }

PATTERN_ZEBRA_ROUTE_TYPE  = "(K|C|S|R|O|I|B)"
PATTERN_ZEBRA_IS_SELECTED = "(>)?"
PATTERN_ZEBRA_IS_IN_FIB   = "(\*)?"
PATTERN_ZEBRA_ROUTE = \
    PATTERN_ZEBRA_ROUTE_TYPE + \
    PATTERN_ZEBRA_IS_SELECTED + \
    PATTERN_ZEBRA_IS_IN_FIB + "\s+" + \
    PATTERN_PREFIXV4 + ".*via " + \
    PATTERN_IPV4 + ".* " + \
    PATTERN_HHMMSS + "?"

REGEXP_ZEBRA_ROUTE = re.compile(PATTERN_ZEBRA_ROUTE)

def parse_show_ip_route(ifs_zebra):
    """
    Parse "show ip route" outputs.
    Args:
        ifs_zebra: The input iterable containing the outputs.
    Returns:
        The resulting list of zebra_route_t instances.
    """
    zebra_routes = list()
    for line in ifs_zebra:
        zebra_route = zebra_route_t()
        line = line.rstrip("\r\n")
        m = REGEXP_ZEBRA_ROUTE.match(line)
        if m:
            zebra_route.type        =  m.group(1)
            zebra_route.is_selected = (m.group(2) == ">")
            zebra_route.is_in_fib   = (m.group(3) == "*")
            zebra_route.destination = IPNetwork(m.group(4))
            zebra_route.via         = IPAddress(m.group(5))
            zebra_route.up_time     = timedelta(
                hours   = int(m.group(6)),
                minutes = int(m.group(7)),
                seconds = int(m.group(8))
            )
            zebra_routes.append(zebra_route)
    return zebra_routes

#--------------------------------------------------------------------------
# Parsing zebra
#--------------------------------------------------------------------------

def parse_zebra(ifs_zebra, router_name):
    """
    Process the zebra's outputs.
    Args:
        ifs_zebra: Input iterable (over the zebra's outputs.)
        router_name: The name of the router.
    Returns:
        A dict mapping for each command the corresponding (last) result.
    """
    regexp_prompt = re.compile("^%s> (.+)" % router_name)
    ret = dict()
    for line in ifs_zebra:
        line = line.rstrip("\r\n")
        m = regexp_prompt.match(line)
        if m:
            command = m.group(1)
            if command == "show ip route":
                if command not in ret.keys():
                    ret[command] = dict()
                ret[command][router_name] = parse_show_ip_route(ifs_zebra)
            else:
                info("Ignoring [%s] outputs" % command)
    return ret

#==========================================================================
# Parsing bgpd
#==========================================================================

#--------------------------------------------------------------------------
# RAM~\cite{bonaventure2004case}. We evaluate the CPU impact of the both
# approaches by comparing we compare the number of exchanged messages on each
# router (updates and keep alives). This is achieved by running the command
# \texttt{show ip neighbors} on bgpd.
#--------------------------------------------------------------------------

class bgp_neighbor_t:
    def __init__(self):
        """
        Constructor.
        """
        self.ip = None
        self.remote_as = None
        self.local_as = None
        self.mrai = None
        self.stats = dict()

    def __str__(self):
        """
        Returns:
            The "%s" representation of this bgp_neighbor_t.
        """
        return "(ip = %(ip)s remote_as = %(remote_as)d local_as = %(local_as)d mrai = %(mrai)d %(stats)s)" % {
            "ip"        : self.ip,
            "remote_as" : self.remote_as,
            "local_as"  : self.local_as,
            "mrai"      : self.mrai,
            "stats"     : self.stats
        }

PATTERN_BGPD_MESSAGE_TYPE = "(Opens|Notification|Updates|Keepalives|Route Refresh|Capability|Total)"
PATTERN_BGPD_MESSAGE_COUNTER = \
    PATTERN_SPACE + PATTERN_BGPD_MESSAGE_TYPE + ":" + \
    PATTERN_SPACE + PATTERN_INT + \
    PATTERN_SPACE + PATTERN_INT

PATTERN_BGPD_MRAI = PATTERN_SPACE + "Minimum time between advertisement runs is " + PATTERN_INT + " seconds"
REGEXP_BGPD_MESSAGE_COUNTER = re.compile(PATTERN_BGPD_MESSAGE_COUNTER)
REGEXP_PATTERN_BGPD_MRAI    = re.compile(PATTERN_BGPD_MRAI)

def parse_bgp_neighbor(ifs_bgpd):
    """
    Parse information related to a single neighbor in "show ip bgp neighbor"  outputs.
    Args:
        ifs_bgpd: The input iterable containing the outputs.
    Returns:
        The resulting list of zebra_route_t instances.
    """
    bgp_neighbor = bgp_neighbor_t()
    regexps = [REGEXP_BGPD_MESSAGE_COUNTER, REGEXP_PATTERN_BGPD_MRAI]
    for line in ifs_bgpd:
        line = line.rstrip("\r\n")
        for regexp in regexps:
            m = regexp.match(line)
            if m:
                if regexp == REGEXP_PATTERN_BGPD_MRAI:
                    bgp_neighbor.mrai = int(m.group(1))
                    return bgp_neighbor
                elif regexp == REGEXP_BGPD_MESSAGE_COUNTER:
                    msg_type = m.group(1).lower()
                    sent     = int(m.group(2))
                    recv     = int(m.group(3))
                    bgp_neighbor.stats[msg_type] = {
                        "sent" : sent,
                        "recv" : recv
                    }
    return bgp_neighbor

PATTERN_BGPD_NEIGHBOR_BEGIN = \
    "BGP neighbor is " + PATTERN_IPV4 + \
    ", remote AS "     + PATTERN_INT  + \
    ", local AS "      + PATTERN_INT  + \
    ".*"

PATTERN_BGPD_NEIGHBORS_END = \
    "Read thread: (\w+) Write thread: (\w+)"

REGEXP_BGPD_NEIGHBOR_BEGIN = re.compile(PATTERN_BGPD_NEIGHBOR_BEGIN)
REGEXP_BGPD_NEIGHBORS_END  = re.compile(PATTERN_BGPD_NEIGHBORS_END)

def parse_show_ip_bgp_neighbors(ifs_bgpd):
    """
    Parse "show ip bgp neighbors" outputs.
    Args:
        ifs_bgpd: The input iterable containing the outputs.
    Returns:
        The resulting list of zebra_route_t instances.
    """
    bgp_neighbors = list()
    regexps = [REGEXP_BGPD_NEIGHBOR_BEGIN, REGEXP_BGPD_NEIGHBORS_END]
    for line in ifs_bgpd:
        line = line.rstrip("\r\n")
        for regexp in regexps:
            m = regexp.match(line)
            if m:
                if regexp == REGEXP_BGPD_NEIGHBOR_BEGIN:
                    bgp_neighbor =  parse_bgp_neighbor(ifs_bgpd)
                    bgp_neighbor.neighbor_ip = IPAddress(m.group(1))
                    bgp_neighbor.local_as    = int(m.group(2))
                    bgp_neighbor.remote_as   = int(m.group(3))
                    bgp_neighbors.append(bgp_neighbor)
                elif regexp == REGEXP_BGPD_NEIGHBORS_END:
                    return stats_neighbors
    return bgp_neighbors

#--------------------------------------------------------------------------
# Parsing "show ip bgp summary"
#
# Similarly, we retrieve the RAM usage by running the command \texttt{show ip bgp
# summary}.
#--------------------------------------------------------------------------

class stats_ip_bgp_summary_t:
    def __init__(self):
        """
        Constructor.
        """
        self.num_entries     = 0
        self.num_peers       = 0
        self.mem_rib_bytes   = 0 # RIB entries
        self.mem_peers_bytes = 0 # Peers

    def __str__(self):
        """
        Returns:
            The string representing this stats_ip_bgp_summary_t instance.
        """
        return "(num_entries = %d mem_rib_bytes = %d num_peers = %d mem_peers_bytes = %d)" % (
            self.num_entries,
            self.mem_rib_bytes,
            self.num_peers,
            self.mem_peers_bytes
        )

# TODO : Use PATTERN_RAM, merge REGEXP_MEM_PEERS*, merge REGEXP_MEM_RIB*
REGEXP_NUM_NEIGHBORS = re.compile("Total number of neighbors (\d+)")
REGEXP_MEM_RIB       = re.compile("RIB entries (\d+), using (\d+) bytes of memory")
REGEXP_MEM_RIB_KIB   = re.compile("RIB entries (\d+), using (\d+) KiB of memory")
REGEXP_MEM_PEERS     = re.compile("Peers (\d+), using (\d+) bytes of memory")
REGEXP_MEM_PEERS_KIB = re.compile("Peers (\d+), using (\d+) KiB of memory")
REGEXP_NO_NEIGHBOR   = re.compile("No IPv4 neighbor is configured")

def parse_show_ip_bgp_summary(ifs_bgpd):
    """
    Parse "show ip bgp summary" outputs.
    Args:
        ifs_bgpd: The input iterable containing the outputs.
    Returns:
        The resulting stats_ip_bgp_summary_t instance.
        None in case of problem.
    """
    ret = stats_ip_bgp_summary_t()
    regexps = [
        REGEXP_NUM_NEIGHBORS, REGEXP_MEM_RIB, REGEXP_MEM_RIB_KIB,
        REGEXP_MEM_PEERS, REGEXP_MEM_PEERS_KIB, REGEXP_NO_NEIGHBOR
    ]
    mem_rib_set   = False
    mem_peers_set = False
    for line in ifs_bgpd:
        line = line.rstrip("\r\n")
        for regexp in regexps:
            m = regexp.match(line)
            if m:
                if regexp == REGEXP_NUM_NEIGHBORS:
                    assert mem_rib_set == True
                    assert mem_peers_set == True
                    return ret
                elif regexp == REGEXP_MEM_RIB:
                    ret.num_entries     = int(m.group(1))
                    ret.mem_rib_bytes   = int(m.group(2))
                    mem_rib_set         = True
                elif regexp == REGEXP_MEM_RIB_KIB:
                    ret.num_entries     = int(m.group(1))
                    ret.mem_rib_bytes   = 1024 * int(m.group(2))
                    mem_rib_set         = True
                elif regexp == REGEXP_MEM_PEERS:
                    ret.num_peers       = int(m.group(1))
                    ret.mem_peers_bytes = int(m.group(2))
                    mem_peers_set       = True
                elif regexp == REGEXP_MEM_PEERS_KIB:
                    ret.num_peers       = int(m.group(1))
                    ret.mem_peers_bytes = 1024 * int(m.group(2))
                    mem_peers_set       = True
                elif regexp == REGEXP_NO_NEIGHBOR:
                    return None

    # Those both flag must be true (if we assume that 'show ip bgp summary' results
    # are stored in ifs_bgpd. Otherwise our parser might have missed the information.
    assert mem_rib_set == True
    assert mem_peers_set == True

    return ret

#--------------------------------------------------------------------------
# Parsing show ip bgp
#--------------------------------------------------------------------------

class bgp_route_t:
    def __init__(self):
        """
        Constructor.
        """
        self.is_valid   = None
        self.is_best    = None
        self.origin     = None
        self.network    = None
        self.next_hop   = None
        self.metric     = None
        self.local_pref = None
        self.weight     = None

    def __str__(self):
        """
        Returns:
            The %s representation of this bgp_route_t.
        """
        return "%(is_valid)s%(is_best)s%(origin)s %(network)s\t%(next_hop)s\t%(metric)d\t%(local_pref)s\t%(weight)d" % {
            "is_valid"   : "*" if self.is_valid else " ",
            "is_best"    : ">" if self.is_best  else " ",
            "origin"     : self.origin,
            "network"    : self.network,
            "next_hop"   : self.next_hop,
            "metric"     : self.metric,
            "local_pref" : self.local_pref,
            "weight"     : self.weight
        }

    def __repr__(self):
        """
        Returns:
            The %r representation of this bgp_route_t.
        """
        return "%(is_best)s%(origin)s %(network)s\t%(next_hop)s\t" % {
            "is_best"    : ">" if self.is_best  else " ",
            "origin"     : self.origin,
            "network"    : self.network,
            "next_hop"   : self.next_hop,
        }

PATTERN_BGPD_IS_VALID = "(\*)?"
PATTERN_BGPD_IS_BEST  = "(>|[ ])?"
PATTERN_BGPD_ORIGIN   = "(e|i|[?]|[ ])"
PATTERN_BGPD_ROUTE = \
    PATTERN_BGPD_IS_VALID + \
    PATTERN_BGPD_IS_BEST  + \
    PATTERN_BGPD_ORIGIN   + \
    PATTERN_PREFIXV4 + "?" + PATTERN_SPACE + \
    PATTERN_IPV4           + PATTERN_SPACE + \
    PATTERN_INT            + PATTERN_SPACE + \
    PATTERN_INT      + "?" + PATTERN_SPACE + \
    PATTERN_INT            + PATTERN_SPACE + \
    ".* "

REGEXP_BGPD_ROUTE        = re.compile(PATTERN_BGPD_ROUTE)
REGEXP_BGPD_NUM_PREFIXES = re.compile("Total number of prefixes " + PATTERN_INT)
REGEXP_BGPD_NO_NETWORK   = re.compile("No BGP network exists")

def parse_show_ip_bgp(ifs_bgpd):
    """
    Parse "show ip bgp neighbors" outputs.
    Args:
        ifs_bgpd: The input iterable containing the outputs.
    Returns:
        The BGP RIB, which is a  dict(IPNetwork : list(bgp_route_t)) mapping
        for each prefix the list of corresponding BGP routes.
        None in case of problem.
    """
    bgp_rib = dict()
    regexps = [REGEXP_BGPD_ROUTE, REGEXP_BGPD_NUM_PREFIXES, REGEXP_BGPD_NO_NETWORK]
    network = None # This information is not repeated on each line
    for line in ifs_bgpd:
        line = line.rstrip("\r\n")
        for regexp in regexps:
            m = regexp.match(line)
            if m:
                if regexp == REGEXP_BGPD_ROUTE:
                    # Rebuild route
                    bgp_route = bgp_route_t()
                    bgp_route.is_valid   = m.group(1) == "*"
                    bgp_route.is_best    = m.group(2) == ">"
                    bgp_route.origin     = m.group(3) if m.group(3) != " " else "e"
                    if m.group(4): # The network is not reminded on each line.
                        network = IPNetwork(m.group(4))
                    bgp_route.network    = network
                    bgp_route.next_hop   = IPAddress(m.group(5))
                    bgp_route.metric     = int(m.group(6))
                    bgp_route.local_pref = int(m.group(7)) if m.group(7) != None else None
                    bgp_route.weight     = int(m.group(8))

                    # Save route
                    if network not in bgp_rib.keys():
                        bgp_rib[network] = list()
                    bgp_rib[network].append(bgp_route)
                elif regexp == REGEXP_BGPD_NUM_PREFIXES:
                    return bgp_rib
                elif regexp == REGEXP_BGPD_NO_NETWORK:
                    warning("parse_show_ip_bgp: %s" % line)
                    return None
    return bgp_rib

def bgp_rib_get_route(bgp_rib, network):
    """
    Retrieve the BGP route toward a given prefix (exact matching) from
    a BGP RIB.
    Args:
        bgp_rib: The BGP RIB.
        network: An IPNetwork set to the destination prefix.
    Returns:
        The corresponding selected bgp_route_t if any, None otherwise.
    """
    try:
        bgp_routes = bgp_rib[network]
        for bgp_route in bgp_routes:
            if bgp_route.is_best: return bgp_route
    except KeyError:
        pass
    return None

#--------------------------------------------------------------------------
# Parsing "show bgp memory"
#--------------------------------------------------------------------------

REGEXP_RAM_RIB_NODES        = re.compile(PATTERN_SPACE.join([PATTERN_INT, "RIB nodes, using",             PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_BGP_ROUTES       = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP routes, using",            PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_BGP_ANCILLARIES  = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP route ancillaries, using", PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_STATIC_ROUTES    = re.compile(PATTERN_SPACE.join([PATTERN_INT, "Static routes, using",         PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_ADJIN            = re.compile(PATTERN_SPACE.join([PATTERN_INT, "Adj-In entries, using",        PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_ADJOUT           = re.compile(PATTERN_SPACE.join([PATTERN_INT, "Adj-Out entries, using",       PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_NH_CACHE         = re.compile(PATTERN_SPACE.join([PATTERN_INT, "Nexthop cache entries, using", PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_DAMPENING        = re.compile(PATTERN_SPACE.join([PATTERN_INT, "Dampening entries, using",     PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_BGP_ATTS         = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP attributes, using",        PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_BGP_EXT_ATTRS    = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP extra attributes, using",  PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_AS_PATH_ENTRIES  = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP AS-PATH entries, using",   PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_AS_PATH_SEGMENTS = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP AS-PATH segments, using",  PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_BGP_COMMUNITIES  = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP community entries, using", PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_EXT_COMMUNITIES  = re.compile(PATTERN_SPACE.join([PATTERN_INT, "BGP community entries, using", PATTERN_INT, PATTERN_RAM,  "of memory"])) # Yes quagga use the same string :(
REGEXP_RAM_CLUSTER_LISTS    = re.compile(PATTERN_SPACE.join([PATTERN_INT, "Cluster lists, using",         PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_PEERS            = re.compile(PATTERN_SPACE.join([PATTERN_INT, "peers, using",                 PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_PEER_GROUPS      = re.compile(PATTERN_SPACE.join([PATTERN_INT, "peer groups, using",           PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_HASH_TABLES      = re.compile(PATTERN_SPACE.join([PATTERN_INT, "hash tables, using",           PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_HASH_BUCKETS     = re.compile(PATTERN_SPACE.join([PATTERN_INT, "hash buckets, using",          PATTERN_INT, PATTERN_RAM,  "of memory"]))
REGEXP_RAM_REGEXES          = re.compile(PATTERN_SPACE.join([PATTERN_INT, "compiled regexes, using",      PATTERN_INT, PATTERN_RAM,  "of memory"]))

def parse_show_bgp_memory(ifs_bgpd):
    """
    Parse "show bgp memory" outputs.
    Args:
        ifs_bgpd: The input iterable containing the outputs.
    Returns:
        The RAM used.
    """
    bgp_mem_total = 0
    regexps = [
        REGEXP_RAM_RIB_NODES, REGEXP_RAM_BGP_ROUTES, REGEXP_RAM_BGP_ANCILLARIES,
        REGEXP_RAM_STATIC_ROUTES, REGEXP_RAM_ADJIN, REGEXP_RAM_ADJOUT,
        REGEXP_RAM_NH_CACHE, REGEXP_RAM_DAMPENING, REGEXP_RAM_BGP_ATTS,
        REGEXP_RAM_BGP_EXT_ATTRS, REGEXP_RAM_AS_PATH_ENTRIES, REGEXP_RAM_AS_PATH_SEGMENTS,
        REGEXP_RAM_BGP_COMMUNITIES, REGEXP_RAM_EXT_COMMUNITIES, REGEXP_RAM_CLUSTER_LISTS,
        REGEXP_RAM_PEERS, REGEXP_RAM_PEER_GROUPS, REGEXP_RAM_HASH_TABLES,
        REGEXP_RAM_HASH_BUCKETS, REGEXP_RAM_REGEXES
    ]

    network = None # This information is not repeated on each line
    for line in ifs_bgpd:
        line = line.rstrip("\r\n")

        matched = False
        for regexp in regexps:
            m = regexp.match(line)
            if m:
                quantity = int(m.group(2))
                multiplier = ram_to_multiplier(m.group(3))
                bgp_mem_total += quantity * multiplier
                matched = True
                break

        if not matched:
            warning("parse_show_bgp_memory: unhandled line = %s" % line)

    return bgp_mem_total

#--------------------------------------------------------------------------
# Parsing bgpd
#--------------------------------------------------------------------------

def iter_bgpd(ifs_bgpd, router_name):
    """
    Iterate over each couple (command, outputs) where:
    - command is a String containing a quagga command (without prompt)
    - outputs is a list of String corresponding to each line returned.
    Args:
        ifs_bgpd: Input iterable (over the bgpd's outputs.)
        router_name: The name of the router.
    Returns;
        The next (command, outputs) pairs at each iteration.
    """
    command = None
    outputs = list()
    regexp_prompt = re.compile("^%s> (.+)" % router_name)
    for line in ifs_bgpd:
        line = line.rstrip("\r\n")
        m = regexp_prompt.match(line)
        if m:
            if command:
                yield command, outputs
            outputs = list()
            command = m.group(1)
        else:
            outputs.append(line)
    yield command, outputs

def parse_bgpd(ifs_bgpd, router_name):
    """
    Process the bgpd's outputs.
    Args:
        ifs_bgpd: Input iterable (over the bgpd's outputs.)
        router_name: The name of the router.
    Returns:
        A dict mapping for each command the corresponding (last) result.
    """
    ret = dict()
    for command, outputs in iter_bgpd(ifs_bgpd, router_name):
        if command == "show ip bgp summary":
            ret[command] = parse_show_ip_bgp_summary(outputs)
        elif command == "show ip bgp":
            ret[command] = parse_show_ip_bgp(outputs)
        elif command == "show ip bgp neighbor":
            ret[command] = parse_show_ip_bgp_neighbors(outputs)
        elif command == "show bgp memory":
            ret[command] = parse_show_bgp_memory(outputs)
        else:
            if command != "quit":
                info("Ignoring [%s] outputs" % command)
    return ret

#--------------------------------------------------------------------------
# Parsing bgpd
#--------------------------------------------------------------------------

PATTERN_FIB_ROUTE = \
    PATTERN_IPV4 + PATTERN_SPACE +\
    PATTERN_IPV4 + PATTERN_SPACE +\
    PATTERN_IPV4 + PATTERN_SPACE +\
    "(\w)+"      + PATTERN_SPACE +\
    PATTERN_INT  + PATTERN_SPACE +\
    "([^\s])+"   + PATTERN_SPACE +\
    "([^\s])+"   + PATTERN_SPACE +\
    PATTERN_INT

REGEXP_FIB_ROUTE = re.compile(PATTERN_FIB_ROUTE)

def parse_fib_routes(ifs_fib):
    """
    Parse a FIB (forwarding table) linux like (see /sbin/route -n)
    Args:
        ifs_fib: Input iterable (over the FIB log.)
    Returns:
        A dict {IPNetwork : dict()} mapping each prefix of the
        FIB with the corresponding information.
    """
    fib = dict()
    for line in ifs_fib:
        line = line.rstrip("\r\n")

        # The FIB ends with an empty line or EOF
        if line == "":
            break

        m = REGEXP_FIB_ROUTE.match(line)
        if m:
            destination = IPAddress(m.group(1))
            gateway     = IPAddress(m.group(2))
            genmask     = IPAddress(m.group(3))
            flags       = m.group(4)
            metric      = int(m.group(5))
            ref         = m.group(6)
            use         = m.group(7)
            iface       = int(m.group(8))

            prefix = IPNetwork("%s/%s" % (destination, genmask))
            fib[prefix] = {
                "destination" : destination,
                "gateway" : gateway,
                "genmask" : genmask,
                "flags"   : flags,
                "metric"  : metric,
                "ref"     : ref,
                "use"     : use,
                "iface"   : iface
            }

    return fib

def is_included(n1, n2):
    """
    Test whether an IPNetwork is included in another IPNetwork.
    Args:
        n1: An IPNetwork instance.
        n2: An IPNetwork instance.
    Returns:
        True iif n1 is included (or equal) to n2, False otherwise.
    """
    assert isinstance(n1, IPNetwork)
    assert isinstance(n2, IPNetwork)
    return n1.first >= n2.first and n1.last <= n2.last

def fib_get_route(fib, destination):
    """
    Find in a FIB the route related to a given destination IP address.
    Args:
        fib: A dict {IPNetwork : dict()} mapping each prefix of the
          FIB with the corresponding information.
        destination: An IPAddress instance.
    Returns:
        The corresponding route (if any), None otherwise.
    """
    assert isinstance(fib, dict)
    assert isinstance(destination, IPAddress)

    most_specific_network = None
    for network in fib.keys():
        if destination in network:
            if most_specific_network == None or is_included(network, most_specific_network):
                most_specific_network = network
    if most_specific_network:
        return fib[most_specific_network]
    return None

def parse_fib(ifs_fib):
    """
    Process the bgpd's outputs.
    Args:
        ifs_fib: Input iterable (over the FIB log.)
        router_name: The name of the router.
    Returns:
        A dict mapping for each prefix the last corresponding FIB route.
    """
    ret = dict()
    regexp_time = re.compile("Time: " + PATTERN_INT + "s")

    for line in ifs_fib:
        line = line.rstrip("\r\n")
        m = regexp_time.match(line)
        if m:
            ret = parse_fib_routes(ifs_fib)

    return ret

#==========================================================================
# Function used to convert our measurable into distribution / cdf / ...
#==========================================================================

def make_distribution(l):
    """
    Transform a list of measures into the corresponding distribution.
    Args:
        l: The input list.
    Returns:
        A list of tuple (x, n) where x belongs to l and n in the number
        of occurrence of x in l.
    """
    assert isinstance(l, list)
    ret = list()
    v = sorted(l)
    x_prev = None
    for x in v:
        if x != x_prev:
            ret.append((x, 1))
            x_prev = x
        else:
            (_, n_prev) = ret[-1]
            ret[-1] = (x, n_prev + 1)
    return ret

def make_cdf(l):
    """
    Transform a list of measures into the corresponding cdf.
    Args:
        l: The input list.
    Returns:
        A list of tuple (x, n) where x belongs to l and n in the number
        of cumulated occurrence <= x in l.
    """
    assert isinstance(l, list)
    ret = list()
    v = sorted(l)
    x_prev = None
    n = 0
    for x in v:
        n += 1
        if x != x_prev:
            ret.append((x, n))
            x_prev = x
        else:
            (_, n_prev) = ret[-1]
            ret[-1] = (x, n_prev + 1)
    return ret

#==========================================================================
# Gnuplot
#==========================================================================

GNUPLOT_COMMON = """
# terminal
set term postscript color eps 18

set border linewidth 1.5
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 2 lc rgb '#00ad60' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 3 lc rgb '#ad6000' lt 1 lw 2 pt 7 pi -1 ps 1.5
set pointintervalbox 3

set out "%(filename_eps)s"
set title "%(title)s"
set xlabel "%(xlabel)s"
set ylabel "%(ylabel)s"
set yrange [%(ymin)s:%(ymax)s]
"""

#GNUPLOT_XYZ_MODE1 = GNUPLOT_COMMON + """
#set zlabel "%(zlabel)s"
#set zlabel rotate by 90
#set key off
#splot "%(filename_dat)s" using 3:2:1 #with points palette pointsize 3 pointtype 7
#"""
#
#GNUPLOT_XYZ_MODE2 = GNUPLOT_COMMON + """
#set zlabel "%(zlabel)s"
#set view map
#set size ratio .9
#
#set object 1 rect from graph 0, graph 0 to graph 1, graph 1 back
#set object 1 rect fc rgb "black" fillstyle solid 1.0
#
#splot "%(filename_dat)s" using 3:2:1 with points pointtype 5 pointsize 1 palette linewidth 30
#"""

# http://www.gnuplotting.org/tag/linespoints/
GNUPLOT_XY = GNUPLOT_COMMON + """
set xtics 1
set xtics out
set ytics 10
set style data boxes
plot "%(filename_dat)s" using 2:1 with linespoints ls 1 notitle
"""

GNUPLOT_HIST = GNUPLOT_COMMON + """
set style fill solid 0.25 border
set style histogram errorbars gap 2 lw 1
set style data histogram
set grid ytics
set xrange [0:]

bs=0.2 # width of a box
plot "%(filename_dat)s" using 1:3:(bs) ls 1 with boxes notitle,\
     "%(filename_dat)s" using 1:3:2:4 ls 1 with yerror notitle
"""

def call_gnuplot(filename_gnu):
    """
    Process a gnu file with gnuplot.
    Args:
        filename_gnu: The absolute path of the gnuplot file.
    """
    with open(os.devnull, "w") as fnull:
        subprocess.Popen(["gnuplot", filename_gnu], stderr = fnull)

class stats_xyz_t:
    def __init__(self):
        """
        Constructor.
        """
        self.data = dict()
        self.gnuplot = dict()

    def set_gnuplot(self, curve_name, d):
        self.gnuplot[curve_name] = d

    def add(self, curve_name, x, y, z):
        """
        Add a (x,y,z) point for a given curve.
        Args:
            curve_name: the String corresponding to the title of the curve.
            x: data which will be accumulated and plotted.
            y: number of routers in the AS.
            z: number of involved concurrent routes.
        """
        if curve_name not in self.data.keys():
            self.data[curve_name] = dict()
        if z not in self.data[curve_name].keys():
            self.data[curve_name][z] = dict()
        if y not in self.data[curve_name][z]:
            self.data[curve_name][z][y] = list()
        self.data[curve_name][z][y].append(x)

    def write_dat_xy(self, curve_name, z, ofs_dat, normalized = False):
        """
        Write the "dat" content for gnuplot in an output file stream.
        Args:
            curve_name: the String corresponding to the title of the curve.
            z:
            ofs_dat: the output stream.
        """
        print("# y = %s (|S| = %d)" % (curve_name, z), file = ofs_dat)
        print("# %sCDF(|T|)\ty\t|S|" % ("%" if normalized else ""), file = ofs_dat)

        pts = list()
        x_sum = 0
        for y, lx in sorted(self.data[curve_name][z].items()):
            assert isinstance(lx, list)
            x_avg = sum(lx) / len(lx)
            x_sum += x_avg
            pts.append((y, x_avg))
        x_cdf = 0
        for y, x in pts:
            x_cdf += x
            if normalized:
                x_pct = 100.0 * x_cdf / x_sum
                print("%(x_pct)s\t%(y)s\t%(z)s" % locals(), file = ofs_dat)
            else:
                print("%(x_cdf)s\t%(y)s\t%(z)s" % locals(), file = ofs_dat)

    def write_gnu_xy(self, curve_name, ofs_gnu, filename_wext):
        """
        Write the "gnu" content for gnuplot in an output file stream.
        Args:
            curve_name: The String corresponding to the title of the curve.
            ofs_gnu: The output stream.
            filename_wext: The absolute path without extension (of the .dat and .eps)
        """
        d = self.gnuplot[curve_name]
        d["filename_eps"] = filename_wext + ".eps"
        d["filename_dat"] = filename_wext + ".dat"
        print(GNUPLOT_XY % d, file = ofs_gnu)

    def write_gnuplot_files(self, output_dir):
        """
        Write gnuplot files according to the measurement stored in self.
        Args:
            output_dir: The directory where files are written.
        """
        print("coucou: keys = %s" % self.data.keys())
        for curve_name in self.data.keys():
            # (x, y) graphs (for each value of z = |S|)
            pattern_wext = os.path.abspath(os.path.join(output_dir, curve_name + "_%d"))
            pattern_dat = pattern_wext + ".dat"
            pattern_gnu = pattern_wext + ".gnu"

            for z in self.data[curve_name].keys():
                # .dat
                filename_dat = pattern_dat % z
                ofs_dat = open(filename_dat, "w")
                try:
                    info("Writting %s" % filename_dat)
                    self.write_dat_xy(curve_name, z, ofs_dat)
                    ofs_dat.close()
                except IOError:
                    error(ERR_CANNOT_WRITE % (filename_dat, e))

                # .gnu
                filename_gnu = pattern_gnu % z
                ofs_gnu = open(filename_gnu, "w")
                try:
                    info("Writting %s" % filename_gnu)
                    title = self.gnuplot[curve_name]["title"]
                    self.gnuplot[curve_name]["title"] += " (|S| = %d)" % z
                    self.write_gnu_xy(curve_name, ofs_gnu, pattern_wext % z)
                    title = self.gnuplot[curve_name]["title"] = title
                    ofs_gnu.close()
                except IOError:
                    error(ERR_CANNOT_WRITE % (filename_gnu, e))

                # Call gnuplot
                filename_eps = pattern_wext % z + ".eps"
                info("Writting %s" % filename_eps)
                call_gnuplot(filename_gnu)

    def write_gnu_histogram(self, curve_name, ofs_gnu, filename_wext):
        """
        Write the "gnu" content for gnuplot in an output file stream.
        Args:
            curve_name: The String corresponding to the title of the curve.
            ofs_gnu: The output stream.
            filename_wext: The absolute path without extension (of the .dat and .eps)
        """
        d = self.gnuplot[curve_name]
        d["ylabel"] = d["xlabel"]
        d["xlabel"] = "|S|"
        d["filename_eps"] = filename_wext + ".eps"
        d["filename_dat"] = filename_wext + ".dat"
        print(GNUPLOT_HIST % d, file = ofs_gnu)

    def write_gnuplot_mean_files(self, output_dir):
        """
        Write gnuplot files according to the measurement stored in self.
        Args:
            output_dir: The directory where files are written.
        """
        for curve_name in self.data.keys():
            filename_wext = os.path.abspath(os.path.join(output_dir, curve_name + "_mean"))
            filename_dat = filename_wext + ".dat"
            filename_gnu = filename_wext + ".gnu"

            # .dat
            with open(filename_dat, "w") as ofs_dat:
                info("Writting %s" % filename_dat)
                print("#Summary for %s" % curve_name, file = ofs_dat)
                print("#|S|\tmin\tavg\tmax", file = ofs_dat)

                for z in self.data[curve_name].keys():
                    # .dat
                    y_min = y_max = None
                    x_sum = y_sum = 0

                    for y, l in self.data[curve_name][z].items():
                        y_min = min(y, y_min) if y_min != None else y
                        y_max = max(y, y_max) if y_max != None else y
                        lx_sum = sum(l)
                        x_sum += lx_sum
                        y_sum += y * lx_sum

                    y_avg = 1.0 * y_sum / x_sum
                    print("%s\t%.2lf\t%.2lf\t%.2lf" % (z, y_min, 1.0 * y_sum / x_sum, y_max), file = ofs_dat)
                    assert y_min <= y_avg
                    assert y_max >= y_avg

            # .gnu
            with open(filename_gnu, "w") as ofs_gnu:
                info("Writting %s" % filename_gnu)
                title = self.gnuplot[curve_name]["title"]
                self.gnuplot[curve_name]["title"] += " (mean)"
                self.write_gnu_histogram(curve_name, ofs_gnu, filename_wext)
                title = self.gnuplot[curve_name]["title"] = title

            # Call gnuplot
            filename_eps = filename_wext + ".eps"
            info("Writting %s" % filename_eps)
            call_gnuplot(filename_gnu)

#==========================================================================
# Main program
#==========================================================================

def process_output(daemon_name, run_dir, router_name):
    """
    Process the output of a given (daemon, router) couple.
    Args:
        run_dir: The directory storing the outputs of the run.
        daemon_name: The daemon name ("bgpd"|"zebra")
        router_name: The name of the router.
    Returns:
        The corresponding stats_t instance.
    """
    out_filename = make_output_filename(run_dir, daemon_name, router_name)
    check_readable_file(out_filename)
    info("Processing %s" % out_filename)
    ifs = open(out_filename, "r")
    if daemon_name == "bgpd":
        stats = parse_bgpd(ifs, router_name)
    if daemon_name == "zebra":
        stats = parse_zebra(ifs, router_name)
    ifs.close()
    return stats


def process_fib(run_dir, router_name):
    """
    Process the output of a given (daemon, router) couple.
    Args:
        run_dir: The directory storing the outputs of the run.
        router_name: The name of the router.
    Returns:
        A dict mapping for each prefix the corresponding IGP cost.
    """
    route_filename = os.path.join(run_dir, "routes_%s.log" % router_name)
    check_readable_file(route_filename)
    info("Processing %s" % route_filename)
    ifs = open(route_filename, "r")
    ret = parse_fib(ifs)
    ifs.close()
    return ret


KEY_MEM         = "mem_usage_kib"
KEY_RAM         = "ram_all_kib"
KEY_CPU         = "num_recv_message"
KEY_OPTIMALITY  = "pct_optimal_routes"
KEY_DIVERSITY   = "diversity"
KEY_CONVERGENCE = "convergence_time_sec"

IGNORED_ROUTERS = set(["nh"])

def add_measure(d, k, x):
    if k not in d.keys():
        d[k] = list()
    d[k].append(x)

def process_run(run_dir, bgp_simulation_duration_sec, stats_xyz, ofs_opt = sys.stdout):
    """
    Process the outputs of a single run.
    Args:
        run_dir: The directory storing the outputs of the run.
        bgp_simulation_duration_sec: The duration of the BGP convergence.
        stats_xyz: Collects the (x,y,z) points
        ofs_opt: Output file stream for optimality stats.
    """
    # Time delta between the date of the first BGP route announcement and the
    # end of simulation (in seconds)
    router_names = get_router_names(run_dir)
    num_routers = len(router_names)
    assert num_routers > 0, "Can't collect router names from %s" % run_dir

    stats_router           = dict()
    stats_diversity        = dict()
    stats_convergence      = dict()
    ebgp_routes            = dict() # {IPNetwork : set(IPAddress)}: map a prefix with the corresponding NH
    fibs                   = dict() # {String : dict()}: map a router name with its FIB
    bgp_ribs               = dict() # {String : dict()}: map a router name with its BGP RIB

    for router_name in router_names:
        if router_name in IGNORED_ROUTERS:
            info("Skipping %s which is outside the AS" % router_name)
            continue

        stats_bgpd  = process_output("bgpd",  run_dir, router_name)
        stats_zebra = process_output("zebra", run_dir, router_name)
        fibs[router_name] = process_fib(run_dir, router_name)

        # Deduce relevant statistics of interest
        # bgpd> show ip bgp summary : RAM usage
        # (x: #concurrent_route, y: min/avg/max)
        # (x: ccdf(RAM), y: #routers, z: #concurrent_routes)
        stats_ip_bgp_summary = stats_bgpd["show ip bgp summary"]
        if not isinstance(stats_ip_bgp_summary, stats_ip_bgp_summary_t):
            info("Skipping %(router_name)s (see 'show ip bgp summary' result)" % locals())
            IGNORED_ROUTERS.add(router_name)
            continue
        assert isinstance(stats_ip_bgp_summary, stats_ip_bgp_summary_t)
        add_measure(stats_router, KEY_MEM, 1.0 * (stats_ip_bgp_summary.mem_peers_bytes + stats_ip_bgp_summary.mem_rib_bytes) / 1024)

        ram_used = stats_bgpd["show bgp memory"]
        if not isinstance(ram_used, int):
            info("Skipping %(router_name)s (see 'show bgp memory' result)" % locals())
            IGNORED_ROUTERS.add(router_name)
            continue
        add_measure(stats_router, KEY_RAM, 1.0 * ram_used / 1024)

        # bgpd> show ip bgp neighbor : CPU usage
        # z given: (x: #concurrent_route, y: min/avg/max)
        # (x: ccdf(CPU), y: #routers, z: #concurrent_routes)
        bgp_neighbors = stats_bgpd["show ip bgp neighbor"]
        assert isinstance(bgp_neighbors, list)
        num_messages_recv = 0
        for bgp_neighbor in bgp_neighbors:
            num_messages_recv += bgp_neighbor.stats["updates"]["recv"]
        add_measure(stats_router, KEY_CPU, num_messages_recv)

        # bgpd> show ip bgp : diversity and/or optimality
        # z given: (x: #concurrent_route, y: min/avg/max)
        # (x: diversity, y: #routers, z: #concurrent_routes)
        bgp_rib = stats_bgpd["show ip bgp"]
        if not isinstance(bgp_rib, dict):
            info("Skipping %(router_name)s (see 'show ip bgp' result)" % locals())
            IGNORED_ROUTERS.add(router_name)
            continue
        bgp_ribs[router_name] = bgp_rib

        for network, bgp_routes in bgp_rib.items():

            # Compute diversity: for each network, count the number of route
            # related to distinct BGP next hops.
            diversity = 0
            nh_seen = set()
            for bgp_route in bgp_routes:
                if bgp_route.next_hop not in nh_seen:
                    diversity += 1
                    nh_seen.add(bgp_route.next_hop)
            add_measure(stats_diversity, network, diversity)

            # Store prefixes related to eBGP routes.
            for bgp_route in bgp_routes:
                if bgp_route.origin == "e":
                    if network not in ebgp_routes.keys():
                        ebgp_routes[network] = set()
                    ebgp_routes[network].add(bgp_route.next_hop)

        # zebra> show ip route : convergence time and/or optimality
        # z given: (x: #concurrent_route, y: min/avg/max)
        # (x: ccdf(RAM), y: #routers, z: #concurrent_routes)
        zebra_routes = stats_zebra["show ip route"]
        assert isinstance(zebra_routes, dict)
        for router_name, zebra_routes in zebra_routes.items():
            for zebra_route in zebra_routes:
                if zebra_route.type == "B":
                    network = zebra_route.destination
                    convergence_time = bgp_simulation_duration_sec - zebra_route.up_time
                    add_measure(stats_convergence, network, convergence_time.total_seconds())

    # Generate points.

    #"eBGP diversity:"
    #pprint(ebgp_routes)

    assert len(ebgp_routes) > 0
    diversities_max = set([len(next_hops) for next_hops in ebgp_routes.values()])

    if len(diversities_max) != 1:
        warning(WNG_DISTINCT_DIVERSITIES % (len(ebgp_routes.keys()), diversities_max))
    else:
        # |S|: num of concurrent eBGP routes for each prefix
        # |T|: num of router having used "mem" (resp "cpu")
        n = len(ebgp_routes.values()[0])
        num_prefixes = len(ebgp_routes.keys())

        # Each prefixes is related to the same value of |S|, so even if there
        # are several prefixes.
        for cpu, r in make_distribution(stats_router.get(KEY_CPU)):
            stats_xyz.add(KEY_CPU, r, cpu, n)

        for mem, r in make_distribution(stats_router.get(KEY_MEM)):
            stats_xyz.add(KEY_MEM, r, mem, n)

        for mem, r in make_distribution(stats_router.get(KEY_RAM)):
            stats_xyz.add(KEY_RAM, r, mem, n)

    for network, stat in stats_convergence.items():
        # |S|: num of concurrent eBGP routes toward "network"
        # |T|: num of router having converged in "tcv" seconds
        n = len(ebgp_routes[network])
        for tcv, r in make_distribution(stat):
            stats_xyz.add(KEY_CONVERGENCE, r, tcv, n)

    for network, stat in stats_diversity.items():
        # |S|: num of concurrent eBGP routes toward "network"
        # |T|: num of router storing "div" routes toward "network"
        n = len(ebgp_routes[network])
        for div, r in make_distribution(stat):
            stats_xyz.add(KEY_DIVERSITY, r, div, n)

    #-----------------------------------------------------------------------
    # Write output file containing stats regarding iBGP optimality.
    # For each eBGP prefix p, for each target BGP router t, we test
    # - if t has learn a BGP route toward p (if not: invalid)
    # - if t use a route toward its closest egress point (if not: suboptimal)
    # The partition invalid/suboptimal/optimal is then written in the output file.
    #
    # The following is a bit crappy, because the following states are not
    # stored in stats_xyz and that's why they are directly written in the
    # output file.
    #-----------------------------------------------------------------------

    # Routes: test whether the router picked the closest
    # egress point.
    stats_optimality = dict()
    num_pairs_optimal    = 0
    num_pairs_suboptimal = 0
    num_pairs_invalid    = 0
    for network in ebgp_routes.keys():
        n = len(ebgp_routes[network])
        candidate_next_hops = ebgp_routes[network]

        num_routers_optimal    = 0
        num_routers_suboptimal = 0
        num_routers_invalid    = 0
        for router_name in router_names:
            if router_name in IGNORED_ROUTERS:
                continue
            fib = fibs[router_name]

            # Retrieve the closest egress point(s) and the corresponding IGP cost
            # for "router_name" to reach "prefix"
            optimal_nexthops = set()
            optimal_metric  = None
            for next_hop in candidate_next_hops:
                fib_route = fib_get_route(fib, next_hop)
                if fib_route:
                    metric = fib_route["metric"]
                    if optimal_metric == None or (metric <= optimal_metric):
                        optimal_metric = metric
                        if metric < optimal_metric: # reset the optimal_metric set.
                            optimal_nexthops = set()
                        optimal_nexthops.add(next_hop)
                else:
                    error(ERR_NEXTHOP_UNREACHABLE % locals())

            # If at least one nexthop is reachable
            if optimal_metric != None:
                # Retrieve the elected BGP route toward this prefix
                bgp_rib = bgp_ribs[router_name]
                bgp_route = bgp_rib_get_route(bgp_rib, network)
                if bgp_route:
                    selected_next_hop = bgp_route.next_hop
                    assert selected_next_hop in candidate_next_hops, "%s not in %s" % (selected_next_hop, candidate_next_hops)

                    # Retrieve the corresponding IGP cost
                    fib_route = fib_get_route(fib, selected_next_hop)
                    assert isinstance(fib_route, dict)
                    selected_metric = fib_route["metric"]

                    assert selected_metric >= optimal_metric, "%d >= %d"

                    if selected_metric == optimal_metric:
                        assert selected_next_hop in optimal_nexthops
                        #info(INF_OPTIMAL % locals())
                        num_routers_optimal += 1
                    else:
                        warning(WNG_SUBOPTIMAL % locals())
                        num_routers_suboptimal += 1
                else:
                    error(ERR_PREFIX_UNREACHABLE % locals())
                    num_routers_invalid += 1
            else:
                error("%s cannot contact any nexthop to reach network %s" % (router_name, network))
                num_routers_invalid += 1

        # Per network statistics
        num_routers = num_routers_optimal + num_routers_suboptimal + num_routers_invalid
        pct_routers_optimal    = 100.0 * num_routers_optimal    / num_routers
        pct_routers_suboptimal = 100.0 * num_routers_suboptimal / num_routers
        pct_routers_invalid    = 100.0 * num_routers_invalid    / num_routers
        add_measure(stats_optimality, n, (pct_routers_optimal, pct_routers_suboptimal, pct_routers_invalid))

        # Update global statistics
        num_pairs_optimal    += num_routers_optimal
        num_pairs_suboptimal += num_routers_suboptimal
        num_pairs_invalid    += num_routers_invalid

    # Global statistics
    num_pairs = num_pairs_optimal + num_pairs_suboptimal + num_pairs_invalid
    pct_pairs_optimal    = 100.0 * num_pairs_optimal    / num_pairs
    pct_pairs_suboptimal = 100.0 * num_pairs_suboptimal / num_pairs
    pct_pairs_invalid    = 100.0 * num_pairs_invalid    / num_pairs

    info("Pairs stats: %(num_pairs)d (router, eBGP prefixes) pairs" % locals())
    info("  Optimal    : %(num_pairs_optimal)6d pairs (%(pct_pairs_optimal)5.2f%%)" % locals())
    info("  Suboptimal : %(num_pairs_suboptimal)6d pairs (%(pct_pairs_suboptimal)5.2f%%)" % locals())
    info("  Invalid    : %(num_pairs_invalid)6d pairs (%(pct_pairs_invalid)5.2f%%)" % locals())

    # For each value of |S| compute the percentage (average) of router
    for n, triples in stats_optimality.items():
        pct_avg_routers_optimal    = 0.0
        pct_avg_routers_suboptimal = 0.0
        pct_avg_routers_invalid    = 0.0
        for triple in triples:
            (pct_routers_optimal, pct_routers_suboptimal, pct_routers_invalid) = triple
            pct_avg_routers_optimal    += pct_routers_optimal
            pct_avg_routers_suboptimal += pct_routers_suboptimal
            pct_avg_routers_invalid    += pct_routers_invalid
        pct_avg_routers_optimal    /= len(triples)
        pct_avg_routers_suboptimal /= len(triples)
        pct_avg_routers_invalid    /= len(triples)
        print("%(n)d\t%(pct_avg_routers_optimal).2lf\t%(pct_avg_routers_suboptimal).2lf\t%(pct_avg_routers_invalid).2lf" % locals(), file = ofs_opt)

def main():
    """
    Main program.
    Returns:
        An integer storing the execution code (0: success)
    """
    # Check arguments
    argc = len(sys.argv)

    if argc < 3:
        error("""usage: %(prog)s simu_dir simulation_duration
        Example: %(prog)s ~/git/ibgp2/results/article/rr/ 75
        """ % {"prog" : sys.argv[0]})
        return 1

    # Check output dir
    simu_dir = os.path.abspath(sys.argv[1])
    simulation_duration_sec = int(sys.argv[2])

    stats_xyz = stats_xyz_t()

    stats_xyz.set_gnuplot(KEY_CPU, {
        "title"  : "CPU usage",
        "xlabel" : "#BGP updates received",
        "ylabel" : "CDF(%routers)",
        "ymin"   : 0,
        "ymax"   : "",
        "zlabel" : "#Concurrent eBGP routes"
    })

    stats_xyz.set_gnuplot(KEY_MEM, {
        "title"  : "Memory usage (RIB entries only)",
        "xlabel" : "RAM used (in kb)",
        "ylabel" : "CDF(%routers)",
        "ymin"   : 0,
        "ymax"   : "",
        "zlabel" : "#Concurrent eBGP routes"
    })

    stats_xyz.set_gnuplot(KEY_RAM, {
        "title"  : "Memory usage",
        "xlabel" : "RAM used (in kb)",
        "ylabel" : "CDF(%routers)",
        "ymin"   : 0,
        "ymax"   : "",
        "zlabel" : "#Concurrent eBGP routes"
    })

    stats_xyz.set_gnuplot(KEY_OPTIMALITY, {
        "title"  : "Optimality",
        "xlabel" : "%(optimal routes)",
        "ylabel" : "CDF(%routers)",
        "ymin"   : 0,
        "ymax"   : "",
        "zlabel" : "#Concurrent eBGP routes"
    })

    stats_xyz.set_gnuplot(KEY_CONVERGENCE, {
        "title"  : "iBGP convergence",
        "xlabel" : "Convergence time (in s)",
        "ylabel" : "CDF(%routers)",
        "ymin"   : 0,
        "ymax"   : "",
        "zlabel" : "#Concurrent eBGP routes"
    })

    stats_xyz.set_gnuplot(KEY_DIVERSITY, {
        "title"  : "BGP route diversity",
        "xlabel" : "#Routes",
        "ylabel" : "CDF(%routers)",
        "ymin"   : 0,
        "ymax"   : "",
        "zlabel" : "#Concurrent eBGP routes"
    })

    output_dir = os.path.join(simu_dir, "gnuplot")
    mkdir(output_dir)
    check_writable_directory(output_dir)

    filename_opt = os.path.join(output_dir, "optimality_mean.dat")

    with open(filename_opt, "w") as ofs_opt:
        print("#|S|\t%opt\t%subopt\t%inv", file = ofs_opt)
        for run_subdir in os.listdir(simu_dir):
            # Each run correspond to a set of concurrent prefixes.
            # Each of them should be learnt by the same number of ASBRs.
            run_dir = os.path.join(simu_dir, run_subdir)

            # Skip the directory where we are writting files
            if os.path.isdir(run_dir) and run_dir != output_dir:
                info("Processing %s" % run_dir)
                bgp_simulation_duration_sec = timedelta(seconds = simulation_duration_sec - 5)
                process_run(run_dir, bgp_simulation_duration_sec, stats_xyz, ofs_opt)

    stats_xyz.write_gnuplot_files(output_dir)
    stats_xyz.write_gnuplot_mean_files(output_dir)

    return 0

if __name__ == '__main__':
    sys.exit(main())

