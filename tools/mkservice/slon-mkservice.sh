#!/bin/sh
#
# 
#
# contributed by Andrew Hammond <andrew.george.hammond@gmail.com>
#
# Create a slon service directory for use with svscan from deamontools.
# This uses multilog in a pretty basic way, which seems to be standard 
# for daemontools / multilog setups. If you want clever logging, see
# logrep below. Currently this script has very limited error handling
# capabilities.
# 
# For non-interactive use, set the following environment variables.
# BASEDIR LOGBASE SYSUSR PASSFILE DBUSER HOST PORT DATABASE CLUSTER SLON_BINARY
# If any of the above are not set, the script asks for configuration
# information interactively. The following environment variables are optional.
# LOGMAX LOGNUM
# If they are not set, they will silently default to reasonable values.
# 
# BASEDIR where you want the service directory structure for the slon
# to be created. This should _not_ be the /var/service directory.
# (default /usr/local/etc)
# LOGBASE where you want your logs to end up. (default /var/log)
# if set to - then revert to old behaviour and put logs under log/main.
# SYSUSR the unix user under which the slon (and multilog) process should run.
# PASSFILE location of the .pgpass file to be used. (default ~sysusr/.pgpass)
# DBUSER the postgres user the slon should connect as (default slony)
# HOST what database server to connect to (default localhost)
# PORT what port to connect to (default 5432)
# DATABASE which database to connect to (default dbuser)
# CLUSTER name of your Slony1 cluster? (default database)
# SLON_BINARY full path name of the slon binary (default `which slon`)
# LOGMAX maximum size (in bytes) of logfiles (default 10485760 which is 10MB)
# LOGNUM number of files to maintain (default 99, assume other tool prunes)

DEFAULT_SLON_BINARY=`which slon`                # silly, wild-ass guess
DEFAULT_BASEDIR='/usr/local/etc'
DEFAULT_LOGBASE='/var/log'
DEFAULT_SYSUSR='pgsql'                          # FreeBSD-centric. Oh well.
DEFAULT_DBUSR='slony'                           # Best Practice...
DEFAULT_SLON_DEBUG_LEVEL=2                      # default to "debug2" level
DEFAULT_HOST='localhost'                        # maybe the unix socket would be better?
DEFAULT_PORT=5432

if [ -z "$BASEDIR" ]; then
    echo -n "Where do you want the service dir created? Don't create this in 
/service or /var/service. Once it's created, either symlink or move
it to the service directory (since linking is an atomic filesystem action). 
Note that log files will not be stored here (that's the next question), so 
this doesn't have to be on a high storage / IO capacity filesystem.
[$DEFAULT_BASEDIR]: "
    read BASEDIR
    if [ -z "$BASEDIR" ]; then
        BASEDIR="$DEFAULT_BASEDIR"
    fi
fi
echo "BASEDIR=$BASEDIR"

if [ -z "$LOGBASE" ]; then
    echo -n "Where should the logfiles live? You probably want to put this
somewhere with plenty of storage and some IO capacity. Note that this
creates a subdirectory where the actual log files are stored.
Use - to disable this (putting the log files under log/main according to
daemontools convention).
[$DEFAULT_LOGDIR]: "
    read LOGDIR
    if [ -z "$LOGDIR" ]; then
        LOGDIR="$DEFAULT_LOGDIR"
    fi
fi

if [ -z "$SYSUSR" ]; then
    echo -n "System user name for slon to run under [$DEFAULT_SYSUSR]: "
    read SYSUSR
    if [ -z "$SYSUSR" ]; then
        SYSUSR="$DEFAULT_SYSUSR"
    fi
fi
echo "SYSUSR=$SYSUSR"

if [ -z "$PASSFILE" ]; then
    DEFAULT_PASSFILE=`eval echo "~$SYSUSR/.pgpass"`
    echo -n "And $SYSUSR's .pgpass file? [$DEFAULT_PASSFILE]: "
    read PASSFILE
    if [ -z "$PASSFILE" ]; then
        PASSFILE="$DEFAULT_PASSFILE"
    fi
fi
echo "PASSFILE=$PASSFILE"

if [ -z "$DBUSER" ]; then
    echo -n "Database user for slon to connect as [$DEFAULT_DBUSR]: "
    read DBUSER
    if [ -z "$DBUSER" ]; then
        DBUSER="$DEFAULT_DBUSR"
    fi
fi
echo "DBUSER=$DBUSER"

if [ -z "$HOST" ]; then
    echo -n "Host to connect to [$DEFAULT_HOST]: "
    read HOST
    if [ -z "$HOST" ]; then
        HOST="$DEFAULT_HOST"
    fi
fi
echo "HOST=$HOST"
if echo "$HOST" | grep / > /dev/null; then
    echo "Using / in your host name will make things break. Aborting."
    exit -1
fi

if [ -z "$PORT" ]; then
    echo -n "Port [$DEFAULT_PORT]: "
    read PORT
    if [ -z "$PORT" ]; then
        PORT="$DEFAULT_PORT"
    fi
fi
echo "PORT=$PORT"

if [ -z "$DATABASE" ]; then
    echo -n "Database [$DBUSER]: "
    read DATABASE
    if [ -z "$DATABASE" ]; then
        DATABASE="$DBUSER"
    fi
fi
echo "DATABASE=$DATABASE"
if echo "$DATABASE" | grep / > /dev/null; then
    echo "Using / in your database name will make things break. Aborting."
    exit -1
fi

if [ -z "$CLUSTER" ]; then
    echo -n "Cluster name: [$DATABASE]: "
    read CLUSTER
    if [ -z "$CLUSTER" ]; then
        CLUSTER="$DATABASE"
    fi
fi
echo "CLUSTER=$CLUSTER"
if echo "$CLUSTER" | grep / > /dev/null; then
    echo "Using / in your cluster name will make things break. Aborting."
    exit -1
fi

if [ -z "$SLON_BINARY" ]; then
    echo -n "Where is the slon binary? [$DEFAULT_SLON_BINARY]: "
    read SLON_BINARY
    if [ -z "$SLON_BINARY" ]; then
        SLON_BINARY=$DEFAULT_SLON_BINARY
    fi
fi
echo "SLON_BINARY=$SLON_BINARY"

SVCNAME="slon_${CLUSTER}_${HOST}_${PORT}_$DATABASE"
DIR="$BASEDIR/$SVCNAME"
LOGDIR="$DIR/log/main"
if [ '-' != "$LOGBASE" ]; then      # - means don't use a different logdir
    LOGDIR="$LOGBASE/$SVCNAME"      # otherwise we're logging somewhere else
fi
CONFIGFILE="$DIR/slon.conf"
echo "CONFIGFILE=$CONFIGFILE"

echo "Service dir will be created under $DIR"
echo "Logs will live under $LOGDIR"

mkdir -p "$DIR/env" "$DIR/supervise" "$DIR/log/env" "$DIR/log/supervise" "$LOGDIR" || exit -1
if [ '-' != "$LOGBASE" ]; then          # - means it's not a linked logdir
    ln -s "$LOGDIR" "$DIR/log/main"
fi
# Make sure the log file initially exists. This allows others to tail -F it
# before it starts getting populated. go go logrep!
touch "$DIR/log/main/current" || exit -1

# Set up the slon.conf file
cat > "$CONFIGFILE" <<EOF
# $CONFIGFILE
##############################################################################
# Connection settings

# Set the cluster name that this instance of slon is running against.
# The default is to read it off the command line.
cluster_name "$CLUSTER"

# Set slon's connection info; default is to read it off the command line.
conn_info "host=$HOST port=$PORT dbname=$DATABASE user=$DBUSER"

# Execute this SQL on each node at slon connect time. Useful to set logging
# levels, or to tune the planner/memory settings. You can specify multiple
# statements by separating them with a ; 
#sql_on_connection 

##############################################################################
# Logging

# If you want to use syslog then redir output from the slon to /dev/null and
# remove the log dir.

# Sets up logging to syslog. If this parameter is 1, messages go both to
# syslog and the standard output. A value of 2 sends output only to syslog
# (some messages will still go to the standard output/error). 
# Default is 0, which means syslog is off.
#syslog 0

# Sets the syslog "facility" to be used when syslog enabled. Valid values
# are LOCAL0, LOCAL1, LOCAL2, LOCAL3, LOCAL4, LOCAL5, LOCAL6, LOCAL7. 
# Default is LOCAL0.
#syslog_facility LOCAL0

# Sets the program name used to identify slon messages in syslog.
# The default is slon.
#syslog_ident slon

# Debug log level (higher value ==> more output).
# Range: [0,4], default 4
log_level $DEFAULT_SLON_DEBUG_LEVEL     # 3 and up is generally too high for production

# Determins, if you would like the pid of the (parent) slon process to appear
# in each log line entry.
# Default 0
log_pid 1           # good to know

# Determines if you would like the timestamp of the event being logged to
# appear in each log line entry.
log_timestamp  0    # multilog will insert a tai64n timestamp

# A strftime()-conformant format string for use if log_timestamp is enabled.
# The default is "%Y-%m-%d %H:%M:%S %Z"
#log_timestamp_format = "%Y-%m-%d %H:%M:%S %Z"

# Location and filename you would like for a file containing the Process ID
# of the slon process.
# The default is not defined in which case no file is written.
#pid_file       # daemontools renders this unnecessary, even undesireable
                # use instead the svc interface to signal the slon.

##############################################################################
# Archive Logging

# This indicates in what directory sync archive files should be stored.
#archive_dir

# This indicates a Unix command to be submitted each time an archive log
# is successfully generated.
#command_on_logarchive

##############################################################################
# Event Tuning

# Check for updates at least this often in milliseconds.
# Range: [10-60000], default 100
#sync_interval 100 

# Maximum amount of time in milliseconds before issuing a SYNC event, 
# This prevents a possible race condition in which the action sequence is
# bumped by the trigger while inserting the log row, which makes this bump
# is immediately visible to the sync thread, but the resulting log rows are
# not visible yet. If the SYNC is picked up by the subscriber, processed
# and finished before the transaction commits, this transaction's changes
# will not be replicated until the next SYNC. But if all application
# activity suddenly stops, there will be no more sequence bumps, so the
# high frequent -s check won't detect that. Thus, the need for
# sync_interval_timeout.
# Range: [0-120000], default 1000
#sync_interval_timeout 1000 

# Maximum number of SYNC events to group together when/if a subscriber falls
# behind. SYNCs are batched only if there are that many available and if they
# are contiguous. Every other event type in between leads to a smaller batch.
# And if there is only one SYNC available, even -g60 will apply just that one.
# As soon as a subscriber catches up, it will apply every single SYNC by itself.
# Range: [0,10000], default: 6
#sync_group_maxsize 6

# Sets how many cleanup cycles to run before a vacuum is done. 0 disables the
# builtin vacuum, intended to be used with the pg_autovacuum daemon.
# Range: [0,100], default: 3
#vac_frequency 3

# Maximum time planned for grouped SYNCs. If replication is behind, slon will
# try to increase numbers of syncs done targeting that they should take this
# quantity of time to process. If the value is set to 0, this logic will be
# ignored. 
# Range [10000,600000] ms, default 60000.
#desired_sync_time 60000 

# This must be used in conjunction with quit_sync_finalsync, and indicates
# which provider node's worker thread should be watched to see if the slon
# should terminate due to reaching some desired "final" event number.
#quit_sync_provider 0

# Final event number to process. This must be used in conjunction with
# quit_sync_finalsync, and allows the slon to terminate itself once it
# reaches a certain event for the specified provider. If the value is set
# to 0, this logic will be ignored. 
#quit_sync_finalsync 0

# Indicates an interval by which this node should lag its providers. If set,
# this is used in the event processing loop to modify what events are to be
# considered for queueing; those events newer than
# now() - lag_interval::interval are left out, to be processed later.
# If the value is left empty, this logic will be ignored. 
#lag_interval

# Size above which an sl_log_? row's log_cmddata is considered large. Up to
# 500 rows of this size are allowed in memory at once. Rows larger than that
# count into the sync_max_largemem space allocated and free()'ed on demand.
# The default value is 8192, meaning that your expected memory consumption
# (for the LOG cursor) should not exceed 8MB.
#sync_max_rowsize 8192

# Maximum memory allocated for large rows, where log_cmddata are larger than
# sync_max_rowsize. Note that the algorithm reads rows until after this value
# is exceeded. Otherwise, a tuple larger than this value would stall
# replication. As a result, don't assume that memory consumption will remain
# smaller than this value.  The default value is 5242880.
#sync_max_largemem 5242880

# How long should the remote listener wait before treating the event selection
# criteria as having timed out?
# Range: [30-30000], default 300 
#remote_listen_timeout 300

EOF

# Set up the envdir contents for the admins. Generously.
echo "$SLON_BINARY"                 > $DIR/env/SLON_BINARY
echo "$CONFIGFILE"                  > $DIR/env/CONFIGFILE
echo "$CLUSTER"                     > $DIR/env/CLUSTER
echo "$HOST"                        > $DIR/env/PGHOST
echo "$PORT"                        > $DIR/env/PGPORT
echo "$DATABASE"                    > $DIR/env/PGDATABASE
# The absence of PGPASSWORD is not an oversight. Use .pgpass, see
# http://www.postgresql.org/docs/current/interactive/libpq-pgpass.html
# Configure the location of .pgpass file here...
# I'd like a better solution than this for expanding the homedir.
echo "$PASSFILE"                    > $DIR/env/PGPASSFILE 
echo "$DBUSER"                      > $DIR/env/PGUSER
# Change these if you're stupid or unfortunate enough to have to.
echo 'ISO'                          > $DIR/env/PGDATESTYLE
echo 'UTC'                          > $DIR/env/PGTZ

# Avoid some subtle errors by documenting stuff... such as
cat > "$DIR/README.txt" <<EOF
This service will start on boot. If you do not want it to, then
touch $DIR/down

To upgrade your slon, first update env/SLON_BINARY to the full
path and name of the new slon binary. Then stop the slon.
svc -d $DIR
Apply your slonik UPDATE FUNCTIONS script(s) then restart your slon.
svc -u $DIR
Finally, check your logs to ensure that the new slon has started and
is running happily.

If you need to have a special purpose config file, or test version,
then you can simply copy the existing slon.conf to some other name,
make your changes there, update env/CONFIGFILE to point at the new
config and restart the slon.
svc -k $DIR

Note that changing variables such as CLUSTER, PGHOST, PGPORT,
PGDATABASE and PGUSER in the env directory will not change where
the slon connects. They are only there for admin/DBA convenience.
exec envdir $DIR/env bash
Is a quick way to get your variables all set up.

If you want to change where the slon connects, you need to edit
$CONFIGFILE
But you probably should not be doing that anyway, because then you
have to rename a whole bunch of stuff and edit all over the place
to keep the naming scheme consistent. Yuck. You should probably
just create a new slon service directory with the correct information,
and shut this one down.
touch $DIR/down; svc -dx $DIR $DIR/log

EOF

cat > "$DIR/env/README.txt" <<EOF
Many of these environment variables are only set as a convenience
for administrators and DBAs. To load them, try
exec envdir $DIR/env bash
Before you change stuff here, please read ../README.txt
EOF

# create the run script for the slon
cat > "$DIR/run" <<EOF
#!/bin/sh
# Note that the slon binary is a variable, so you can edit the value in
# env/SLON_BINARY and restart to upgrade slons. See README.txt in this dir.
exec 2>&1
exec envdir ./env sh -c 'exec setuidgid ${SYSUSR} "\${SLON_BINARY}" -f "\${CONFIGFILE}"'
EOF
chmod a+x "$DIR/run"
echo "$DIR/run created"

# setup an envdir for multilog
echo ${LOGMAX-"10485760"}           > $DIR/log/env/LOGMAX
echo ${LOGNUM-"99"}                 > $DIR/log/env/LOGNUM

cat > "$DIR/log/README.txt" <<EOF
To force a log rotation, use
svc -a $DIR/log

The size (in bytes) of the log files (before they get rotated) is controlled
by the s parameter for multilog. This is set up as an envdir variable at
$DIR/log/env/LOGMAX
You might want to increase or decrease this. It goes up to a maximum of
16777215 (15MB) and defaults to 99999 (97kB) if unset. Leaving it unset 
will break this script. It defaults to 10485760 (which is 10MB).
You need to restart multilog for changes to this to take effect.
svc -k $DIR/log

The n paramter decides how many old log files to keep around. This is set
up as an envdir variable at
$DIR/log/env/LOGNUM
You will probably want to decrease this if you are not using some other
tool to manage old logfiles. Multilog defaults to 10 if this is unset, but
like the size above, it will break this script if left unset. The script
defaults to 99 under the assumption that you are using some other, system
wide tool (like cfengine) to prune your logs.
EOF

# create the run file for the multilog
cat > "$DIR/log/run" <<EOF
#!/bin/sh
# This puts everything in the main log. Unfortunately multilog only allows
# you to select which log you want to write to as opposed to writing each
# line to every log which matches the criteria. Split up logs would make
# debugging harder. See also README.txt in this directory.

exec envdir ./env sh -c 'exec setuidgid $SYSUSR multilog t s"\$LOGMAX" n"\$LOGNUM" ./main'
EOF
chmod a+x "$DIR/log/run"
echo "$DIR/log/run created"

# create and fix ownerships and permissions for .pgpass appropriately
touch "$PASSFILE"
chown "$SYSUSR" "$PASSFILE"
chmod 600 "$PASSFILE"
if [ ! -s "$PASSFILE" ]; then
    echo "Populating $PASSFILE with header and example."
    cat > "$PASSFILE" <<EOF
#hostname:port:database:username:password
#$HOST:$PORT:*:$DBUSER:secret
EOF
fi

# fix permissions and ownership
chown -R "$SYSUSR" "$DIR"
chmod a+rX "$DIR"

cat <<EOF
When you're ready to start the slon, simply link the newly created dir, 
$DIR
under your current supervisor's directory. On FreeBSD this is /var/service
by default. So the command would be:

sudo ln -s $DIR /var/service

The slon should be started within 5 seconds.

You can further configure the slon by modifying stuff in the 
$DIR/env
directory. Specifically, make sure that the SLON variable points to the right
slon binary. You may also want to edit
$CONFIGFILE

Finally, you need to make sure that $PASSFILE 
is correctly in place. If you didn't have one before (or it was empty), it
has been created and populated with some sample data.

Logfiles can be found at $LOGDIR
You may also want to set up a logrep to filter out the more intresting
log lines. See logrep-mkservice.sh.
EOF
