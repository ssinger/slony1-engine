NUMCLUSTERS=1
NUMNODES=3    # These are the "regular" Slony-I nodes
              # node #4 will also be populated as a log shipping node

ORIGINNODE=1  # at least initially - origin is later moved to node #3
WORKERS=1
ARCHIVE2=true   # Node #2 needs to run log archiving
