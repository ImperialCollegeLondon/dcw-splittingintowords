/*
 *	backtrack: read a dictionary forming a dictionary set, add some
 *		   extra words from the command line, take a sentence
 *		   WITH NO SPACES, and attempt to parse the sentence as
 *		   a sequence of words.  Of course there can be many
 *		   solutions (aka the "loitering with intent" vs
 *		   "loitering within tent" problem).
 *		   Which solution do we find?  at each stage, we pick **the
 *		   longest possible prefix such that it's lowercased version
 *		   is a dictionary word**.  But if no solution is found
 *		   having picked the longest word, we backtrack and try the
 *		   next shortest word...
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "set.h"

#define min(x,y) ((x)<(y)?(x):(y))

// no single word in the dictionary longer than..
#define MAXWORDLEN 1024

// max number of words in setence..
#define MAXWORDS 100

typedef char aword[MAXWORDLEN];
typedef char *wordarray[MAXWORDS];

// while splitting, we represent words within the sentence
// as a list of lengths eg given "MostEnglishsentencesaremostlylowercase",
// we'd have 4 (Most), 7 (English), 9 (sentences), 3 (are) etc..
// this saves copying words out of the original sentence all the time
typedef int wordinfo[MAXWORDS];



/*
 * alllower( string );
 *	Lower case the given string, in place.
 */
void alllower( char *p )
{
	for( ; *p; ++p) *p = tolower(*p);
}


/*
 *  set worddset = readdict( wordlistfile, extra_words[], int *longest );
 *	Read a word list <wordlistfile> (and add some extra words contained
 *	in <extra_words[]>, terminated by NULL), build and return a set
 *	of all those LOWERCASED words, also setting *longest to the length
 *	of the longest word in the whole set.
 */
set readdict( char *wordlistfile, wordarray extra_words, int *longest )
{
	set dict = setCreate( NULL );
	*longest = 0;

	for( char **w = extra_words; *w != NULL; w++ )
	{
		// add lowercased word to dictionary
		alllower( *w );
		setInclude( dict, *w );
		// update longest
		int len = strlen(*w);
		if( len > *longest ) *longest = len;
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
		// update longest
		int len = strlen(word);
		if( len > *longest ) *longest = len;
	}
	fclose( fh );

	return dict;
}


/*
 * convertwords( sentence, nwords, wlen, result );
 *	Given a <sentence> in original case, the number of words <nwords>,
 *	and an array <wlen> of word lengths, build <result>, a word array.
 *	The storage for <result> must have alredy been allocated, but not
 *	the storage for each string (char *).
 */
void convertwords( char *sentence, int nwords, wordinfo wlen, wordarray result )
{
	assert( nwords < MAXWORDS );
	int size = strlen(sentence) + nwords + 1;
	assert( size < MAXWORDLEN );
	char *block = (char *)malloc( size*sizeof(char) );
	assert( block != NULL );
	char *dst = block;
	char *src = sentence;
	for( int i=0; i<nwords; i++ )
	{
		result[i] = dst;
		// copy wlen chars from src into dst..
		int l = wlen[i];
		strncpy( dst, src, l );
		dst[l] = '\0';
		//printf( "debug: word %d is %s\n", i, dst );
		src += l;
		dst += l+1;
	}
}


/*
 * int nwords = canbreakwords( lc_str, dict, maxwordlen, wordlen[], nwordssofar );
 *	Given a lower-case string <lc_str>, a dictionary set <dict>, and the
 *	length of the longest word in the set <maxwordlen>, try to break
 *	the original sentence up into an array of word lengths, preferring to
 *	pick the longest possible prefix that is a word in the dictionary set,
 *	but backtracking if necessary.
 *	The array of word lengths is built up in wordlen[], no more than
 *	MAXWORDS allowed.
 *	Return the number of words found - or -1 if no breakdown is possible.
 */
int canbreakwords( char *lc_str, set dict, int maxwordlen, wordinfo wordlen, int nwordssofar )
{
	for( int wlen = min(maxwordlen,strlen(lc_str)); wlen>0; wlen-- )
	{
		// consider word starting at lc_str, length wlen:
		// is it a dict word?

		// temporarly string terminate lc_str at wlen..
		char ch = lc_str[wlen];
		lc_str[wlen] = '\0';

		// check if lc_str is a word (in dict)?
		bool isword = setIn( dict, lc_str);

		// found a dictionary word?
		if( isword )
		{
			// add wlen to words so far..
			wordlen[nwordssofar] = wlen;
			assert( nwordssofar < MAXWORDS );

			//printf( "debug: cbw: found word %s of length %d\n", lc_str, wlen );

			// change it back
			lc_str[wlen] = ch;

			// have we finished the entire string?
			if( ch == '\0' )
			{
				return nwordssofar+1;
			}

			// try to break the rest..
			int nwords = canbreakwords( lc_str+wlen, dict, maxwordlen, wordlen, nwordssofar+1 );
			if( nwords != -1 )
			{
				return nwords;
			}
		}

		// change it back
		lc_str[wlen] = ch;

	}
	return -1;
}


/*
 * int nwords = breakwords( sentence, dict, longestwordlen, words[] );
 *	Given a <sentence> with no spaces, a dictionary set <dict>, and
 *	the length of the longest word in the set <longestwordlen>, break
 *	the original sentence up into an array of words, preferring to pick
 *	the longest possible prefix that is a word in the dictionary set,
 *	but backtracking to pick shorter word-prefixes if necessary.
 *	The array of words is built up in words[], no more than MAXWORDS
 *	allowed.  Each individual word can be no longer than MAXWORDLEN.
 *	Return the number of words found - or -1 if no breakdown is possible.
 */
int breakwords( char *sentence, set dict, int longestwordlen, wordarray words )
{
	assert( strlen(sentence) < MAXWORDLEN );
	aword lc_sentence;
	strcpy( lc_sentence, sentence );
	alllower( lc_sentence );

	wordinfo wordlen;
	int nwords = canbreakwords( lc_sentence, dict, longestwordlen, wordlen, 0 );

	if( nwords == -1 ) return -1;

	// now need to extract the words using wordlen[i] = starting posn
	convertwords( sentence, nwords, wordlen, words );
	return nwords;
}


aword wordlistfile = "/usr/share/dict/words";
char *usage =
	"backtrack (''|wordlistfile) sentencewithoutspaces [extra words]";

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
	int maxwordlen=0;
	set dict = readdict( wordlistfile, extra_words, &maxwordlen );
	printf( "read dict, maxwordlen=%d\n", maxwordlen );

	wordarray words;
	int nwords = breakwords( sentence, dict, maxwordlen, words );

	// print results:
	if( nwords == -1 )
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
	}

	return 0;
}
