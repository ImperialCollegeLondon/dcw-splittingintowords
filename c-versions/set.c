/*
 * set.c: "set of strings" storage for C..
 * 	we store a set as a reduced (no values) hash table,
 * 	hashing each set member (key) into a dynamic array of
 * 	binary search trees, and then search/include/exclude
 * 	the key in/from the corresponding search tree.  The
 * 	set also stores a key print function pointer so that
 * 	the set members can be complex data structures printed
 * 	appropriately.  We handle exclusion of a member from
 * 	a set by marking the key as not "in" the set.
 *
 * (C) Duncan C. White, 1996-2017 although it seems longer:-)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "set.h"


#define	NHASH	32533


typedef struct tree_s *tree;


struct set_s {
	tree *		data;			/* dynamic array of trees */
	set_printfunc	p;
};

struct tree_s {
	set_key		k;			/* Key, i.e. set member */
	int		in;			/* is member included? */
	tree		left;			/* Left... */
	tree		right;			/* ... and Right trees */
};


/*
 * operation
 */
typedef enum { Search, Define, Exclude } tree_operation;


/* Private functions */

static void foreach_tree( tree, set_foreachcbfunc, void * );
static void dump_cb( set_key, void * );
static void include_cb( set_key, void * );
static void exclude_cb( set_key, void * );
static void exclude_if_notin_cb( set_key, void *);
static void diff_cb( set_key, void *);
static void count_cb( set_key, void *);
static tree copy_tree( tree );
static void free_tree( tree );
static int depth_tree( tree );
static tree talloc( set_key );
static tree tree_op( set, set_key, tree_operation );
static int shash( char * );


/*
 * Create an empty set
 */
set setCreate( set_printfunc p )
{
	int   i;
	set   s;

	s = (set) malloc( sizeof(struct set_s) );
	s->data = (tree *) malloc( NHASH*sizeof(tree) );
	s->p = p;

	for( i = 0; i < NHASH; i++ )
	{
		s->data[i] = NULL;
	}
	return s;
}


/*
 * Empty an existing set - ie. retain only the skeleton..
 */
void setEmpty( set s )
{
	int   i;

	for( i = 0; i < NHASH; i++ )
	{
		if( s->data[i] != NULL )
		{
			free_tree( s->data[i] );
			s->data[i] = NULL;
		}
	}
}


/*
 * Copy an existing set.
 */
set setCopy( set s )
{
	int   i;
	set   result;

	result = (set) malloc( sizeof(struct set_s) );
	result->data = (tree *) malloc( NHASH*sizeof(tree) );
	result->p = s->p;

	for( i = 0; i < NHASH; i++ )
	{
		result->data[i] = copy_tree( s->data[i] );
	}

	return result;
}


/*
 * Free the given set - clean it up and delete it's skeleton too..
 */
void setFree( set s )
{
	int   i;

	for( i = 0; i < NHASH; i++ )
	{
		if( s->data[i] != NULL )
		{
			free_tree( s->data[i] );
		}
	}
	free( (void *) s->data );
	free( (void *) s );
}


/*
 * Set metrics:
 *  calculate the min, max and average depth of all non-empty trees
 *  sadly can't do this with a setForeach unless the depth is magically
 *  passed into the callback..
 */
void setMetrics( set s, int *min, int *max, double *avg )
{
	int	i;
	int	nonempty = 0;
	int	total    = 0;

	*min =  100000000;
	*max = -100000000;
	for( i = 0; i < NHASH; i++ ) {
		if( s->data[i] != NULL )
		{
			int d = depth_tree( s->data[i] );
			if( d < *min ) *min = d;
			if( d > *max ) *max = d;
			total += d;
			nonempty++;
		}
	}
	*avg = ((double)total)/(double)nonempty;
}


/*
 * Include item in set s
 */
void setInclude( set s, set_key item )
{
	(void) tree_op( s, item, Define);
}


/*
 * Exclude item from set s
 */
void setExclude( set s, set_key item )
{
	(void) tree_op( s, item, Exclude);
}


/*
 * Convenience function:
 *  Given a changes string of the form "[+-]item[+-]item[+-]item..."
 *  modify the given set s, including (+) or excluding (-) items
 * NB: This assumes that key == char *..
 */
void setModify( set s, set_key changes )
{
	char *str = strdup( changes );	/* so we can modify it! */
	char *p = str;

	char cmd = *p;
	while( cmd != '\0' )		/* while not finished */
	{
		assert( cmd == '+' || cmd == '-' );
		p++;

		/* got a string of the form... [+-]itemstring[+-\0]... */
		/* cmd = the + or - command                            */
		/* and p points at the first char  ^p                  */

		/* find the next +- command,                  ^q       */
		char *q = p;
		for( ; *q != '\0' && *q != '+' && *q != '-'; q++ );

		/* terminate itemstring here, remembering the next cmd */
		char nextcmd = *q;
		*q = '\0';

		/* now actually include/exclude the item from the set  */
		if( cmd == '+' )
		{
			setInclude( s, p );
		} else
		{
			setExclude( s, p );
		}

		/* set up for next time                                */
		cmd = nextcmd;			   /* the next command */
		p = q;				   /* the next item    */
	}
	free( (void *)str );
}


/*
 * Look for an item in the set s
 */
int setIn( set s, set_key item )
{
	tree x = tree_op(s, item, Search);

	return x != NULL && x->in;
}


/*
 * perform a foreach operation over a given set
 * call a given callback for each item pair.
 */
void setForeach( set s, set_foreachcbfunc cb, void * arg )
{
	int	i;

	for( i = 0; i < NHASH; i++ ) {
		if( s->data[i] != NULL )
		{
			foreach_tree( s->data[i], cb, arg );
		}
	}
}


/* ----------- Higher level operations using setForeach -------------- */
/* - each using it's own callback, sometimes with a custom structure - */


/*
 * setDump: Display a set to a file.
 *  Here, we need to know where (FILE *out) and how (printfunc p) to
 *  display each item of the set.
 */
typedef struct { FILE *out; set_printfunc p; } dumparg;
static void dump_cb( set_key k, void * arg )
{
	dumparg *dd = (dumparg *)arg;
	if( dd->p != NULL )
	{
		(*(dd->p))( dd->out, k );
	} else
	{
		fprintf( dd->out, "%s,", k );
	}
}
void setDump( FILE *out, set s )
{
	dumparg arg; arg.out = out; arg.p = s->p;
	fputs("{ ",out);
	setForeach( s, &dump_cb, (void *)&arg );
	fputs(" }",out);
}


/*
 * setUnion: a += b
 *  include each item of b into a
 */
static void include_cb( set_key k, void *s )
{
	setInclude( (set)s, k );
}
void setUnion( set a, set b )
{
	setForeach( b, &include_cb, (void *)a );
}


/*
 * Set subtraction, a -= b
 *  exclude each item of b from a
 */
static void exclude_cb( set_key k, void *s )
{
	setExclude( (set)s, k );
}
void setSubtraction( set a, set b )
{
	setForeach( b, &exclude_cb, (void *)a );
}


/*
 * Set intersection, a = a&b
 *   exclude each member of a FROM a UNLESS in b too
 *   here we need to pass both sets to the callback,
 *   via this "pair of sets" structure:
 */
typedef struct { set a, b; } setpair;
static void exclude_if_notin_cb( set_key k, void *arg )
{
	setpair *d = (setpair *)arg;
	if( ! setIn(d->b, k) )
	{
		setExclude( d->a, k );
	}
}
void setIntersection( set a, set b )
{
	setpair data; data.a = a; data.b = b;
	setForeach( a, &exclude_if_notin_cb, (void *)&data );
}


/*
 * Set difference, simultaneous a -= b and b -= a
 *  exclude each item of both sets from both sets, LEAVING
 *  - a containing elements ONLY in a, and
 *  - b containing elements ONLY in b.
 */
static void diff_cb( set_key k, void *arg )
{
	setpair *d = (setpair *)arg;
	if( setIn(d->a, k) )
	{
		setExclude( d->a, k );
		setExclude( d->b, k );
	}
}
void setDiff( set a, set b )
{
	setpair data; data.a = a; data.b = b;
	setForeach( b, &diff_cb, (void *)&data );
}


/*
 * Set members: how many members in the set?
 */
static void count_cb( set_key k, void *arg )
{
	int *n = (int *)arg;
	(*n)++;
}
int setMembers( set s )
{
	int n = 0;
	setForeach( s, &count_cb, (void *)&n );
	return n;
}


/*
 * Set isEmpty: is the set empty?
 */
int setIsEmpty( set s )
{
	int n = setMembers(s);
	return n==0;
}


/* -------------------- Binary search tree ops --------------------- */

/*
 * Allocate a new node in the tree
 */
static tree talloc( set_key k )
{
	tree   p = (tree) malloc(sizeof(struct tree_s));

	if( p == NULL )
	{
		fprintf( stderr, "talloc: No space left\n" );
		exit(1);
	}
	p->left = p->right = NULL;
	p->k    = strdup(k);		/* Save key */
	p->in   = 1;			/* Include it */
	return p;
}


/*
 * Operate on the Binary Search Tree
 * Search, Define, Exclude.
 */
static tree tree_op( set s, set_key k, tree_operation op )
{
	tree	ptr;
	tree *	aptr = s->data + shash(k);

	while( (ptr = *aptr) != NULL )
	{
		int rc = strcmp(ptr->k, k);
		if( rc == 0 )
		{
			if( op == Define )
			{
				ptr->in = 1;
			} else if( op == Exclude )
			{
				ptr->in = 0;
			} else if( ! ptr->in )
			{
				return NULL;
			}
			return ptr;
		}
		if (rc < 0)
		{
			/* less - left */
			aptr = &(ptr->left);
		} else
		{
			/* more - right */
			aptr = &(ptr->right);
		}
	}

	if (op == Define )
	{
		return *aptr = talloc(k);	/* Alloc new node */
	}

	return NULL;				/* not found */
}


/*
 * Copy one tree
 */
static tree copy_tree( tree t )
{
	tree result = NULL;
	if( t )
	{
		result = talloc( t->k );
		result->in    = t->in;
		result->left  = copy_tree( t->left );
		result->right = copy_tree( t->right );
	}
	return result;
}


/*
 * foreach one tree
 */
static void foreach_tree( tree t, set_foreachcbfunc f, void * arg )
{
	assert( f != NULL );
	if( t )
	{
		if( t->left ) foreach_tree( t->left, f, arg );
		if( t->in ) (*f)( t->k, arg );
		if( t->right ) foreach_tree( t->right, f, arg );
	}
}


/*
 * Free one tree
 */
static void free_tree( tree t )
{
	if( t )
	{
		if( t->left ) free_tree( t->left );
		if( t->right ) free_tree( t->right );
		free( (void *) t->k );
		free( (void *) t );
	}
}


/*
 * Compute the depth of a given tree
 */
#define max(a,b)  ((a)>(b)?(a):(b))
static int depth_tree( tree t )
{
	if( t )
	{
		int d2 = depth_tree( t->left );
		int d3 = depth_tree( t->right );
		return 1 + max(d2,d3);
	}
	return 0;
}


/*
 * Calculate hash on a string
 */
static int shash( char *str )
{
	unsigned char	ch;
	unsigned int	hh;
	for (hh = 0; (ch = *str++) != '\0'; hh = hh * 65599 + ch );
	return hh % NHASH;
}
