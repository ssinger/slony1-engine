#!/bin/sh
# 
# Cluster starter

# This script should be run periodically to search for slon
# "node[n].conf" configuration files, and make sure corresponding slon
# daemons are running.

# Environment variables needed:

# PATH=/opt/dbs/pgsql74/bin:$PATH
#
#   The PATH must include, preferably at the beginning, the path to
#   the slon binaries that are to be run
#
#SLHOME=/var/spool/slony1
#   This indicates the "home" directory for slon configuration files
#
#LOGHOME=/var/log/slony1
#   This indicates the "home" directory for slon log files
#
#   This indicates a list of clusters that are to be managed
#
# There are further expectations of directory structures under $SLHOME
# and $LOGHOME, as follows:
#
# Under $SLHOME, there is a structure of "conf" files in the directory
#   $SLHOME/$cluster/conf
# with names like node1.conf, node2.conf, and so forth
#
# Under $SLHOME, there is also a directory for storing PID files thus
# $SLHOME/$cluster/pid/node1.pid, $SLHOME/$cluster/pid/node2.pid, and
# so forth
#
# Under $LOGHOME, logs will be appended, for each cluster, and each
# node, into $LOGHOME/$cluster/node$nodenum/$cluster-node$nodenum.log
#CLUSTERS="oxrsorg oxrslive oxrsaero oxrsin oxrsflex2 oxrsmobi"

WATCHLOG="$LOGHOME/watcher.log"

# This function provides a shorthand to log messages to
log_action () {
    LOGVALUE=$1
    NOW=`date`
    echo "$NOW $LOGVALUE" >> $WATCHLOG
}
 
invoke_slon () {
    LOGHOME=$1
    NODENUM=$2
    CLUSTER=$3
    SLONCONF=$4
    log_action "run slon - slon -f $SLONCONF >> $LOGHOME/node${NODENUM}/${CLUSTER}-node${NODENUM}.log"
    mkdir -p $LOGHOME/node${NODENUM}
    slon -f $SLONCONF >> $LOGHOME/node${NODENUM}/${CLUSTER}-node${NODENUM}.log &
}

start_slon_if_needed () {
    CONFIGPATH=$1
    NODENUM=$2
    LOGHOME=$3

    if [ -e $CONFIGPATH/conf/node${NODENUM}.conf ]; then
	SLONCONF="$CONFIGPATH/conf/node${NODENUM}.conf"
	SLONPIDFILE=`grep "^ *pid_file=" $SLONCONF | cut -d "=" -f 2 | cut -d "#" -f 1 | cut -d " " -f 1 | cut -d "'" -f 2`
	CLUSTER=`grep "^ *cluster_name=" $SLONCONF | cut -d "=" -f 2 | cut -d "'" -f 2`
    else
	log_action "Could not find node configuration in $CONFIGPATH/conf/node${NODENUM}.conf"
	return
    fi

    # Determine what the format name should be in the ps command
    case `uname` in
	SunOS) 
              PSCOMM="comm"
	      BPS="/usr/ucb/ps"
	      ;;
	Darwin) 
	      PSCOMM="command"
	      BPS="/bin/ps"
	      ;;
	*) 
	      PSCOMM="command" 
	      BPS="ps"
	      ;;
    esac
    if [ -e $SLONPIDFILE ] ; then
	SLONPID=`cat $SLONPIDFILE`

	FINDIT=`ps -p ${SLONPID} -o ${PSCOMM}= | grep slon`
	if [ -z $FINDIT ]; then
        # Need to restart slon
	    log_action "slon died for config $CONFIGPATH/conf/node${NODENUM}.conf"
	    invoke_slon $LOGHOME $NODENUM $CLUSTER $SLONCONF
	else
	    echo "Slon already running - $SLONPID"
	fi
    else
        ${BPS} auxww | egrep "[s]lon -f $CONFIGPATH/conf/node${NODENUM}.conf" > /dev/null
	if [ $? -eq 0 ] ; then 	
            echo "Slon already running - but PID marked dead"
	else
	    invoke_slon $LOGHOME $NODENUM $CLUSTER $SLONCONF
	fi
    fi
}

for cluster in `echo $CLUSTERS`; do
    for conffile in `find $SLHOME/$cluster/conf -name "node[0-9]*.conf"`; do
	# 1. Use sed to chop off all the path info
        # 2. First cut to chop off up to "node"
        # 3. Second cut to take off the ".conf" part
        nodenum=`echo $conffile | sed 's/.*\///g' | cut -d "e" -f 2 | cut -d "." -f 1`
	start_slon_if_needed $SLHOME/$cluster $nodenum $LOGHOME
    done
done
