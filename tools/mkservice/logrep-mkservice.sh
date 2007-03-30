#!/bin/sh
#
# $Id: logrep-mkservice.sh,v 1.1 2007-03-30 14:49:46 cbbrowne Exp $
#
# contributed by Andrew Hammond <andrew.george.hammond@gmail.com>
#
# logrep: use tail -F to pull from another log file for filtering to create
# special purpose log files. Some example filters follow.
#
# For non-interactive use, set the following environment variables.
# BASEDIR SYSUSR SOURCE EXTENSION CRITERIA 
#
# Slony subscribe specific information
# '-*' '+* * ERROR*' '+* * WARN*' '+* * CONFIG enableSubscription*' '+* * DEBUG2 remoteWorkerThread_* prepare to copy table*' '+* * DEBUG2 remoteWorkerThread_* all tables for set * found on subscriber*' '+* * DEBUG2 remoteWorkerThread_* copy*' '+* * DEBUG2 remoteWorkerThread_* Begin COPY of table*' '+* * DEBUG2 remoteWorkerThread_* * bytes copied for table*' '+* * DEBUG2 remoteWorkerThread_* * seconds to*' '+* * DEBUG2 remoteWorkerThread_* set last_value of sequence*' '+* * DEBUG2 remoteWorkerThread_* copy_set*'
#
# Errors to trigger nagios
# '-*' '+* * ERROR*'

DEFAULT_BASEDIR='/usr/local/etc'
DEFAULT_SYSUSR='pgsql'                          # FreeBSD-centric. Oh well.
DEFAULT_SOURCE='slon_123'
DEFAULT_EXTENSION='_NODEBUG'
DEFAULT_CRITERIA="'-* [*] DEBUG*'"

if [ -z "$BASEDIR" ]; then
    echo -n "Where do you want the service dir created? Don't want to created this in /service or /var/service. Once it's created, either link or move it to the service directory (since linking is an atomic filesystem action). If your service directory is on a small, relatively static partition, you will almost certainly want to put this on a partition that can handle some log files and then link.
[$DEFAULT_BASEDIR]: "
    read BASEDIR
    if [ -z "$BASEDIR" ]; then
        BASEDIR="$DEFAULT_BASEDIR"
    fi
fi
echo "BASEDIR=$BASEDIR"

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

DIR="$BASEDIR/logrep_$SOURCE$EXTENSION"
echo "Service dir will be created under $DIR"

mkdir -p "$DIR/supervise" "$DIR/log/supervise" "$DIR/log/main" || exit -1
# Make sure the log file initially exists. This allows others to tail -F it
# before it starts getting populated.
touch "$DIR/log/main/current" || exit -1

# create the run script
cat > "$DIR/run" <<EOF
#!/bin/sh
exec 2>&1
exec setuidgid $SYSUSR tail -F "$BASEDIR/$SOURCE/log/main/current"
EOF
chmod a+x "$DIR/run"
echo "$DIR/run created"

# create the run file for the multilog
cat > "$DIR/log/run" <<EOF
#!/bin/sh
# DO NOT add another timestamp using the t parameter to multilog. Unless
# of course you like being confused.
# Note that size (in bytes) of the log files (before they get rotated) is 
# controlled by the s parameter. You might want to increase this. It goes
# up to a maximum of 16777215 (15MB) and defaults to 99999 (97kB).
# I'm using 10485760 (10MB)
# The n paramter decides how many old log files to keep around. Defaults
# to 10. 
exec setuidgid $SYSUSR multilog s10485760 n99 $CRITERIA ./main
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
