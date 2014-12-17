# 
# This file contains the various sets of possible DB
# configuration defaults/overrides

CLUSTER1=${CLUSTER1:-"Slony_Regress1"}
CLUSTER2=${CLUSTER2:-"slony_regress2"}
CLUSTER3=${CLUSTER3:-"slony_regress3"}

# Note that there is only ONE encoding
ENCODING=${ENCODING:-"UNICODE"}

DB1=${DB1:-"slonyregress1"}
HOST1=${HOST1:-"localhost"}
USER1=${USER1:-${PGUSER:-"postgres"}}
WEAKUSER1=${WEAKUSER1:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT1=${PORT1:-${PGPORT:-"5432"}}
PGBINDIR1=${PGBINDIR1:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB2=${DB2:-"slonyregress2"}
HOST2=${HOST2:-"localhost"}
USER2=${USER2:-${PGUSER:-"postgres"}}
WEAKUSER2=${WEAKUSER2:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT2=${PORT2:-${PGPORT:-"5432"}}
PGBINDIR2=${PGBINDIR2:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB3=${DB3:-"slonyregress3"}
HOST3=${HOST3:-"localhost"}
USER3=${USER3:-${PGUSER:-"postgres"}}
WEAKUSER3=${WEAKUSER3:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT3=${PORT3:-${PGPORT:-"5432"}}
PGBINDIR3=${PGBINDIR3:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB4=${DB4:-"slonyregress4"}
HOST4=${HOST4:-"localhost"}
USER4=${USER4:-${PGUSER:-"postgres"}}
WEAKUSER4=${WEAKUSER4:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT4=${PORT4:-${PGPORT:-"5432"}}
PGBINDIR4=${PGBINDIR4:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB5=${DB5:-"slonyregress5"}
HOST5=${HOST5:-"localhost"}
USER5=${USER5:-${PGUSER:-"postgres"}}
WEAKUSER5=${WEAKUSER5:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT5=${PORT5:-${PGPORT:-"5432"}}
PGBINDIR5=${PGBINDIR5:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB6=${DB6:-"slonyregress6"}
HOST6=${HOST6:-"localhost"}
USER6=${USER6:-${PGUSER:-"postgres"}}
WEAKUSER6=${WEAKUSER6:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT6=${PORT6:-${PGPORT:-"5432"}}
PGBINDIR6=${PGBINDIR6:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB7=${DB7:-"slonyregress7"}
HOST7=${HOST7:-"localhost"}
USER7=${USER7:-${PGUSER:-"postgres"}}
WEAKUSER7=${WEAKUSER7:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT7=${PORT7:-${PGPORT:-"5432"}}
PGBINDIR7=${PGBINDIR7:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB8=${DB8:-"slonyregress8"}
HOST8=${HOST8:-"localhost"}
USER8=${USER8:-${PGUSER:-"postgres"}}
WEAKUSER8=${WEAKUSER8:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT8=${PORT8:-${PGPORT:-"5432"}}
PGBINDIR8=${PGBINDIR8:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB9=${DB9:-"slonyregress9"}
HOST9=${HOST9:-"localhost"}
USER9=${USER9:-${PGUSER:-"postgres"}}
WEAKUSER9=${WEAKUSER9:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT9=${PORT9:-${PGPORT:-"5432"}}
PGBINDIR9=${PGBINDIR9:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB10=${DB10:-"slonyregress10"}
HOST10=${HOST10:-"localhost"}
USER10=${USER10:-${PGUSER:-"postgres"}}
WEAKUSER10=${WEAKUSER01:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT10=${PORT10:-${PGPORT:-"5432"}}
PGBINDIR10=${PGBINDIR10:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB11=${DB11:-"slonyregress11"}
HOST11=${HOST11:-"localhost"}
USER11=${USER11:-${PGUSER:-"postgres"}}
WEAKUSER11=${WEAKUSER11:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT11=${PORT11:-${PGPORT:-"5432"}}
PGBINDIR11=${PGBINDIR11:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB12=${DB12:-"slonyregress12"}
HOST12=${HOST12:-"localhost"}
USER12=${USER12:-${PGUSER:-"postgres"}}
WEAKUSER12=${WEAKUSER12:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT12=${PORT12:-${PGPORT:-"5432"}}
PGBINDIR12=${PGBINDIR12:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

DB13=${DB13:-"slonyregress13"}
HOST13=${HOST13:-"localhost"}
USER13=${USER13:-${PGUSER:-"postgres"}}
WEAKUSER13=${WEAKUSER13:-${WEAKUSER:-${PGUSER:-"weakuser"}}}
PORT13=${PORT13:-${PGPORT:-"5432"}}
PGBINDIR13=${PGBINDIR13:-${PGBINDIR:-"/usr/local/pgsql/bin"}}

# Where to look for tools (e.g. - slony1_dump.sh)
SLTOOLDIR=${SLTOOLDIR:-"../tools"}

# Email address of the tester
SLONYTESTER=${SLONYTESTER:-"j.random.luser@example.net"}

# File in which to stow SQL queries that summarize test results
SLONYTESTFILE=${SLONYTESTFILE:-"/tmp/Slony-I-test-results.log"}
