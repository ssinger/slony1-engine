/* ----------------------------------------------------------------------
 * avl_tree.c
 *
 *	AVL style self balancing tree support.
 *
 *	Copyright (c) 2007-2009, PostgreSQL Global Development Group
 *	Author: Jan Wieck, Afilias USA INC.
 *
 *
 * ----------------------------------------------------------------------
 */

#include "avl_tree.h"

/* ----
 * local function declarations
 * ----
 */
static AVLnode *avl_makenode(void);
static void avl_reset_node(AVLnode * node, AVLfreefunc * freefunc);
static int avl_insertinto(AVLtree * tree, AVLnode ** node,
			   void *cdata, AVLnode ** result);
static void avl_rotate_left(AVLnode ** node);
static void avl_rotate_right(AVLnode ** node);


/* ----
 * avl_init() -
 *
 *	Initilize an AVL tree structure.
 *
 *	compfunc is a user supplied tri-state comparision function that compares
 *	two tree elements and returns <0, 0 or >0 (like strcmp).
 *
 *	If freefunc is specified, it is called to free no longer used elements.
 *	Note that the element will not immediately be free'd on avl_delete(),
 *	but only on a avl_delete, avl_insert sequence using the same key.
 * ----
 */
void
avl_init(AVLtree * tree, AVLcompfunc * compfunc, AVLfreefunc * freefunc)
{
	tree->root = NULL;
	tree->compfunc = compfunc;
	tree->freefunc = freefunc;
}


/* ----
 * avl_reset() -
 *
 *	Reset an AVL tree to empty. This will call the user defined
 *	free function for all elements in the tree, destroy all nodes
 *	and declare the tree empty again.
 * ----
 */
void
avl_reset(AVLtree * tree)
{
	avl_reset_node(tree->root, tree->freefunc);
	tree->root = NULL;
}


/* ----
 * avl_reset_node() -
 *
 *	avl_reset()'s workhorse.
 * ----
 */
void
avl_reset_node(AVLnode * node, AVLfreefunc * freefunc)
{
	if (node == NULL)
		return;

	avl_reset_node(node->lnode, freefunc);
	avl_reset_node(node->rnode, freefunc);

	if (freefunc !=NULL)
		freefunc	(node->cdata);

	free(node);
}


/* ----
 * avl_insert() -
 *
 *	Insert an element into an AVL tree. On return AVL_DATA(node) might
 *	point to an existing entry with the same key. If the caller replaces
 *	the entry, it must free the existing one since AVL will no longer
 *	keep track of it.
 * ----
 */
AVLnode *
avl_insert(AVLtree * tree, void *cdata)
{
	AVLnode    *result;

	/*
	 * If this is an empty tree, create the root node.
	 */
	if (tree->root == NULL)
		return (tree->root = avl_makenode());

	/*
	 * Traverse the tree to find the insert point.
	 */
	result = NULL;
	avl_insertinto(tree, &(tree->root), cdata, &result);
	return result;
}


/* ----
 * avl_lookup() -
 *
 *	Search for an existing key in the tree.
 * ----
 */
AVLnode *
avl_lookup(AVLtree * tree, void *cdata)
{
	AVLnode    *node;
	int			cmp;

	node = tree->root;
	while (node != NULL)
	{
		cmp = tree->compfunc(cdata, node->cdata);
		if (cmp == 0)
		{
			/*
			 * Found the node. If it is marked deleted, return NULL anyway.
			 * Otherwise return this node.
			 */
			if (node->deleted)
				return NULL;
			return node;
		}

		/*
		 * Search on ...
		 */
		if (cmp < 0)
			node = node->lnode;
		else
			node = node->rnode;
	}

	/*
	 * No such element found
	 */
	return NULL;
}


/* ----
 * avl_delete() -
 *
 *	Mark a given element as deleted. Subsequent lookups for the element
 *	will return NULL. Note that the caller should NOT free the memory
 *	of the element, as it is AVL's property altogether after the delete.
 *
 *	avl_delete() returns 1 on success, 0 if no such element was found.
 * ----
 */
int
avl_delete(AVLtree * tree, void *cdata)
{
	AVLnode    *node;

	if ((node = avl_lookup(tree, cdata)) == NULL)
		return 0;

	node->deleted = 1;
	return 1;
}


/* ----
 * avl_insertinto() -
 *
 *	The heart of avl_insert().
 * ----
 */
static int
avl_insertinto(AVLtree * tree, AVLnode ** node,
			   void *cdata, AVLnode ** result)
{
	int			cmp;

	/*
	 * Compare the node at hand with the new elements key.
	 */
	cmp = (tree->compfunc) (cdata, (*node)->cdata);

	if (cmp > 0)
	{
		/*
		 * New element is > than node. Insert to the right.
		 */
		if ((*node)->rnode == NULL)
		{
			/*
			 * Right side of current node is empty. Create a new node there
			 * and return new maximum depth. Note that this can only be 1
			 * because otherwise this node would have been unbalanced before.
			 */
			(*node)->rnode = *result = avl_makenode();
			(*node)->rdepth = 1;
			return 1;
		}

		/*
		 * Right hand node exists. Recurse into that and remember the new
		 * right hand side depth.
		 */
		(*node)->rdepth = avl_insertinto(tree, &((*node)->rnode),
										 cdata, result) + 1;

		/*
		 * A right hand side insert can unbalance this node only to the right.
		 */
		if (AVL_BALANCE(*node) > 1)
		{
			if (AVL_BALANCE((*node)->rnode) > 0)
			{
				/*
				 * RR situation, rebalance the tree by left rotating this
				 * node.
				 */
				avl_rotate_left(node);
			}
			else
			{
				/*
				 * RL situation, rebalance the tree by first right rotating
				 * the right hand side, then left rotating this node.
				 */
				avl_rotate_right(&((*node)->rnode));
				avl_rotate_left(node);
			}
		}

		return AVL_MAXDEPTH(*node);
	}
	else if (cmp < 0)
	{
		/*
		 * New element is < than node. Insert to the left.
		 */
		if ((*node)->lnode == NULL)
		{
			/*
			 * Left side of current node is empty. Create a new node there and
			 * return new maximum depth. Note that this can only be 1 because
			 * otherwise this node would have been unbalanced before.
			 */
			(*node)->lnode = *result = avl_makenode();
			(*node)->ldepth = 1;
			return AVL_MAXDEPTH(*node);
		}

		/*
		 * Left hand node exists. Recurse into that and remember the new left
		 * hand side depth.
		 */
		(*node)->ldepth = avl_insertinto(tree, &((*node)->lnode),
										 cdata, result) + 1;

		/*
		 * A left hand side insert can unbalance this node only to the left.
		 */
		if (AVL_BALANCE(*node) < -1)
		{
			if (AVL_BALANCE((*node)->lnode) < 0)
			{
				/*
				 * LL situation, rebalance the tree by right rotating this
				 * node.
				 */
				avl_rotate_right(node);
			}
			else
			{
				/*
				 * LR situation, rebalance the tree by first left rotating the
				 * left node, then right rotating this node.
				 */
				avl_rotate_left(&((*node)->lnode));
				avl_rotate_right(node);
			}
		}

		return AVL_MAXDEPTH(*node);
	}
	else
	{
		/*
		 * The new element is equal to this node. If it is marked for
		 * deletion, free the user element data now. The caller is supposed to
		 * replace it with a new element having the the key.
		 */
		if ((*node)->deleted && tree->freefunc !=NULL)
		{
			(tree->freefunc) ((*node)->cdata);
			(*node)->cdata = NULL;
			(*node)->deleted = 0;
		}
		*result = *node;
		return AVL_MAXDEPTH(*node);
	}
}


/* ----
 * avl_makenode() -
 *
 *	Create a new empty node.
 * ----
 */
static AVLnode *
avl_makenode(void)
{
	AVLnode    *new;

	new = (AVLnode *) malloc(sizeof(AVLnode));
	memset(new, 0, sizeof(AVLnode));

	return new;
}


/* ----
 * avl_rotate_left() -
 *
 *	Rebalance a node to the left.
 * ----
 */
static void
avl_rotate_left(AVLnode ** node)
{
	AVLnode    *rtmp;

	/*
	 * save right node
	 */
	rtmp = (*node)->rnode;

	/*
	 * move right nodes left side to this nodes right
	 */
	(*node)->rnode = rtmp->lnode;
	if (rtmp->lnode != NULL)
		(*node)->rdepth = AVL_MAXDEPTH(rtmp->lnode) + 1;
	else
		(*node)->rdepth = 0;

	/*
	 * Move this node to right nodes left side
	 */
	rtmp->lnode = *node;
	rtmp->ldepth = AVL_MAXDEPTH(*node) + 1;

	/*
	 * Let parent point to right node
	 */
	*node = rtmp;
}


/* ----
 * avl_rotate_right() -
 *
 *	Rebalance a node to the right.
 * ----
 */
static void
avl_rotate_right(AVLnode ** node)
{
	AVLnode    *ltmp;

	/*
	 * save left node
	 */
	ltmp = (*node)->lnode;

	/*
	 * move left nodes right side to this nodes left
	 */
	(*node)->lnode = ltmp->rnode;
	if (ltmp->rnode != NULL)
		(*node)->ldepth = AVL_MAXDEPTH(ltmp->rnode) + 1;
	else
		(*node)->ldepth = 0;

	/*
	 * Move this node to left nodes right side
	 */
	ltmp->rnode = *node;
	ltmp->rdepth = AVL_MAXDEPTH(*node) + 1;

	/*
	 * Let parent point to left node
	 */
	*node = ltmp;
}
