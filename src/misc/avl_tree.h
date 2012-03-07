/* ----------------------------------------------------------------------
 * avl_tree.h
 *
 *	Declarations for AVL style balanced tree support.
 *
 *	Copyright (c) 2003-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 * ----------------------------------------------------------------------
 */

#ifndef _AVL_TREE_H_INCLUDED_
#define _AVL_TREE_H_INCLUDED_

/* ----
 * Callback function type declarations
 * ----
 */
typedef int (AVLcompfunc) (void *, void *);
typedef void (AVLfreefunc) (void *);


/* ----
 * Data structures
 * ----
 */
typedef struct AVLnode_s
{
	struct AVLnode_s *lnode,
			   *rnode;
	int			ldepth,
				rdepth;
	void	   *cdata;
	int			deleted;
}	AVLnode;

typedef struct AVLtree_s
{
	AVLnode    *root;
	AVLcompfunc *compfunc;
	AVLfreefunc *freefunc;
}	AVLtree;

/* ----
 * Macros
 * ----
 */
#define		AVL_DATA(n)		(n)->cdata
#define		AVL_SETDATA(n,p) ((n)->cdata = (p))
#define		AVL_MAXDEPTH(n) (((n)->ldepth > (n)->rdepth) ? (n)->ldepth : (n)->rdepth)
#define		AVL_BALANCE(n)	((n)->rdepth - (n)->ldepth)

#define		AVL_INITIALIZER(cmp,free) {NULL, (cmp), (free)}


/* ----
 * Public functions
 * ----
 */
void avl_init(AVLtree * tree, AVLcompfunc * compfunc,
		 AVLfreefunc * freefunc);
void		avl_reset(AVLtree * tree);
AVLnode    *avl_insert(AVLtree * tree, void *cdata);
AVLnode    *avl_lookup(AVLtree * tree, void *cdata);
int			avl_delete(AVLtree * tree, void *cdata);

#endif   /* _AVL_TREE_H_INCLUDED_ */
