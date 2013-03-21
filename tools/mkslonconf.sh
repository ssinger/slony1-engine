#!/bin/sh
# 
# mkslonconf.sh
# Build a set of slon.conf files compatible with launch_clusters.sh

# Start with: 

# a) Environment set up with libpq-compatible parameters to connect to
#    a database
# b) SLONYCLUSTER set to the appropriate cluster name
# c) MKDESTINATION being a directory for conf/pid files to live in
# d) LOGHOME being a directory for log directories to live in
# SLONYCLUSTER=oxrsaero
# MKDESTINATION=/tmp
# LOGHOME=/tmp/logs

echo "building slon config files in ${MKDESTINATION}/${SLONYCLUSTER}"
mkdir -p ${MKDESTINATION}/${SLONYCLUSTER}
mkdir -p ${MKDESTINATION}/${SLONYCLUSTER}/conf
mkdir -p ${MKDESTINATION}/${SLONYCLUSTER}/pid
echo "Make sure ${MKDESTINATION}/${SLONYCLUSTER}/conf, ${MKDESTINATION}/${SLONYCLUSTER}/pid exist"

query="select pa_server, min(pa_conninfo) from \"_${SLONYCLUSTER}\".sl_path group by pa_server;"
#echo "Query: $query"
queryoutput="/tmp/slonconf.query.$$"
psql --no-align --command "$query" --field-separator "|" --quiet --tuples-only > $queryoutput
exec 3< $queryoutput

create_conf_file ()
{
   node=$1
   dsn=$2
   conffile="${MKDESTINATION}/${SLONYCLUSTER}/conf/node${node}.conf"
cat <<_EOF_ > $conffile
# Sets how many cleanup cycles to run before a vacuum is done.
# Range: [0,100], default: 3
#vac_frequency=3

# Debug log level (higher value ==> more output).  Range: [0,4], default 2
#log_level=2

# Check for updates at least this often in milliseconds.
# Range: [10-60000], default 100
#sync_interval=1000

# Maximum amount of time in milliseconds before issuing a SYNC event, 
# This prevents a possible race condition in which the action sequence 
# is bumped by the trigger while inserting the log row, which makes 
# this bump is immediately visible to the sync thread, but 
# the resulting log rows are not visible yet.  If the sync is picked 
# up by the subscriber, processed and finished before the transaction 
# commits, this transaction's changes will not be replicated until the 
# next SYNC.  But if all application activity suddenly stops, 
# there will be no more sequence bumps, so the high frequent -s check 
# won't detect that.  Thus, the need for sync_interval_timeout.
# Range: [0-120000], default 1000
#sync_interval_timeout=10000

# Maximum number of SYNC events to group together when/if a subscriber
# falls behind.  SYNCs are batched only if there are that many available 
# and if they are contiguous. Every other event type in between leads to 
# a smaller batch.  And if there is only one SYNC available, even -g60 
# will apply just that one. As soon as a subscriber catches up, it will 
# apply every single SYNC by itself.
# Range:  [0,100], default: 6
#sync_group_maxsize=6

# Size above which an sl_log_? row's log_cmddata is considered large.
# Up to 500 rows of this size are allowed in memory at once. Rows larger
# than that count into the sync_max_largemem space allocated and free'd
# on demand.
# Range:  [1024,32768], default: 8192
#sync_max_rowsize=8192

# Maximum amount of memory allowed for large rows. Note that the algorithm
# will stop fetching rows AFTER this amount is exceeded, not BEFORE. This
# is done to ensure that a single row exceeding this limit alone does not
# stall replication.
# Range:  [1048576,1073741824], default: 5242880
#sync_max_largemem=5242880

# If this parameter is 1, messages go both to syslog and the standard 
# output. A value of 2 sends output only to syslog (some messages will 
# still go to the standard output/error).  The default is 0, which means 
# syslog is off.  
# Range:  [0-2], default: 0
#syslog=0

# If true, include the process ID on each log line.  Default is false.
#log_pid=false

# If true, include the timestamp on each log line.  Default is true.
#log_timestamp=true

# A strftime()-conformant format string for use with log timestamps.
# Default is '%Y-%m-%d %H:%M:%S %Z'
#log_timestamp_format='%Y-%m-%d %H:%M:%S %Z'

# Where to write the pid file.  Default:  no pid file
pid_file='${MKDESTINATION}/${SLONYCLUSTER}/pid/node${node}.pid'

# Sets the syslog "facility" to be used when syslog enabled.  Valid 
# values are LOCAL0, LOCAL1, LOCAL2, LOCAL3, LOCAL4, LOCAL5, LOCAL6, LOCAL7.
#syslog_facility=LOCAL0

# Sets the program name used to identify slon messages in syslog.
#syslog_ident=slon

# Set the cluster name that this instance of slon is running against
# default is to read it off the command line
cluster_name='${SLONYCLUSTER}'

# Set slon's connection info, default is to read it off the command line
conn_info='${dsn}'

# maximum time planned for grouped SYNCs
# If replication is behind, slon will try to increase numbers of
# syncs done targeting that they should take this quantity of
# time to process. in ms
# Range [10000,600000], default 60000. 
desired_sync_time=60000

# Execute the following SQL on each node at slon connect time
# useful to set logging levels, or to tune the planner/memory
# settings.  You can specify multiple statements by seperating
# them with a ;
#sql_on_connection="SET log_min_duration_statement TO '1000';"

# Command to run upon committing a log archive.
# This command is passed one parameter, namely the full pathname of
# the archive file
#command_on_logarchive=""

# A PostgreSQL value compatible with ::interval which indicates how
# far behind this node should lag its providers.
# lag_interval=""

# Directory in which to stow sync archive files
# archive_dir=""

_EOF_
mkdir -p $LOGHOME/$SLONYCLUSTER/node${node}

echo " For node ${node}, created conf file $conffile"
echo " as well as log directory $LOGHOME/$SLONYCLUSTER/node${node}"
echo " -------------------------------------------------------------"

}

while read line <&3; do
    #echo "Line: $line"
    node=`echo ${line} | cut -d "|" -f 1`   
    dsn=`echo ${line} | cut -d "|" -f 2`

   #echo "node[$node]  dsn[$dsn]"
    conffile="${MKDESTINATION}/${SLONYCLUSTER}/conf/node${node}.conf"
    echo "Generating slon conf file $conffile"

    if [ -e $conffile ] ; then
	echo "config file $conffile already exists."
	echo "Do you want to (Overwrite) it or (Skip) it (Anything else aborts) [Overwrite|Skip]?"
	read nl
	case $nl in
	    (Overwrite)
	    echo "overwriting..."
	    create_conf_file $node "$dsn"
	    (Skip)
	    echo "skipping..."
	    (*)
	    echo "invalid input - [$nl] aborting..."
	    exit -1
	esac
    else
	echo "creating conf file for new node $node with DSN [$dsn]"
	create_conf_file $node "$dsn"
    fi
done
rm $queryoutput

cat <<EOF 
---------------------------------------------------------------------
Be sure to review .conf files created in
${MKDESTINATION}/${SLONYCLUSTER}/conf to ensure they are reasonable
before starting up slons against them.  Customize as needed.

Notably, if you have nodes which are reached via different DSNs from
different locations, then the conn_info value may not be correct.

In addition, this script will happily create .conf files for nodes
which it found missing.  If you wanted nodes to be controlled from
some other host, this could draw them in to the local host, which
would not be what you wanted.  In most cases, this would only cause
minor inconvenience; if you are running log shipping against a
particular remote subscriber, this could cause you some real
heartburn...
---------------------------------------------------------------------
EOF
