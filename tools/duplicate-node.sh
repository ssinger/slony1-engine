#!/bin/sh
# 

# Parameters:
#  - need cluster name
#  - need node # of the new node
#  - need conninfo for node being duplicated
#  - need conninfo for node being created
#  - need conninfo for origin node
#  - need to have Slony-I tools directory in PATH

help()
{
echo <<EOF
duplicate-node.sh: Parameters
Base parms:
  -c, --cluster: Cluster name
  -n, --nodenum: New node number

libpq parameters to specify DB connections:
  -ho, --host-origin: PGHOST for origin node
  -hd, --host-dup: PGHOST for node being duplicated
  -hn, --host-new: PGHOST for new node being created

  -do, --database-origin: PGDATABASE for origin node
  -dd, --database-dup: PGDATABASE for node being duplicated
  -dn, --database-new: PGDATABASE for new node being created

  -po, --port-origin: PGPORT for origin node
  -pd, --port-dup: PGPORT for node being duplicated
  -pn, --port-new: PGPORT for new node being created

  -uo, --user-origin: PGUSER for origin node
  -ud, --user-dup: PGUSER for node being duplicated
  -un, --user-new: PGUSER for new node being created

Note that values that are not specified will draw defaults from
PGHOST/PGDATABASE/PGPORT/PGUSER

Note that Slony-I tools must be included in PATH

Note that the use of .pgpass and/or suitably permissive pg_hba.conf is
assumed - this script provides no way to specify passwords on the
command line.

EOF
}

until [ -z "$1" ]  # Until all parameters used up...
do
  case $1 in
      "-c" | "--cluster")
	SCLUSTER=$2
	echo "set cluster: ${SCLUSTER}"
        shift
	shift
	;;
      "-n" | "--nodenum")
        NODENUM=$2
        shift
	shift
	;;
      "-ho" | "--host-origin")
        HOSTORIGIN=$2
	shift
	shift
	;;
      "-hn" | "--host-new")
        HOSTNEW=$2
	shift
	shift
	;;
      "-hd" | "--host-dup")
        HOSTDUP=$2
	shift
	shift
	;;
      "-po" | "--port-origin")
        PORTORIGIN=$2
	shift
	shift
	;;
      "-pn" | "--port-new")
        PORTNEW=$2
	shift
	shift
	;;
      "-pd" | "--port-dup")
        PORTDUP=$2
	shift
	shift
	;;
      "-do" | "--database-origin")
        DATABASEORIGIN=$2
	shift
	shift
	;;
      "-dn" | "--database-new")
        DATABASENEW=$2
	shift
	shift
	;;
      "-dd" | "--database-dup")
        DATABASEDUP=$2
	shift
	shift
	;;
      "-uo" | "--user-origin")
        USERORIGIN=$2
	shift
	shift
	;;
      "-un" | "--user-new")
        USERNEW=$2
	shift
	shift
	;;
      "-ud" | "--user-dup")
        USERDUP=$2
	shift
	shift
	;;
      *)
        help
	exit -1
	;;
  esac
done


SCLUSTER=${SCLUSTER:-"no_cluster_specified"}
NODENUM=${NODENUM:-"no_new_node_specified"}
HOSTORIGIN=${HOSTORIGIN:-${PGHOST}}
HOSTNEW=${HOSTNEW:-${PGHOST}}
HOSTDUP=${HOSTDUP:-${PGHOST}}
PORTORIGIN=${PORTORIGIN:-${PGPORT}}
PORTNEW=${PORTNEW:-${PGPORT}}
PORTDUP=${PORTDUP:-${PGPORT}}
DATABASEORIGIN=${DATABASEORIGIN:-${PGDATABASE}}
DATABASENEW=${DATABASENEW:-${PGDATABASE}}
DATABASEDUP=${DATABASEDUP:-${PGDATABASE}}
USERORIGIN=${USERORIGIN:-${PGUSER}}
USERNEW=${USERNEW:-${PGUSER}}
USERDUP=${USERDUP:-${PGUSER}}

case `uname` in
  SunOS|AIX|MINGW32*)
        for i in /tmp /usr/tmp /var/tmp; do
      if [ -d $i ]; then
        tmpdir=$i
        break
      fi
    done
    if [ -z $tmpdir ]; then
       err 3 "unable to create tmp dir"
       exit
    fi

    # do this 10 times else fail
    rstring=$(random_az 8)
    rstring=duplicate-node.$rstring
    
    mkdir $tmpdir/$rstring
    retcode=$?
    if [ $retcode -ne 0 ]; then
      err $retcode "unable to create temp dir"
    else
      mktmp=$tmpdir/$rstring
    fi
  ;;
  Linux)
    mktmp=`mktemp -d -t duplicate-node.XXXXXX`
    if [ $MY_MKTEMP_IS_DECREPIT ] ; then
        mktmp=`mktemp -d /tmp/duplicate-node.XXXXXX`
    fi
    if [ ! -d $mktmp ]; then
      err 3 "mktemp failed"
    fi
  ;;
  *)
    mktmp=`mktemp -d -t duplicate-node`
    if [ ! -d $mktmp ]; then
      err 3 "mktemp failed"
    fi
  ;;
esac

mkdir -p ${mktmp}

ORIGINPARMS=" -h ${HOSTORIGIN} -p ${PORTORIGIN} -d ${DATABASEORIGIN} -U ${USERORIGIN} "
PROVIDERPARMS=" -h ${HOSTDUP} -p ${PORTDUP} -d ${DATABASEDUP} -U ${USERDUP} "

echo "Origin parms: ${ORIGINPARMS}"
echo "Provider parms: ${PROVIDERPARMS}"

# Step 1.
#  - verify that origin node is legitimately an origin
QUERY="select count(1)=sum(case when set_origin = \"_${SCLUSTER}\".getlocalnodeid('_${SCLUSTER}') then 1 else 0 end) from \"_${SCLUSTER}\".sl_set;"
ORIGINP=`psql -At ${ORIGINPARMS} -c "${QUERY}"`
if [ x${ORIGINP} = "xt" ] ; then
    echo "Node: ${HOSTORIGIN}/${PORTORIGIN}/${DATABASEORIGIN} is an origin node - OK"
else
    echo "Node: ${HOSTORIGIN}/${PORTORIGIN}/${DATABASEORIGIN} is not an origin node"
    exit -1
fi

#  - verify that dupe node is a provider

QUERY="select count(1)=sum(case when set_origin = \"_${SCLUSTER}\".getlocalnodeid('_${SCLUSTER}') then 1 when exists (select 1 from \"_${SCLUSTER}\".sl_subscribe where sub_receiver= \"_${SCLUSTER}\".getlocalnodeid('_${SCLUSTER}') and sub_forward) then 1 else 0 end) from \"_${SCLUSTER}\".sl_set;"

PROVIDERP=`psql -At ${PROVIDERPARMS} -c "${QUERY}"`
if [ x${PROVIDERP} = "xt" ] ; then
    echo "Node: ${HOSTDUP}/${PORTDUP}/${DATABASEDUP} seems like a suitable provider - OK"
else
    echo "Node: ${HOSTDUP}/${PORTDUP}/${DATABASEDUP} is not a suitable provider"
    exit -1
fi

PROVQ="select \"_${SCLUSTER}\".getlocalnodeid('_${SCLUSTER}');"
PROVIDERNODE=`psql -At ${PROVIDERPARMS} -c "${PROVQ}"`
echo "Provider node ID: [${PROVIDERNODE}]"

# Step 2.
#  - use slony1_extract_schema.sh to get the schema from the origin

(PGPORT=${PORTORIGIN} PGHOST=${HOSTORIGIN} PGUSER=${USERORIGIN} slony1_extract_schema.sh ${DATABASEORIGIN} ${SCLUSTER} dupe_database) > ${mktmp}/schema.sql

# Step 3.
#  - create preamble based on dupe node + new node
SPREAMBLE=${mktmp}/slonik.preamble
echo "CLUSTER NAME = ${SCLUSTER};" > ${SPREAMBLE}
echo "node ${PROVIDERNODE} admin conninfo='dbname=${DATABASEDUP} host=${HOSTDUP} user=${USERDUP} port=${PORTDUP}';" >> ${SPREAMBLE}
echo "node ${NODENUM} admin conninfo='dbname=${DATABASENEW} host=${HOSTNEW} user=${USERNEW} port=${PORTNEW}';" >> ${SPREAMBLE}

#  - STORE NODE on the new node

echo "include <${SPREAMBLE}>;" > $mktmp/step1-storenode.slonik
echo "store node (id= ${NODENUM}, comment='Duplicate of node ${PROVIDERNODE}', event node=${PROVIDERNODE});" >> $mktmp/step1-storenode.slonik
#  - STORE PATH to get connections between new node and dupe node

echo "include <${SPREAMBLE}>;" > $mktmp/step2-storepath.slonik
echo "store path (server= ${NODENUM}, client=${PROVIDERNODE}, conninfo='dbname=${DATABASENEW} host=${HOSTNEW} user=${USERNEW} port=${PORTNEW}');" >> $mktmp/step2-storepath.slonik
echo "store path (server= ${PROVIDERNODE}, client=${NODENUM}, conninfo='dbname=${DATABASEDUP} host=${HOSTDUP} user=${USERDUP} port=${PORTDUP}');" >> $mktmp/step2-storepath.slonik

echo "include <${SPREAMBLE}>;" > $mktmp/step3-subscribe-sets.slonik

#  - Get list of sets that "dupe node" subscribes to *or provides*
SQUERY="select set_id from \"_${SCLUSTER}\".sl_set where set_origin = \"_${SCLUSTER}\".getlocalnodeid('_${SCLUSTER}') or exists (select 1 from \"_${SCLUSTER}\".sl_subscribe where sub_receiver= \"_${SCLUSTER}\".getlocalnodeid('_${SCLUSTER}') and sub_forward and set_id = sub_set);"
echo "Set query: ${SQUERY}"
SETS=`psql -At ${PROVIDERPARMS} -c "${SQUERY}" | sort -n`
for set in `echo ${SETS}`; do
  echo "Prepare subscription for set ${set}"
  echo "subscribe set (id=${set}, provider=${PROVIDERNODE}, receiver=${NODENUM}, forward=true);" >> ${mktmp}/step3-subscribe-sets.slonik
done

echo "Scripts to set up duplicate node have been set up in ${mktmp}"
echo "Please review them before running them."
