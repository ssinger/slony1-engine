#ifndef _CONFOPTIONS_H_
#define _CONFOPTIONS_H_

#include "c.h"


void InitializeConfOptions(void);

bool set_config_option(const char *name, const char *value);

extern double real_placeholder;
extern char *string_placeholder;


extern int vac_frequency;
extern int slon_log_level;
extern int sync_interval;
extern int sync_interval_timeout;

extern int sync_group_maxsize;

bool logpid;
bool logtimestamp;


enum config_type
{
        SLON_C_BOOL,
        SLON_C_INT,
        SLON_C_REAL,
        SLON_C_STRING
};

struct config_generic
{
        /* constant fields, must be set correctly in initial value: */
        const char *name;
        const char *short_desc;
        const char *long_desc;
        enum config_type vartype;       /* type of variable (set only at startup) */
};


struct config_int
{
	struct config_generic gen;
	/* these fields must be set correctly in initial value: */
	/* (all but reset_val are constants) */
	int	*variable;
        int	default_val;
        int     min;
        int     max;
};

struct config_bool
{
        struct config_generic gen;
        /* these fields must be set correctly in initial value: */
        /* (all but reset_val are constants) */
        bool	*variable;
        bool    default_val;
};

struct config_real
{
        struct config_generic gen;
        /* these fields must be set correctly in initial value: */
        /* (all but reset_val are constants) */
        double	*variable;
        double  default_val;
        double  min;
        double  max;
};

struct config_string
{
        struct config_generic gen;
        /* these fields must be set correctly in initial value: */
        /* (all are constants) */
        char		**variable;
        const char	*default_val;
};



static struct config_int ConfigureNamesInt[] =
{
        {
                {
                        (const char *)"vac_frequency",                                                                /* conf name */
                        gettext_noop("Sets how many cleanup cyclse to run before a vacum is done"),    /* short desc */
                        gettext_noop("Sets how many cleanup cyclse to run before a vacum is done"),    /* long desc */
                        SLON_C_INT                                                                      /* config type */
                },
                &vac_frequency,                                                                         /* var name */
                3,                                                                                      /* default val */
                0,                                                                                      /* min val */
                100                                                                                     /* max val */
        },
	{
		{
			(const char *)"log_level",
			gettext_noop("debug log level"),
			gettext_noop("debug log level"),
			SLON_C_INT
		},
		&slon_log_level,
		4,
		0,
		4
	},
	{
		{
			(const char *)"sync_interval",
			gettext_noop("sync even interval"),
			gettext_noop("sync even interval in ms"),
			SLON_C_INT			
		},
		&sync_interval,
		100,
		10,
		60000
	},
	{
		{
			(const char *)"sync_interval_timeout",
			gettext_noop("sync interval time out"),
			gettext_noop("sync interval time out"),
			SLON_C_INT
		},
		&sync_interval_timeout,
		1000,
		0,
		120000
	},
	{
		{
			(const char *)"sync_group_maxsize",
			gettext_noop("sync group"),
			gettext_noop("sync group"),
			SLON_C_INT
		},
		&sync_group_maxsize,
		6,
		0,
		100
	},

        NULL
};
static struct config_int ConfigureNamesBool[] =
{
        {
                {
                        (const char *)"log_pid",                                                             /* conf name */
                        gettext_noop("place holder"),                                                   /* short desc */
                        gettext_noop("place holder"),                                                   /* long desc */
                        SLON_C_BOOL                                                                     /* config type */
                },
                &logpid,                                                                      /* var_name */
                false                                                                                    /* default_value */
        },
        {
		{
			(const char *)"log_timestamp",
			gettext_noop("place holder"),
			gettext_noop("place holder"),
			SLON_C_BOOL
		},
		&logtimestamp,
		true
	},
	NULL
};

static struct config_int ConfigureNamesReal[] =
{
        {
                {
                        (const char *)"real_placeholder",                                                             /* conf name */
                        gettext_noop("place holder"),                                                   /* short desc */
                        gettext_noop("place holder"),                                                   /* long desc */
                        SLON_C_REAL                                                                     /* config type */
                },
                &real_placeholder,                                                                      /* var_name */
                0.0,                                                                                    /* default val */
                0.0,                                                                                    /* min_value */
                1.0                                                                                     /* max value */
        },
        NULL
};

static struct config_int ConfigureNamesString[] =
{
        {
                {
                        (const char *)"string_placeholder",                                                           /* conf name */
                        gettext_noop("place holder"),                                                   /* short desc */
                        gettext_noop("place holder"),                                                   /* long desc */
                        SLON_C_STRING                                                                   /* config type */
                },
                &string_placeholder,                                                                    /* var_name */
                "default"                                                                               /* default value */
        },
	NULL
};

#endif
