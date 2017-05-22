/*
 *	findlongest: read a dictionary forming a dictionary set, add some
 *		     extra words from the command line, take a sentence
 *		     WITH NO SPACES, and attempt to parse the sentence as
 *		     a sequence of words.  Of course there can be many
 *		     solutions (aka the "loitering with intent" vs
 *		     "loitering within tent" problem).
 *		     Which solution do we find?  at each stage, we pick **the
 *		     longest possible prefix such that it's lowercased version
 *		     is a dictionary word**.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "set.h"


// no single word in the dictionary longer than..
#define MAXWORDLEN 1024

// max number of words in setence..
#define MAXWORDS 100

typedef char aword[MAXWORDLEN];
typedef char *wordarray[MAXWORDS];


/*
 * alllower( string );
 *	Lower case the given string, in place.
 */
void alllower( char *p )
{
	for( ; *p; ++p) *p = tolower(*p);
}


/*
 *  set worddset = readdict( wordlistfile, extra_words[] );
 *	Read a word list <wordlistfile> (and add some extra words contained
 *	in <extra_words[]>, terminated by NULL), build and return a set
 *	of all those LOWERCASED words.
 */
set readdict( char *wordlistfile, wordarray extra_words )
{
	set dict = setCreate( NULL );

	for( char **w = extra_words; *w != NULL; w++ )
	{
		// add lowercased word to dictionary
		alllower( *w );
		setInclude( dict, *w );
	}

	// foreach line (word!) in wordlistfile
	FILE *fh = fopen( wordlistfile, "r" );
	assert( fh != NULL );
	aword word;
	while( fgets(word, MAXWORDLEN, fh ) != NULL )
	{
		// remove trailing '\n' - if not present, line too long: die!
		char *last = word + strlen(word) - 1;
		assert( *last == '\n' );
		*last = '\0';

		// add lowercased word to dictionary
		alllower( word );
		setInclude( dict, word );
	}
	fclose( fh );

	return dict;
}


/*
 * int len = findprefixlen( string, dict );
 *	Given a <string> and a dictionary set <dict>,
 *	find and return the length of the LONGEST prefix of string
 *	that is a word (i.e. present in dict).
 *	note: we don't need to return the longest prefix itself;
 *	just it's length, but it's nice to print the longest prefix out:-).
 */
int findprefixlen( char *string, set dict )
{
	aword prefix;
	aword longestword;
	strcpy( prefix, "" );
	strcpy( longestword, "" );
	char *prefixend = prefix;
	char *stringend = string+MAXWORDLEN;
	int maxlen = 0;

	for( char *p=string; *p != '\0' && p<stringend; p++ )
	{
		// extend prefix
		*prefixend++ = *p;
		*prefixend = '\0';
		//printf( "debug: string=%s, prefix=%s\n", string, prefix );

		int len = prefixend-prefix;

		// check if prefix is a word (in dict)?
		if( setIn( dict, prefix) )
		{
			//printf( "debug: found word: %s\n", prefix );
			assert( len > strlen(longestword) );
			strcpy( longestword, prefix );
			maxlen = len;
		}
	}
	printf( "longest prefix that is a word: %s, len %d\n",
		longestword, maxlen );
	return maxlen;
}


/*
 * int nwords = breakwords( sentence, lc_sentence, dict, words[] );
 *	Given a <sentence> in original case, and the same sentence in
 *	lower-case <lc_sentence>, and a dictionary set <dict>, break the
 *	original sentence up into an array of words, where each word is
 *	the **longest possible prefix** that is a word in the dictionary set.
 *	The array of words is built up in words[], no more than MAXWORDS
 *	allowed.  Each individual word can be no longer than MAXWORDLEN.
 *	Return the number of words found - or zero if no breakdown is possible.
 */
int breakwords( char *sentence, char *lc_sentence, set dict, wordarray words )
{
	int nwords = 0;
	while( *sentence != '\0' )
	{
		int len = findprefixlen( lc_sentence, dict );

		// fail if no prefix word found
		if( len == 0 ) return 0;

		// push corresponding prefix of original sentence onto words[]
		words[nwords] = malloc( (len+1) * sizeof(char) );
		char *s = words[nwords];
		strncpy( s, sentence, len );
		s[len] = '\0';
		nwords++;

		// now remove found word from sentence and lc_sentence
		sentence += len;
		lc_sentence += len;
	}
	return nwords;
}


aword wordlistfile = "/usr/share/dict/words";
char *usage =
	"findlongest (''|wordlistfile) sentencewithoutspaces [extra words]";

int main( int argc, char **argv )
{
	if( argc < 3 )
	{
		fprintf( stderr, "%s\n", usage );
		exit(1);
	}

	// if wordlistfile is an empty string, use above default
	if( strlen(argv[1]) > 0 )
	{
		strcpy( wordlistfile, argv[1] );
	}
	aword sentence;
	strcpy( sentence, argv[2] );
	int nextra = argc-3;
	assert( nextra < MAXWORDS );

	char **extra_words = argv+3;

	// dict: the set of all dictionary words, lower cased
	set dict = readdict( wordlistfile, extra_words );

	aword lc_sentence;
	strcpy( lc_sentence, sentence );
	alllower( lc_sentence );

	wordarray words;
	int nwords = breakwords( sentence, lc_sentence, dict, words );

	// print results:
	if( nwords == 0 )
	{
		printf( "No solution found\n" );
	} else
	{
		printf( "found solution with %d words\n", nwords );
		// where's Perl's "join" function when you need it:-)
		for( int i=0; i<nwords; i++ )
		{
			printf( "%s%c", words[i], i==nwords-1?'\n':' ' );
		}
		putchar( '\n' );
	}

	return 0;
}
