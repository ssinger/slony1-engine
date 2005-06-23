#include <pthread.h>
#include <string.h>
#include <errno.h>
#include "libpq-fe.h"
#include "confoptions.h"
#include "postgres.h"
#include "misc.h"


static struct config_generic **conf_variables;
static int	size_conf_variables;
static int	num_conf_variables;

static int	conf_var_compare(const void *a, const void *b);
static int	conf_name_compare(const char *namea, const char *nameb);

bool		set_config_option(const char *name, const char *value);
void	   *get_config_option(const char *name);

bool		bool_placeholder;
double		real_placeholder;
char	   *string_placeholder;



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

	conf_vars = (struct config_generic **)malloc(size_vars * sizeof(struct config_generic *));
	if (conf_vars == NULL)
	{
		slon_log(FATAL, "malloc failed");
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
	qsort((void *)conf_variables, num_conf_variables, sizeof(struct config_generic *), conf_var_compare);
}


static bool
add_conf_variable(struct config_generic *var, int elevel)
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
	qsort((void *)conf_variables, num_conf_variables,
		  sizeof(struct config_generic *), conf_var_compare);
	return true;
}

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
					struct config_bool *conf = (struct config_bool *)gconf;

					*conf->variable = conf->default_val;
					break;
				}
			case SLON_C_INT:
				{
					struct config_int *conf = (struct config_int *)gconf;

					*conf->variable = conf->default_val;
					break;
				}
			case SLON_C_REAL:
				{
					struct config_real *conf = (struct config_real *)gconf;

					*conf->variable = conf->default_val;
					break;
				}
			case SLON_C_STRING:
				{
					char	   *str;
					struct config_string *conf = (struct config_string *)gconf;

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
parse_bool(const char *value, bool * result)
{
	size_t		len = strlen(value);

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
		|| val != (long)((int32) val)
#endif
		)
		return false;
	if (result)
		*result = (int)val;
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
		bsearch((void *)&key,
				(void *)conf_variables,
				num_conf_variables,
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
	struct config_generic *confa = *(struct config_generic **)a;
	struct config_generic *confb = *(struct config_generic **)b;

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
			return cha - chb;
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
				struct config_bool *conf = (struct config_bool *)record;

				return (void *)conf->variable;
				break;
			}
		case SLON_C_INT:
			{
				struct config_int *conf = (struct config_int *)record;

				return (void *)conf->variable;
				break;
			}
		case SLON_C_REAL:
			{
				struct config_real *conf = (struct config_real *)record;

				return (void *)conf->variable;
				break;
			}
		case SLON_C_STRING:
			{
				struct config_string *conf = (struct config_string *)record;

				return (void *)*conf->variable;
				break;
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
				struct config_bool *conf = (struct config_bool *)record;
				bool		newval;

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
					slon_log(SLON_DEBUG2, "parameter \"%s\"\n", name);
				}

				*conf->variable = newval;

				break;
			}
		case SLON_C_INT:
			{
				struct config_int *conf = (struct config_int *)record;
				int			newval;

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
					slon_log(SLON_DEBUG2, "parameter \"%s\"\n", name);
				}
				*conf->variable = newval;
				break;
			}
		case SLON_C_REAL:
			{
				struct config_real *conf = (struct config_real *)record;
				double		newval;

				if (value)
				{
					if (!parse_real(value, &newval))
					{
						slon_log(SLON_WARN, "parameter \"%s\" requires a numeric value\n", name);
						return false;
					}
					if (newval < conf->min || newval > conf->max)
					{
						slon_log(SLON_WARN, "%g is outside the valid range for parameter \"%s\" (%g .. %g)\n",
								 newval, name, conf->min, conf->max);
						return false;
					}
				}
				else
				{
					slon_log(SLON_DEBUG2, "parameter \"%s\"\n", name);
				}
				*conf->variable = newval;
				break;
			}
		case SLON_C_STRING:
			{
				struct config_string *conf = (struct config_string *)record;
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
					slon_log(SLON_DEBUG2, "parameter \"%s\"\n", name);
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
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
