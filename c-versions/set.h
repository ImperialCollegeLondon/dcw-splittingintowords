/*
 * set.h: set (based on hash) storage for C..
 *
 * (C) Duncan C. White, 1996-2017 although it seems longer:-)
 */

typedef struct set_s *set;
typedef char *set_key;

typedef void (*set_printfunc)( FILE *, set_key );
typedef void (*set_foreachcbfunc)( set_key, void * );

extern set setCreate( set_printfunc p );
extern void setEmpty( set s );
extern set setCopy( set s );
extern void setFree( set s );
extern void setMetrics( set s, int * min, int * max, double * avg );
extern void setInclude( set s, set_key item );
extern void setExclude( set s, set_key item );
extern void setModify( set s, set_key changes );
extern int setIn( set s, set_key item );
extern void setForeach( set s, set_foreachcbfunc cb, void * arg );
extern void setDump( FILE * out, set s );
extern void setUnion( set a, set b );
extern void setSubtraction( set a, set b );
extern void setIntersection( set a, set b );
extern void setDiff( set a, set b );
extern int setMembers( set s );
extern int setIsEmpty( set s );
