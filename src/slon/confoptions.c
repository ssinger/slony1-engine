#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "slon.h"
#include "types.h"

#ifdef qsort
#undef qsort
#endif

static struct config_generic **conf_variables;
static int	size_conf_variables;
static int	num_conf_variables;

static int	conf_var_compare(const void *a, const void *b);
static int	conf_name_compare(const char *namea, const char *nameb);

bool		set_config_option(const char *name, const char *value);
void	   *get_config_option(const char *name);

static double real_placeholder;

void		dump_configuration(void);
void		build_conf_variables(void);



static struct config_int ConfigureNamesInt[] =
{

	{
		{
			(const char *) "vac_frequency",		/* conf name */
			gettext_noop("Sets how many cleanup cycles to run before a vacuum is done"),		/* short desc */
			gettext_noop("Sets how many cleanup cycles to run before a vacuum is done"),		/* long desc */
			SLON_C_INT			/* config type */
		},
		&vac_frequency,			/* var name */
		3,						/* default val */
		0,						/* min val */
		100						/* max val */
	},
	{
		{
			(const char *) "log_level",
			gettext_noop("debug log level"),
			gettext_noop("debug log level"),
			SLON_C_INT
		},
		&slon_log_level,
		0,
		-1,
		4
	},
	{
		{
			(const char *) "sync_interval",
			gettext_noop("sync event interval"),
			gettext_noop("sync event interval in ms"),
			SLON_C_INT
		},
		&sync_interval,
		2000,
		10,
		60000
	},
	{
		{
			(const char *) "sync_interval_timeout",
			gettext_noop("sync interval time out - milliseconds"),
			gettext_noop("sync interval time out - milliseconds"),
			SLON_C_INT
		},
		&sync_interval_timeout,
		10000,
		0,
		1200000
	},
	{
		{
			(const char *) "sync_group_maxsize",
			gettext_noop("maximum number of SYNCs to be grouped together into one transaction"),
			gettext_noop("if running log shipping, and collecting archives on multiple nodes it is likely desirable to set this to 1 so they are certain to have agreeable contents"),
			SLON_C_INT
		},
		&sync_group_maxsize,
		20,
		0,
		10000
	},
#ifdef HAVE_SYSLOG
	{
		{
			(const char *) "syslog",
			gettext_noop("Uses syslog for logging."),
			gettext_noop("If this parameter is 1, messages go both to syslog "
						 "and the standard output. A value of 2 sends output only to syslog. "
			"(Some messages will still go to the standard output/error.) The "
						 "default is 0, which means syslog is off."),
			SLON_C_INT
		},
		&Use_syslog,
		0,
		0,
		2
	},
#endif
	{
		{
			(const char *) "quit_sync_provider",
			gettext_noop("Node to watch for a final SYNC"),
			gettext_noop("We want to terminate slon when the worker thread reaches a certain SYNC number "
					"against a certain provider.  This is the provider... "),
			SLON_C_INT
		},
		&quit_sync_provider,
		0,
		0,
		2147483647
	},

	{
		{
			(const char *) "remote_listen_timeout",		/* conf name */
			gettext_noop("How long to wait, in seconds, before timeout when querying for remote events"),		/* short desc */
			gettext_noop("How long to wait, in seconds, before timeout when querying for remote events"),		/* long desc */
			SLON_C_INT			/* config type */
		},
		&remote_listen_timeout, /* var name */
		300,					/* default val */
		30,						/* min val */
		30000					/* max val */
	},
	{
		{
			(const char *) "monitor_interval",
			gettext_noop("monitor thread interval for dumping the state queue"),
			gettext_noop("number of milliseconds monitor thread waits to queue up status entries"),
			SLON_C_INT
		},
		&monitor_interval,
		500,
		10,
		12000
	},
	{
		{
			(const char *) "explain_interval",	/* conf name */
			gettext_noop("Interval in seconds in which the remote worker will report an explain of the log selection query"),	/* short desc */
			gettext_noop("Interval in seconds in which the remote worker will report an explain of the log selection query"),	/* long desc */
			SLON_C_INT			/* config type */
		},
		&explain_interval,		/* var name */
		0,						/* default val (never) */
		0,						/* min val */
		86400					/* max val (1 day) */
	},
	{
		{
			(const char *) "tcp_keepalive_idle",
			gettext_noop("The number of seconds after which a TCP keep alive "
						 "is sent across an idle connection. tcp_keepalive "
						 "must be enabled for this to take effect.  Default "
						 "of 0 means use operating system default"
						 "use default"),
			NULL,
			SLON_C_INT,
		},
		&keep_alive_idle,
		0,						/* default val */
		0,						/* min val */
		1073741824				/* max val */
	},
	{
		{
			(const char *) "tcp_keepalive_interval",
			gettext_noop("The number of seconds in between TCP keep alive "
						 "requests. tcp_keepalive "
						 "must be enabled. Default value of 0 means use "
						 "operating system defaut"),
			NULL,
			SLON_C_INT,
		},
		&keep_alive_interval,
		0,
		0,						/* min val */
		1073741824				/* max val */
	},
	{
		{
			(const char *) "tcp_keepalive_count",
			gettext_noop("The number of keep alive requests to the server "
						 "that can be lost before the connection is declared "
						 "dead. tcp_keep_alive must be on. Default value "
						 "of 0 means use operating system default"),
			NULL,
			SLON_C_INT,
		},
		&keep_alive_count,
		0,
		0,						/* min val */
		1073741824				/* max val */
	},
	{
		{
			(const char *) "apply_cache_size",
			gettext_noop("apply cache size"),
			gettext_noop("apply cache size in number of prepared queries"),
			SLON_C_INT
		},
		&apply_cache_size,
		100,
		10,
		2000
	},
	{{0}}
};

static struct config_bool ConfigureNamesBool[] =
{
	{
		{
			(const char *) "log_pid",	/* conf name */
			gettext_noop("Should logs include PID?"),	/* short desc */
			gettext_noop("Should logs include PID?"),	/* long desc */
			SLON_C_BOOL			/* config type */
		},
		&logpid,				/* var_name */
		false					/* default_value */
	},
	{
		{
			(const char *) "log_timestamp",
			gettext_noop("Should logs include timestamp?"),
			gettext_noop("Should logs include timestamp?"),
			SLON_C_BOOL
		},
		&logtimestamp,
		true
	},

	{

		{
			(const char *) "tcp_keepalive",
			gettext_noop("Enables sending of TCP KEEP alive between slon "
						 "and the PostgreSQL backends. "),
			NULL,
			SLON_C_BOOL,
		},
		&keep_alive,
		true
	},
	{
		{
			(const char *) "monitor_threads",
			gettext_noop("Should the monitoring thread be run?"),
			NULL,
			SLON_C_BOOL,
		},
		&monitor_threads,
		true
	},
	{{0}}
};

static struct config_real ConfigureNamesReal[] =
{
	{
		{
			(const char *) "real_placeholder",	/* conf name */
			gettext_noop("place holder"),		/* short desc */
			gettext_noop("place holder"),		/* long desc */
			SLON_C_REAL			/* config type */
		},
		&real_placeholder,		/* var_name */
		0.0,					/* default val */
		0.0,					/* min_value */
		1.0						/* max value */
	},
	{{0}}
};

static struct config_string ConfigureNamesString[] =
{
	{
		{
			(const char *) "cluster_name",		/* conf name */
			gettext_noop("Name of the replication cluster"),	/* short desc */
			NULL,				/* long desc */
			SLON_C_STRING		/* config type */
		},
		&rtcfg_cluster_name,	/* var_name */
		NULL					/* default value */
	},
	{
		{
			(const char *) "conn_info",
			gettext_noop("connection info string"),
			NULL,
			SLON_C_STRING
		},
		&rtcfg_conninfo,
		NULL
	},
	{
		{
			(const char *) "pid_file",
			gettext_noop("Where to write the pid file"),
			NULL,
			SLON_C_STRING
		},
		&pid_file,
		NULL
	},
	{
		{
			(const char *) "log_timestamp_format",
			gettext_noop("A strftime()-style log timestamp format string."),
			gettext_noop("If modified, a trailing space to separate this from the next field is likely wanted."),
			SLON_C_STRING
		},
		&log_timestamp_format,
		"%Y-%m-%d %H:%M:%S %Z "
	},
	{
		{
			(const char *) "archive_dir",
			gettext_noop("Where to drop the sync archive files"),
			NULL,
			SLON_C_STRING
		},
		&archive_dir,
		NULL
	},
	{
		{
			(const char *) "sql_on_connection",
			gettext_noop("SQL to send to each connected node upon "
						 "connection establishment, useful to enable "
						 "duration logging, or to adjust any other "
						 "connection settable GUC"),
			NULL,
			SLON_C_STRING
		},
		&sql_on_connection,
		NULL
	},

	{
		{
			(const char *) "lag_interval",
			gettext_noop("A PostgreSQL value compatible with ::interval "
						 "which indicates how far behind this node should "
						 "lag its providers."),
			NULL,
			SLON_C_STRING
		},
		&lag_interval,
		NULL
	},

	{
		{
			(const char *) "command_on_logarchive",
			gettext_noop("Command to run (probably a shell script) "
						 "every time a log archive is committed. "
						 "This command will be passed one parameter: "
						 "The full pathname of the archive file"
			),
			NULL,
			SLON_C_STRING
		},
		&command_on_logarchive,
		NULL
	},


#ifdef HAVE_SYSLOG
	{
		{
			(const char *) "syslog_facility",
			gettext_noop("Sets the syslog \"facility\" to be used when syslog enabled."),
			gettext_noop("Valid values are LOCAL0, LOCAL1, LOCAL2, LOCAL3, "
						 "LOCAL4, LOCAL5, LOCAL6, LOCAL7."),
			SLON_C_STRING
		},
		&Syslog_facility,
		"LOCAL0"
	},
	{
		{
			(const char *) "syslog_ident",
			gettext_noop("Sets the program name used to identify slon messages in syslog."),
			NULL,
			SLON_C_STRING
		},
		&Syslog_ident,
		"slon"
	},
#endif
	{
		{
			(const char *) "cleanup_interval",
			gettext_noop("A PostgreSQL value compatible with ::interval "
						 "which indicates what aging interval should be used "
						 "for deleting old events, and hence for purging sl_log_* tables."),
			NULL,
			SLON_C_STRING
		},
		&cleanup_interval,
		"10 minutes"
	},
	{{0}}
};

void
dump_configuration(void)
{
	int			i;

	for (i = 0; ConfigureNamesInt[i].gen.name; i++)
	{
		slon_log(SLON_CONFIG, "main: Integer option %s = %d\n",
			ConfigureNamesInt[i].gen.name, *(ConfigureNamesInt[i].variable));
	}
	for (i = 0; ConfigureNamesBool[i].gen.name; i++)
	{
		slon_log(SLON_CONFIG, "main: Boolean option %s = %d\n",
		  ConfigureNamesBool[i].gen.name, *(ConfigureNamesBool[i].variable));
	}
	for (i = 0; ConfigureNamesReal[i].gen.name; i++)
	{
		slon_log(SLON_CONFIG, "main: Real option %s = %f\n",
		  ConfigureNamesReal[i].gen.name, *(ConfigureNamesReal[i].variable));
	}
	for (i = 0; ConfigureNamesString[i].gen.name; i++)
	{
		slon_log(SLON_CONFIG, "main: String option %s = %s\n",
				 ConfigureNamesString[i].gen.name, ((*ConfigureNamesString[i].variable) == NULL) ? "[NULL]" : *(ConfigureNamesString[i].variable));
	}


}


void
build_conf_variables(void)
{
	int			size_vars;
	int			num_vars = 0;
	struct config_generic **conf_vars;
	int			i;

	for (i = 0; ConfigureNamesBool[i].gen.name; i++)
	{
		struct config_bool *conf = &ConfigureNamesBool[i];

		/*
		 * Rather than requiring vartype to be filled in by hand, do this:
		 */
		conf->gen.vartype = SLON_C_BOOL;
		num_vars++;
	}

	for (i = 0; ConfigureNamesInt[i].gen.name; i++)
	{
		struct config_int *conf = &ConfigureNamesInt[i];

		conf->gen.vartype = SLON_C_INT;
		num_vars++;
	}

	for (i = 0; ConfigureNamesReal[i].gen.name; i++)
	{
		struct config_real *conf = &ConfigureNamesReal[i];

		conf->gen.vartype = SLON_C_REAL;
		num_vars++;
	}

	for (i = 0; ConfigureNamesString[i].gen.name; i++)
	{
		struct config_string *conf = &ConfigureNamesString[i];

		conf->gen.vartype = SLON_C_STRING;
		num_vars++;
	}

	size_vars = num_vars + num_vars / 4;

	conf_vars = (struct config_generic **) malloc(size_vars * sizeof(struct config_generic *));
	if (conf_vars == NULL)
	{
		slon_log(SLON_FATAL, "malloc failed");
		return;
	}
	num_vars = 0;

	for (i = 0; ConfigureNamesBool[i].gen.name; i++)
	{
		conf_vars[num_vars++] = &ConfigureNamesBool[i].gen;
	}
	for (i = 0; ConfigureNamesInt[i].gen.name; i++)
	{
		conf_vars[num_vars++] = &ConfigureNamesInt[i].gen;
	}
	for (i = 0; ConfigureNamesReal[i].gen.name; i++)
	{
		conf_vars[num_vars++] = &ConfigureNamesReal[i].gen;
	}
	for (i = 0; ConfigureNamesString[i].gen.name; i++)
	{
		conf_vars[num_vars++] = &ConfigureNamesString[i].gen;
	}

	if (conf_variables)
	{
		free(conf_variables);
	}
	conf_variables = conf_vars;
	num_conf_variables = num_vars;
	size_conf_variables = size_vars;
	qsort((void *) conf_variables, (size_t) num_conf_variables, sizeof(struct config_generic *), conf_var_compare);
}


#ifdef NEED_ADD_CONF_VARIABLE
static bool
add_conf_variable(struct config_generic * var, int elevel)
{
	if (num_conf_variables + 1 >= size_conf_variables)
	{
		/*
		 * Increase the vector by 25%
		 */
		int			size_vars = size_conf_variables + size_conf_variables / 4;
		struct config_generic **conf_vars;

		if (size_vars == 0)
		{
			size_vars = 100;
			conf_vars = (struct config_generic **)
				malloc(size_vars * sizeof(struct config_generic *));
		}
		else
		{
			conf_vars = (struct config_generic **)
				realloc(conf_variables, size_vars * sizeof(struct config_generic *));
		}

		if (conf_vars == NULL)
		{
			slon_log(elevel, "malloc failed");
			return false;		/* out of memory */
		}
		conf_variables = conf_vars;
		size_conf_variables = size_vars;
	}
	conf_variables[num_conf_variables++] = var;
	qsort((void *) conf_variables, num_conf_variables,
		  sizeof(struct config_generic *), conf_var_compare);
	return true;
}
#endif

void
InitializeConfOptions(void)
{
	int			i;
	char	   *env;

	build_conf_variables();

	for (i = 0; i < num_conf_variables; i++)
	{
		struct config_generic *gconf = conf_variables[i];

		switch (gconf->vartype)
		{
			case SLON_C_BOOL:
				{
					struct config_bool *conf = (struct config_bool *) gconf;

					*conf->variable = conf->default_val;
					break;
				}
			case SLON_C_INT:
				{
					struct config_int *conf = (struct config_int *) gconf;

					*conf->variable = conf->default_val;
					break;
				}
			case SLON_C_REAL:
				{
					struct config_real *conf = (struct config_real *) gconf;

					*conf->variable = conf->default_val;
					break;
				}
			case SLON_C_STRING:
				{
					char	   *str;
					struct config_string *conf = (struct config_string *) gconf;

					*conf->variable = NULL;
					if (conf->default_val)
					{
						str = strdup(conf->default_val);
						*conf->variable = str;
					}
					break;
				}
		}
	}

	env = getenv("CLUSTER");
	if (env != NULL)
	{
		set_config_option("cluster_name", env);
	}
}

static bool
parse_bool(const char *value, bool *result)
{
	int			len = (int) strlen(value);

	if (strncasecmp(value, "true", len) == 0)
	{
		if (result)
		{
			*result = true;
		}
	}
	else if (strncasecmp(value, "false", len) == 0)
	{
		if (result)
		{
			*result = false;
		}
	}
	else if (strncasecmp(value, "yes", len) == 0)
	{
		if (result)
		{
			*result = true;
		}
	}
	else if (strncasecmp(value, "no", len) == 0)
	{
		if (result)
		{
			*result = false;
		}
	}
	else if (strncasecmp(value, "on", len) == 0)
	{
		if (result)
		{
			*result = true;
		}
	}
	else if (strncasecmp(value, "off", len) == 0)
	{
		if (result)
		{
			*result = false;
		}
	}
	else if (strncasecmp(value, "1", len) == 0)
	{
		if (result)
		{
			*result = true;
		}
	}
	else if (strncasecmp(value, "0", len) == 0)
	{
		if (result)
		{
			*result = false;
		}
	}
	else
	{
		return false;
	}
	return true;
}


/*
 * Try to parse value as an integer.  The accepted formats are the usual
 * decimal, octal, or hexadecimal formats.	If the string parses okay, return
 * true, else false.  If result is not NULL, return the value there.
 */
static bool
parse_int(const char *value, int *result)
{
	long		val;
	char	   *endptr;


	errno = 0;
	val = strtol(value, &endptr, 0);

	if (endptr == value || *endptr != '\0' || errno == ERANGE
#ifdef HAVE_LONG_INT_64
	/* if long > 32 bits, check for overflow of int4 */
		|| val != (long) ((int32) val)
#endif
		)
		return false;
	if (result)
		*result = (int) val;
	return true;
}


/*
 * Try to parse value as a floating point constant in the usual format.
 * If the value parsed okay return true, else false.  If result is not NULL,
 * return the semantic value there.
 */
static bool
parse_real(const char *value, double *result)
{
	double		val;
	char	   *endptr;

	errno = 0;
	val = strtod(value, &endptr);
	if (endptr == value || *endptr != '\0' || errno == ERANGE)
		return false;
	if (result)
		*result = val;
	return true;
}


static struct config_generic *
find_option(const char *name, int elevel)
{
	const char **key = &name;
	struct config_generic **res;

	res = (struct config_generic **)
		bsearch((void *) &key,
				(void *) conf_variables,
				(size_t) num_conf_variables,
				sizeof(struct config_generic *),
				conf_var_compare);
	if (res)
	{
		return *res;
	}
	else
	{
		slon_log(elevel, "conf option %s not found", name);
		return NULL;
	}
}

static int
conf_var_compare(const void *a, const void *b)
{
	struct config_generic *confa = *(struct config_generic **) a;
	struct config_generic *confb = *(struct config_generic **) b;

	return conf_name_compare(confa->name, confb->name);
}

static int
conf_name_compare(const char *namea, const char *nameb)
{
	/*
	 * The temptation to use strcasecmp() here must be resisted, because the
	 * array ordering has to remain stable across setlocale() calls. So, build
	 * our own with a simple ASCII-only downcasing.
	 */
	while (*namea && *nameb)
	{
		char		cha = *namea++;
		char		chb = *nameb++;

		if (cha >= 'A' && cha <= 'Z')
			cha += 'a' - 'A';
		if (chb >= 'A' && chb <= 'Z')
			chb += 'a' - 'A';
		if (cha != chb)
			return (int) (cha - chb);
	}
	if (*namea)
		return 1;				/* a is longer */
	if (*nameb)
		return -1;				/* b is longer */
	return 0;
}

void *
get_config_option(const char *name)
{
	struct config_generic *record;

	record = find_option(name, SLON_WARN);
	if (record == NULL)
	{
		slon_log(SLON_WARN, "unrecognized configuration parameter \"%s\"\n", name);
		return NULL;
	}
	switch (record->vartype)
	{
		case SLON_C_BOOL:
			{
				struct config_bool *conf = (struct config_bool *) record;

				return (void *) conf->variable;
				/* break; */
			}
		case SLON_C_INT:
			{
				struct config_int *conf = (struct config_int *) record;

				return (void *) conf->variable;
				/* break; */
			}
		case SLON_C_REAL:
			{
				struct config_real *conf = (struct config_real *) record;

				return (void *) conf->variable;
				/* break; */
			}
		case SLON_C_STRING:
			{
				struct config_string *conf = (struct config_string *) record;

				return (void *) *conf->variable;
				/* break; */
			}
	}
	return NULL;
}

bool
set_config_option(const char *name, const char *value)
{
	struct config_generic *record;


	record = find_option(name, SLON_WARN);

	if (record == NULL)
	{
		slon_log(SLON_WARN, "unrecognized configuration parameter \"%s\"\n", name);
		return false;
	}
	switch (record->vartype)
	{
		case SLON_C_BOOL:
			{
				struct config_bool *conf = (struct config_bool *) record;
				bool		newval = false;

				if (value)
				{
					if (!parse_bool(value, &newval))
					{
						slon_log(SLON_WARN, "parameter \"%s\" requires a Boolean value\n", name);
						return false;
					}
				}
				else
				{
					slon_log(SLON_CONFIG, "parameter \"%s\"\n", name);
				}

				*conf->variable = newval;

				break;
			}
		case SLON_C_INT:
			{
				struct config_int *conf = (struct config_int *) record;
				int			newval = 0;

				if (value)
				{
					if (!parse_int(value, &newval))
					{
						slon_log(SLON_WARN, "parameter \"%s\" requires a integer value\n", name);
						return false;
					}
					if (newval < conf->min || newval > conf->max)
					{
						slon_log(SLON_WARN, "%d is outside the valid range for parameter \"%s\" (%d .. %d)\n",
								 newval, name, conf->min, conf->max);
						return false;
					}
				}
				else
				{
					slon_log(SLON_CONFIG, "parameter \"%s\"\n", name);
				}
				*conf->variable = newval;
				break;
			}
		case SLON_C_REAL:
			{
				struct config_real *conf = (struct config_real *) record;
				double		newval = 0;

				if (value)
				{
					if (!parse_real(value, &newval))
					{
						slon_log(SLON_WARN, "parameter \"%s\" requires a numeric value\n", name);
						return false;
					}
					/* @ -realcompare @ */
					if (newval < conf->min || newval > conf->max)
					{
						slon_log(SLON_WARN, "%g is outside the valid range for parameter \"%s\" (%g .. %g)\n",
								 newval, name, conf->min, conf->max);
						return false;
					}
					/* @ +realcompare @ */
				}
				else
				{
					slon_log(SLON_CONFIG, "parameter \"%s\"\n", name);
				}
				*conf->variable = newval;
				break;
			}
		case SLON_C_STRING:
			{
				struct config_string *conf = (struct config_string *) record;
				char	   *newval = NULL;

				if (value)
				{
					newval = strdup(value);
					if (newval == NULL)
					{
						return false;
					}
				}
				else
				{
					slon_log(SLON_CONFIG, "parameter \"%s\"\n", name);
					free(newval);
				}
				*conf->variable = newval;
				break;
			}
	}
	return true;
}


/*
 * Local Variables:
 *		tab-width: 4
 *		c-indent-level: 4
 *		c-basic-offset: 4
 * End:
 */
