#!/bin/sh
# $Id: upgrade_12_20_common.sh,v 1.1.2.1 2009-02-23 17:19:15 cbbrowne Exp $

# We begin by setting DB configuration defaults/overrides
CLUSTER=${CLUSTER1:-"slony_regress1"}
NUMNODES=${NUMNODES:-"2"}
ACCEPTABLELAG="'935 seconds'::interval"

PGBINDIR=${PGBINDIR:-"/usr/local/pgsql/bin"}
SLTOOLDIR=${SLTOOLDIR:-".."}

DB1=${DB1:-"slonyregress1"}
HOST1=${HOST1:-"localhost"}
USER1=${USER1:-${PGUSER:-"postgres"}}
PORT1=${PORT1:-${PGPORT:-"5432"}}

DB2=${DB2:-"slonyregress2"}
HOST2=${HOST2:-"localhost"}
USER2=${USER2:-${PGUSER:-"postgres"}}
PORT2=${PORT2:-${PGPORT:-"5432"}}

DB3=${DB3:-"slonyregress3"}
HOST3=${HOST3:-"localhost"}
USER3=${USER3:-${PGUSER:-"postgres"}}
PORT3=${PORT3:-${PGPORT:-"5432"}}

DB4=${DB4:-"slonyregress4"}
HOST4=${HOST4:-"localhost"}
USER4=${USER4:-${PGUSER:-"postgres"}}
PORT4=${PORT4:-${PGPORT:-"5432"}}

# Feel free to add additional entries for each database that is to be
# part of the upgrade

# Note that environment variables may be used as alternatives to
# updates to this script; set values for DB1, DB2, ..., HOST1, HOST2,
# ..., USER1, USER2, ..., and so forth

# Now, derived values to be used later...
PSQL=${PGBINDIR}/psql
CLSCHEMA="\"_${CLUSTER}\""

err()
{
    exitval=$1
    shift
    echo 1>&2 "$0: ERROR: $*"
    numerrors=`expr ${numerrors} + 1`

    exit $exitval
}
