/*	*/
#define MAXSTATEMENTS 1000
enum quote_states
{
	Q_NORMAL_STATE,
	Q_HOPE_TO_DASH,				/* If next char is -, then start a -- comment
								 * 'til the end of the line */
	Q_DASHING_STATE,			/* comment using -- to the end of the line */
	Q_HOPE_TO_CCOMMENT,			/* If next char is *, then start a C-style
								 * comment */
	Q_CCOMMENT,					/* Inside a C-style comment */
	Q_HOPE_CEND,				/* expecting the end of a C-style comment */
	Q_DOUBLE_QUOTING,			/* inside a "double-quoted" quoting */
	Q_SINGLE_QUOTING,			/* inside a 'single-quoted' quoting */
	Q_DOLLAR_QUOTING,			/* inside a $doll$ dollar quoted $doll$
								 * section */
	Q_DOLLAR_BUILDING,			/* inside the $doll$ of a dollar quoted
								 * section */
	Q_DOLLAR_UNBUILDING,		/* inside a possible closing $doll$ of a
								 * dollar quoted section */
	Q_DONE						/* NULL ends it all... */
};

extern int	scan_for_statements(const char *extended_statement);
