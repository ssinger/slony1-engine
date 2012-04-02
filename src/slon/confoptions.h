/*	*/
#ifndef _CONFOPTIONS_H_
#define _CONFOPTIONS_H_

void		InitializeConfOptions(void);

bool		set_config_option(const char *name, const char *value);
void	   *get_config_option(const char *name);

void		dump_configuration(void);

extern char *rtcfg_cluster_name;
extern char *rtcfg_conninfo;

extern char *pid_file;
extern char *archive_dir;

extern int	slon_log_level;
extern int	sync_interval;
extern int	sync_interval_timeout;
extern int	remote_listen_timeout;

extern int	sync_group_maxsize;
extern int	desired_sync_time;

extern int	quit_sync_provider;
extern int	quit_sync_finalsync;
extern bool keep_alive;
extern int	keep_alive_idle;
extern int	keep_alive_interval;
extern int	keep_alive_count;

extern int	apply_cache_size;

/*
 * ----------
 * Global variables in cleanup_thread.c
 * ----------
 */

extern int	vac_frequency;
extern char *cleanup_interval;

char	   *Syslog_ident;
char	   *Syslog_facility;
int			Use_syslog;

bool		logpid;
bool		logtimestamp;
bool		drop_indices;
char	   *log_timestamp_format;
char	   *sql_on_connection;
char	   *lag_interval;
char	   *command_on_logarchive;

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
	enum config_type vartype;	/* type of variable (set only at startup) */
};


struct config_int
{
	struct config_generic gen;
	/* these fields must be set correctly in initial value: */
	/* (all but reset_val are constants) */
	int		   *variable;
	int			default_val;
	int			min;
	int			max;
};

struct config_bool
{
	struct config_generic gen;
	/* these fields must be set correctly in initial value: */
	/* (all but reset_val are constants) */
	bool	   *variable;
	bool		default_val;
};

struct config_real
{
	struct config_generic gen;
	/* these fields must be set correctly in initial value: */
	/* (all but reset_val are constants) */
	double	   *variable;
	double		default_val;
	double		min;
	double		max;
};

struct config_string
{
	struct config_generic gen;
	/* these fields must be set correctly in initial value: */
	/* (all are constants) */
	char	  **variable;
	const char *default_val;
};


/**
static struct config_int* ConfigureNamesInt;
static struct config_bool* ConfigureNamesBool;
static struct config_real *ConfigureNamesReal;
static struct config_string*  ConfigureNamesString;
**/
#endif
/*
 * Local Variables:
 *	tab-width: 4
 *	c-indent-level: 4
 *	c-basic-offset: 4
 * End:
 */
