#!/bin/sh
#
# 
#
# contributed by Andrew Hammond <andrew.george.hammond@gmail.com>
#
# This uses "tail -F" to pull data from log files allowing you to use
# multilog filters (by setting the CRITERIA) to create special purpose
# log files. The goal is to provide a way to monitor log files in near 
# realtime for "interesting" data without either hacking up the initial
# log file or wasting CPU/IO by re-scanning the same log repeatedly.
# 
# For non-interactive use, set the following environment variables.
# BASEDIR LOGBASE SYSUSR SOURCE EXTENSION CRITERIA 
# If any of the above are not set, the script asks for configuration
# information interactively. The following environment variables are optional.
# LOGMAX LOGNUM
# If they are not set, they will silently default to reasonable values.
# 
# BASEDIR where you want the service directory structure for the logrep
# to be created. This should _not_ be the /var/service directory.
# LOGBASE where you want your logs to end up. (default /var/log)
# if set to - then revert to old behaviour and put logs under log/main.
# SYSUSR unix user under which the service should run.
# SOURCE name of the service with the log you want to follow.
# EXTENSION a tag to differentiate this logrep from others using the same source.
# CRITERIA the multilog filter you want to use.
# LOGMAX maximum size (in bytes) of logfiles (default 10485760 which is 10MB)
# LOGNUM number of files to maintain (default 99, assume other tool prunes)
# 
# A trivial example of this would be to provide a log file of all slon
# ERROR messages which could be used to trigger a nagios alarm.
# EXTENSION='ERRORS'
# CRITERIA="'-*' '+* * ERROR*'"
# (Reset the monitor by rotating the log using svc -a $svc_dir)
# 
# A more interesting application is a subscription progress log.
# EXTENSION='COPY'
# CRITERIA="'-*' '+* * ERROR*' '+* * WARN*' '+* * CONFIG enableSubscription*' '+* * DEBUG2 remoteWorkerThread_* prepare to copy table*' '+* * DEBUG2 remoteWorkerThread_* all tables for set * found on subscriber*' '+* * DEBUG2 remoteWorkerThread_* copy*' '+* * DEBUG2 remoteWorkerThread_* Begin COPY of table*' '+* * DEBUG2 remoteWorkerThread_* * bytes copied for table*' '+* * DEBUG2 remoteWorkerThread_* * seconds to*' '+* * DEBUG2 remoteWorkerThread_* set last_value of sequence*' '+* * DEBUG2 remoteWorkerThread_* copy_set*'"
# 
# If you have a subscription log then it's easy to determine if a given
# slon is in the process of handling copies or other subscription activity.
# If the log isn't empty, and doesn't end with a 
# "CONFIG enableSubscription: sub_set:1"
# (or whatever set number you've subscribed) then the slon is currently in
# the middle of initial copies.
# If you happen to be monitoring the mtime of your primary slony logs to 
# determine if your slon has gone brain-dead, checking this is a good way
# to avoid mistakenly clobbering it in the middle of a subscribe. As a bonus,
# recall that since the the slons are running under svscan, you only need to
# kill it (via the svc interface) and let svscan start it up again laster.
# I've also found the COPY logs handy for following subscribe activity 
# interactively.

DEFAULT_BASEDIR='/usr/local/etc'
DEFAULT_LOGDIR='/var/log'
DEFAULT_SYSUSR='pgsql'                          # FreeBSD-centric. Oh well.
DEFAULT_SOURCE='slon_123'
DEFAULT_EXTENSION='_NODEBUG'
DEFAULT_CRITERIA="'-* [*] DEBUG*'"

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
    echo -n "System user name for followgrep to run under [$DEFAULT_SYSUSR]: "
    read SYSUSR
    if [ -z "$SYSUSR" ]; then
        SYSUSR="$DEFAULT_SYSUSR"
    fi
fi
echo "SYSUSR=$SYSUSR"

if [ -z "$SOURCE" ]; then
    echo -n "Service to follow [$DEFAULT_SOURCE]: "
    read SOURCE
    if [ -z "$SOURCE" ]; then
        DATABASE="$DEFAULT_SOURCE"
    fi
fi
echo "SOURCE=$SOURCE"

if [ -z "$EXTENSION" ]; then
    echo -n "Name extension [$DEFAULT_EXTENSION]: "
    read EXTENSION
    if [ -z "$EXTENSION" ]; then
        EXTENSION="$DEFAULT_EXTENSION"
    fi
fi
echo "EXTENSION=$EXTENSION"

if [ -z "$CRITERIA" ]; then
    echo -n "Criteria [$DEFAULT_CRITERIA]: "
    read CRITERIA
    if [ -z "$CRITERIA" ]; then
        CRITERIA="$DEFAULT_CRITERIA"
    fi
fi
echo "CRITERIA=$CRITERIA"


SVCNAME="logrep_$SOURCE$EXTENSION"
DIR="$BASEDIR/$SVCNAME"
LOGDIR="$DIR/log/main"
if [ '-' != "$LOGBASE" ]; then      # - means don't use a different logdir
    LOGDIR="$LOGBASE/$SVCNAME"      # otherwise we're logging somewhere else
fi

echo "Service dir will be created under $DIR"
echo "Logs will live under $LOGDIR"


mkdir -p "$DIR/env" "$DIR/supervise" "$DIR/log/env" "$DIR/log/supervise" "$LOGDIR" || exit -1
if [ '-' != "$LOGBASE" ]; then          # - means it's not a linked logdir
    ln -s "$LOGDIR" "$DIR/log/main"
fi
# Make sure the log file initially exists. This allows others to tail -F it
# before it starts getting populated. go go recursive logrep!
touch "$DIR/log/main/current" || exit -1

# create the run script
cat > "$DIR/run" <<EOF
#!/bin/sh
exec 2>&1
exec env ./env setuidgid $SYSUSR tail -F "$BASEDIR/$SOURCE/log/main/current"
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
# Since we're presumably drawing data from another logfile which already 
# has timestamps, adding another would simply sow confusion.

exec envdir ./env sh -c 'exec setuidgid $SYSUSR multilog s"\$LOGMAX" n"\$LOGNUM" ./main'
EOF
chmod a+x "$DIR/log/run"
echo "$DIR/log/run created"

# fix permissions and ownership
chown -R "$SYSUSR" "$DIR"
chmod a+rX "$DIR"

cat <<EOF
When you're ready to start logrep, simply link the newly created dir, 
$DIR
under your current supervisor's directory. On FreeBSD this is /var/service
by default. So the command would be:

sudo ln -s $DIR /var/service

The service should be started within 5 seconds.
EOF
