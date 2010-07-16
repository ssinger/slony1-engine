#!/bin/sh
# 
# Do cluster analyses

CLSCRHOME=/opt/dbs/scripts
PGPORT=5433 PGHOST=hacluster-logdbs PGDATABASE=app1logs  PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh app1logs
PGPORT=5433 PGHOST=hacluster-logdbs PGDATABASE=ap2logs   PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh ap2logs
PGPORT=5433 PGHOST=hacluster-logdbs PGDATABASE=app3logs  PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh app3logs
PGPORT=5432 PGHOST=hacluster-app1   PGDATABASE=sappapp1  PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh sappapp1
PGPORT=5435 PGHOST=hacluster-app4   PGDATABASE=sappapp4  PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh app4
PGPORT=5532 PGHOST=hacluster-ap2    PGDATABASE=sappap2   PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh sappap2
PGPORT=5416 PGHOST=hacluster-app3   PGDATABASE=sappapp3  PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh sappapp3
PGPORT=5747 PGHOST=hacluster-app5   PGDATABASE=sappapp5  PGUSER=slony ${CLSCRHOME}/slony-cluster-analysis.sh sappapp5
