/*-------------------------------------------------------------------------
 * xxid.c
 *
 *	Our own datatype for safe storage of transaction ID's.
 *
 *	Copyright (c) 2003, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: xxid.c,v 1.3 2003-11-18 15:11:35 wieck Exp $
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <limits.h>

#include "access/xact.h"


#ifndef PG_GETARG_TRANSACTIONID
#define PG_GETARG_TRANSACTIONID(n)	DatumGetTransactionId(PG_GETARG_DATUM(n))
#endif
#ifndef PG_RETURN_TRANSACTIONID
#define PG_RETURN_TRANSACTIONID(x)	return TransactionIdGetDatum(x)
#endif


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
Datum           _Slony_I_xxidin(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxidout(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxideq(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxidne(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxidlt(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxidle(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxidgt(PG_FUNCTION_ARGS);
Datum           _Slony_I_xxidge(PG_FUNCTION_ARGS);
Datum           _Slony_I_btxxidcmp(PG_FUNCTION_ARGS);
Datum           _Slony_I_getCurrentXid(PG_FUNCTION_ARGS);


/*
 *		xxidin			- data input function for type _Slony_I_xxid
 */
Datum
_Slony_I_xxidin(PG_FUNCTION_ARGS)
{
	char   *str = PG_GETARG_CSTRING(0);

	PG_RETURN_TRANSACTIONID((TransactionId) strtoul(str, NULL, 0));
}


/*
 *		xxidout			- data output function for type _Slony_I_xxid
 */
Datum
_Slony_I_xxidout(PG_FUNCTION_ARGS)
{
	TransactionId	value = PG_GETARG_TRANSACTIONID(0);
	char		   *str = palloc(11);

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

	PG_RETURN_BOOL(value1 < value2);
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

	PG_RETURN_BOOL(value1 <= value2);
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

	PG_RETURN_BOOL(value1 > value2);
}


/*
 *		xxidge			- is value1 <= value2 ?
 */
Datum
_Slony_I_xxidge(PG_FUNCTION_ARGS)
{
	TransactionId value1 = PG_GETARG_TRANSACTIONID(0);
	TransactionId value2 = PG_GETARG_TRANSACTIONID(1);

	if (TransactionIdEquals(value1, value2))
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(value1 >= value2);
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

	if (value1 < value2)
		PG_RETURN_INT32(-1);
	PG_RETURN_INT32(1);
}


/*
 *		getCurrentXid	- Return the current transaction ID as xxid
 */
Datum
_Slony_I_getCurrentXid(PG_FUNCTION_ARGS)
{
	PG_RETURN_TRANSACTIONID(GetCurrentTransactionId());
}


