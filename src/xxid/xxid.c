/*-------------------------------------------------------------------------
 * xxid.c
 *
 *	Our own datatype for safe storage of transaction ID's.
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: xxid.c,v 1.10 2005-06-09 15:02:15 wieck Exp $
 *-------------------------------------------------------------------------
 */

#include "config.h"
#include "postgres.h"

#include <limits.h>

#include "access/xact.h"
#include "access/transam.h"
#include "executor/spi.h"


#ifndef PG_GETARG_TRANSACTIONID
#define PG_GETARG_TRANSACTIONID(n)	DatumGetTransactionId(PG_GETARG_DATUM(n))
#endif
#ifndef PG_RETURN_TRANSACTIONID
#define PG_RETURN_TRANSACTIONID(x)	return TransactionIdGetDatum(x)
#endif


typedef struct
{
	int32		varsz;
	TransactionId xmin;
	TransactionId xmax;
	int			nxip;
	TransactionId xip[1];
}	xxid_snapshot;


PG_FUNCTION_INFO_V1(_Slony_I_xxidin);
PG_FUNCTION_INFO_V1(_Slony_I_xxidout);
PG_FUNCTION_INFO_V1(_Slony_I_xxideq);
PG_FUNCTION_INFO_V1(_Slony_I_xxidne);
PG_FUNCTION_INFO_V1(_Slony_I_xxidlt);
PG_FUNCTION_INFO_V1(_Slony_I_xxidle);
PG_FUNCTION_INFO_V1(_Slony_I_xxidgt);
PG_FUNCTION_INFO_V1(_Slony_I_xxidge);
PG_FUNCTION_INFO_V1(_Slony_I_btxxidcmp);
PG_FUNCTION_INFO_V1(_Slony_I_getCurrentXid);
PG_FUNCTION_INFO_V1(_Slony_I_getMinXid);
PG_FUNCTION_INFO_V1(_Slony_I_getMaxXid);
Datum		_Slony_I_xxidin(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxidout(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxideq(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxidne(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxidlt(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxidle(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxidgt(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxidge(PG_FUNCTION_ARGS);
Datum		_Slony_I_btxxidcmp(PG_FUNCTION_ARGS);
Datum		_Slony_I_getCurrentXid(PG_FUNCTION_ARGS);
Datum		_Slony_I_getMinXid(PG_FUNCTION_ARGS);
Datum		_Slony_I_getMaxXid(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(_Slony_I_xxid_snapshot_in);
PG_FUNCTION_INFO_V1(_Slony_I_xxid_snapshot_out);
PG_FUNCTION_INFO_V1(_Slony_I_xxid_lt_snapshot);
PG_FUNCTION_INFO_V1(_Slony_I_xxid_ge_snapshot);
Datum		_Slony_I_xxid_snapshot_in(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxid_snapshot_out(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxid_lt_snapshot(PG_FUNCTION_ARGS);
Datum		_Slony_I_xxid_ge_snapshot(PG_FUNCTION_ARGS);



/*
 *		xxidin			- data input function for type _Slony_I_xxid
 */
Datum
_Slony_I_xxidin(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);

	PG_RETURN_TRANSACTIONID((TransactionId) strtoul(str, NULL, 0));
}


/*
 *		xxidout			- data output function for type _Slony_I_xxid
 */
Datum
_Slony_I_xxidout(PG_FUNCTION_ARGS)
{
	TransactionId value = PG_GETARG_TRANSACTIONID(0);
	char	   *str = palloc(11);

	snprintf(str, 11, "%lu", (unsigned long)value);

	PG_RETURN_CSTRING(str);
}


/*
 *		xxideq			- is value1 = value2 ?
 */
Datum
_Slony_I_xxideq(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	PG_RETURN_BOOL(TransactionIdEquals(value1, value2));
}


/*
 *		xxidne			- is value1 <> value2 ?
 */
Datum
_Slony_I_xxidne(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	PG_RETURN_BOOL(!TransactionIdEquals(value1, value2));
}


/*
 *		xxidlt			- is value1 < value2 ?
 */
Datum
_Slony_I_xxidlt(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	if (TransactionIdEquals(value1, value2))
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(TransactionIdPrecedes(value1, value2));
}


/*
 *		xxidle			- is value1 <= value2 ?
 */
Datum
_Slony_I_xxidle(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	if (TransactionIdEquals(value1, value2))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(TransactionIdPrecedesOrEquals(value1, value2));
}


/*
 *		xxidgt			- is value1 > value2 ?
 */
Datum
_Slony_I_xxidgt(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	if (TransactionIdEquals(value1, value2))
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(TransactionIdFollows(value1, value2));
}


/*
 *		xxidge			- is value1 >= value2 ?
 */
Datum
_Slony_I_xxidge(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	if (TransactionIdEquals(value1, value2))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(TransactionIdFollowsOrEquals(value1, value2));
}


/*
 *		btxxidcmp		- Three state compare
 */
Datum
_Slony_I_btxxidcmp(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	if (TransactionIdEquals(value1, value2))
		PG_RETURN_INT32(0);

	if (TransactionIdPrecedes(value1, value2))
		PG_RETURN_INT32(-1);
	PG_RETURN_INT32(1);
}


/*
 *		getCurrentXid	- Return the current transaction ID as xxid
 */
Datum
_Slony_I_getCurrentXid(PG_FUNCTION_ARGS)
{
	PG_RETURN_TRANSACTIONID(GetTopTransactionId());
}


/*
 *		getMinXid	- Return the minxid from the current snapshot
 */
Datum
_Slony_I_getMinXid(PG_FUNCTION_ARGS)
{
	if (SerializableSnapshot == NULL)
		elog(ERROR, "Slony-I: SerializableSnapshot is NULL in getMinXid()");

	PG_RETURN_TRANSACTIONID(SerializableSnapshot->xmin);
}


/*
 *		getMaxXid	- Return the maxxid from the current snapshot
 */
Datum
_Slony_I_getMaxXid(PG_FUNCTION_ARGS)
{
	if (SerializableSnapshot == NULL)
		elog(ERROR, "Slony-I: SerializableSnapshot is NULL in getMaxXid()");

	PG_RETURN_TRANSACTIONID(SerializableSnapshot->xmax);
}


/*
 *		xxid_snapshot_in	- input function for type xxid_snapshot
 */
Datum
_Slony_I_xxid_snapshot_in(PG_FUNCTION_ARGS)
{
	static int	a_size = 0;
	static TransactionId *xip = NULL;

	int			a_used = 0;
	TransactionId xmin;
	TransactionId xmax;
	xxid_snapshot *snap;
	int			size;

	char	   *str = PG_GETARG_CSTRING(0);
	char	   *endp;

	if (a_size == 0)
	{
		a_size = 4096;
		xip = (TransactionId *) malloc(sizeof(TransactionId) * a_size);
		if (xip == NULL)
			elog(ERROR, "Out of memory in xxid_snapshot_in");
	}

	xmin = (TransactionId) strtoul(str, &endp, 0);
	if (*endp != ':')
		elog(ERROR, "illegal xxid_snapshot input format");
	str = endp + 1;

	xmax = (TransactionId) strtoul(str, &endp, 0);
	if (*endp != ':')
		elog(ERROR, "illegal xxid_snapshot input format");
	str = endp + 1;

	while (*str != '\0')
	{
		if (a_used >= a_size)
		{
			a_size *= 2;
			xip = (TransactionId *) realloc(xip, sizeof(TransactionId) * a_size);
			if (xip == NULL)
				elog(ERROR, "Out of memory in xxid_snapshot_in");
		}

		if (*str == '\'')
		{
			str++;
			xip[a_used++] = (TransactionId) strtoul(str, &endp, 0);
			if (*endp != '\'')
				elog(ERROR, "illegal xxid_snapshot input format");
			str = endp + 1;
		}
		else
		{
			xip[a_used++] = (TransactionId) strtoul(str, &endp, 0);
			str = endp;
		}
		if (*str == ',')
			str++;
		else
		{
			if (*str != '\0')
				elog(ERROR, "illegal xxid_snapshot input format");
		}
	}

	size = offsetof(xxid_snapshot, xip) + sizeof(TransactionId) * a_used;
	snap = (xxid_snapshot *) palloc(size);
	snap->varsz = size;
	snap->xmin = xmin;
	snap->xmax = xmax;
	snap->nxip = a_used;
	if (a_used > 0)
		memcpy(&(snap->xip[0]), xip, sizeof(TransactionId) * a_used);

	PG_RETURN_POINTER(snap);
}


/*
 *		xxid_snapshot_out	- output function for type xxid_snapshot
 */
Datum
_Slony_I_xxid_snapshot_out(PG_FUNCTION_ARGS)
{
	xxid_snapshot *snap = (xxid_snapshot *) PG_GETARG_VARLENA_P(0);

	char	   *str = palloc(28 + snap->nxip * 13);
	char	   *cp = str;
	int			i;

	snprintf(str, 26, "%lu:%lu:", (unsigned long)snap->xmin,
			 (unsigned long)snap->xmax);
	cp = str + strlen(str);

	for (i = 0; i < snap->nxip; i++)
	{
		snprintf(cp, 13, "%lu%s", (unsigned long)snap->xip[i],
				 (i < snap->nxip - 1) ? "," : "");
		cp += strlen(cp);
	}

	PG_RETURN_CSTRING(str);
}


/*
 *		xxid_lt_snapshot	- is xid < snapshot ?
 */
Datum
_Slony_I_xxid_lt_snapshot(PG_FUNCTION_ARGS)
{
	TransactionId value = PG_GETARG_TRANSACTIONID(0);
	xxid_snapshot *snap = (xxid_snapshot *) PG_GETARG_VARLENA_P(1);
	int			i;

	if (TransactionIdPrecedes(value, snap->xmin))
		PG_RETURN_BOOL(true);

	if (!TransactionIdPrecedes(value, snap->xmax))
		PG_RETURN_BOOL(false);

	for (i = 0; i < snap->nxip; i++)
	{
		if (TransactionIdEquals(value, snap->xip[i]))
			PG_RETURN_BOOL(false);
	}

	PG_RETURN_BOOL(true);
}


/*
 *		xxid_ge_snapshot	- is xid >= snapshot ?
 */
Datum
_Slony_I_xxid_ge_snapshot(PG_FUNCTION_ARGS)
{
	TransactionId value = PG_GETARG_TRANSACTIONID(0);
	xxid_snapshot *snap = (xxid_snapshot *) PG_GETARG_VARLENA_P(1);
	int			i;

	if (TransactionIdEquals(value, snap->xmax))
		PG_RETURN_BOOL(true);
	if (TransactionIdFollowsOrEquals(value, snap->xmax))
		PG_RETURN_BOOL(true);

	if (TransactionIdPrecedes(value, snap->xmin))
		PG_RETURN_BOOL(false);

	for (i = 0; i < snap->nxip; i++)
	{
		if (TransactionIdEquals(value, snap->xip[i]))
			PG_RETURN_BOOL(true);
	}

	PG_RETURN_BOOL(false);
}
