.\" ----------
.\" Slony-I-implementation.nr
.\"
.\"		Technical details of the actual Slony-I implementation.
.\"
.\"		Copyright (c) 2003, PostgreSQL Global Development Group
.\"		Author: Jan Wieck, Afilias USA INC.
.\"
.\"	$Id: Slony-I-implementation.nr,v 1.1 2004-01-17 17:26:06 wieck Exp $
.\" ----------
.\" format this document with
.\"
.\"     groff -t -p -ms -mpspic <file> > <output.ps>
.\"
.\" and ensure that the temporary index file exists and that you call
.\" groff again as long as that changes and that the Slon image exists ...
.\"
.\" Ah ... type "make" and you're done.
.\" ----------
.fam H
.po  1.25i
.nr PS 12
.ds Slony1     Slony-\f(TRI\fP
.ds Slony1bold \fBSlony-\fP\f(TBI\fP
.ds LH \*[Slony1]
.ds RH Working document
.\" .RP
.\" **********************************************************************
.PSPIC Slon.eps 6.0
.TL
\*[Slony1]
.br
\!.br
A replication system for PostgreSQL
.AU
Jan Wieck
.AI
Afilias USA INC.
Horsham, Pennsylvania, USA
.AB
This document describes several implementation details of
the \*[Slony1] replication engine and related components.
.AE
.\" **********************************************************************
.\" Some magic here to put the TOC right after the coverpage
.\"
.\" The Makefile created a file tmp.idx with a dummy TOC (.XS ... .XE)
.\" We source that file here and replace its content with the real
.\" TOC definition during the run. The Makefile then needs to invoke
.\" us twice so that in the second run we get the real TOC out of it.
.\"
.so tmp.idx
.TC
.open idx tmp.idx
.de TCENT
.ds xref*cur_SN \\*[SN]
.ds xref*cur_PN \\n[PN]
.ds xref*cur_TL \\$1
.write idx .XS \\n(PN
.write idx \\*[SN] \\$1
.write idx .XE
..
.\" **********************************************************************
.\" In the same file we hold cross reference information.
.\"
.de XREF
\\$3\\*[xref.\\$1.\\$2]\\$4
..
.de XPOS
.ds xref.\\$1.SN \\*[xref*cur_SN]
.ds xref.\\$1.PN \\*[xref*cur_PN]
.ds xref.\\$1.TL \\*[xref*cur_TL]
.write idx .ds xref.\\$1.SN \\*[xref*cur_SN]
.write idx .ds xref.\\$1.PN \\*[xref*cur_PN]
.write idx .ds xref.\\$1.TL \\*[xref*cur_TL]
..
.\" **********************************************************************
.af PN 1
.bp 1
.NH 1
Replication Engine Architecture
.TCENT "Replication Engine Architecture
.PP
.pso pic figure-1.pic
.PP
Figure 1 illustrates the thread architecture of the Slony replication
engine.
.NH 2
Sync Thread
.TCENT "Sync Thread
.PP
The Sync Thread maintains one connection to the local database.
In a configurable interval it checks if the action sequence has been
modified which indicates that some replicable database activity has
happened. It then generates a SYNC event by calling CreateEvent().
There are no interactions with other threads.
.NH 2
Cleanup Thread
.TCENT "Cleanup Thread
.PP
The Cleanup Thread maintains one connection to the local database.
In a configurable interval it calls the Cleanup() stored
procedure that will remove old confirm, event and log data. In another
interval it vacuums the confirm, event and log tables. There are no
interactions with other threads.
.NH 2
Local Listen Thread
.TCENT "Local Listen Thread
.PP
The Local Listen Thread maintains one connection to the local database.
It waits for "Event" notification and scans for events that originate
at the local node. When receiving new configuration events, caused by
administrative programs calling the stored procedures to change the
cluster configuration, it will modify the in-memory configuration of
the replication engine accordingly. This includes starting new threads
if required.
.NH 2
Remote Listen Threads
.TCENT "Remote Listen Threads
.PP
There is one Remote Listen Thread per remote node, the local node
receives events from (event provider). Regardless of the number of nodes
in the cluster, a typical leaf node will only have one Remote Listen Thread
since it receives events from all origins through the same provider.

A Remote Listen Thread maintains one database connection to its event
provider. Upon receiving notifications for events or confirmations, it
selects the new information from the respective tables and feeds them
into the respective internal message queues for the worker threads.

The engine starts one remote node specific worker thread (see below)
per node that is
the origin of one or more data sets, the local node is subscribed to.
If such a node specific worker thread exists, the event is forwarded into
its message queue. Confirmations from all origins, as well as events
originating on nodes where no specific thread exists for are forwarded
to the default worker thread.
.NH 2
Remote Worker Threads
.TCENT "Remote Worker Threads
.PP
There is one Remote Worker Thread per remote node, where the local node
is subscribed to originating data set(s). A remote worker thread maintains
one local database connection to do the actual replication
data application, the event storing and confirmation.

Every Set origination on the remote node the worker is handling, has
one data provider (which can but must not be identical to the event provider).
Per distinct data provider over these sets, the worker thread maintains
one database connection to perform the actual replication data selection. 

A remote worker thread waits on its internal message queue for
events forwarded by the remote listen thread(s). It then processes these
events, including data selection and application, and confirmation. This
also includes maintaining the engines in-memory configuration information.
.NH 2
Default Worker Thread
.TCENT "Default Worker Thread
.PP
The Default Worker Thread maintains one local database connection. 
It receives all confirmations as well as those
remote events, where no specific worker thread exists, via an
internal message queue. This handling
includes forwarding and confirming the event into the local database
and maintaining the engines in-memory cluster configuration information.
.\" **********************************************************************
.close idx
