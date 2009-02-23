/*-------------------------------------------------------------------------
 * slony_logshipper.h
 *
 *	Definitions for slony_logshipper
 *
 *	Copyright (c) 2003-2004, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *	$Id: slony_upgrade_20.h,v 1.1.2.1 2009-02-23 17:19:15 cbbrowne Exp $
 *-------------------------------------------------------------------------
 */


/* ----------
 * SlonDString
 * ----------
 */
#define		SLON_DSTRING_SIZE_INIT	256
#define		SLON_DSTRING_SIZE_INC	2

typedef struct
{
	size_t		n_alloc;
	size_t		n_used;
	char	   *data;
}	SlonDString;

#define		dstring_init(__ds) \
do { \
	(__ds)->n_alloc = SLON_DSTRING_SIZE_INIT; \
	(__ds)->n_used = 0; \
	(__ds)->data = malloc(SLON_DSTRING_SIZE_INIT); \
	if ((__ds)->data == NULL) { \
		perror("dstring_init: malloc()"); \
		exit(-1); \
	} \
} while (0)
#define		dstring_reset(__ds) \
do { \
	(__ds)->n_used = 0; \
	(__ds)->data[0] = '\0'; \
} while (0)
#define		dstring_free(__ds) \
do { \
	free((__ds)->data); \
	(__ds)->n_used = 0; \
	(__ds)->data = NULL; \
} while (0)
#define		dstring_nappend(__ds,__s,__n) \
do { \
	if ((__ds)->n_used + (__n) >= (__ds)->n_alloc)	\
	{ \
		while ((__ds)->n_used + (__n) >= (__ds)->n_alloc) \
			(__ds)->n_alloc *= SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
		if ((__ds)->data == NULL) \
		{ \
			perror("dstring_nappend: realloc()"); \
			exit(-1); \
		} \
	} \
	memcpy(&((__ds)->data[(__ds)->n_used]), (__s), (__n)); \
	(__ds)->n_used += (__n); \
} while (0)
#define		dstring_append(___ds,___s) \
do { \
	register int ___n = strlen((___s)); \
	dstring_nappend((___ds),(___s),___n); \
} while (0)
#define		dstring_addchar(__ds,__c) \
do { \
	if ((__ds)->n_used + 1 >= (__ds)->n_alloc)	\
	{ \
		(__ds)->n_alloc *= SLON_DSTRING_SIZE_INC; \
		(__ds)->data = realloc((__ds)->data, (__ds)->n_alloc); \
		if ((__ds)->data == NULL) \
		{ \
			perror("dstring_append: realloc()"); \
			exit(-1); \
		} \
	} \
	(__ds)->data[(__ds)->n_used++] = (__c); \
} while (0)
#define		dstring_terminate(__ds) \
do { \
	(__ds)->data[(__ds)->n_used] = '\0'; \
} while (0)
#define		dstring_data(__ds)	((__ds)->data)


typedef enum {
	LOG_DEBUG = 0,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
} log_level;


#ifndef MSGMAX
#define MSGMAX 1024
#endif


/*
 * Functions in dbutil.c
 */
int			slon_mkquery(SlonDString * dsp, char *fmt,...);
int			slon_appendquery(SlonDString * dsp, char *fmt,...);


/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
