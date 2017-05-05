May 2017: on LinkedIn's Plain Old C programming group, Eric Bidong set
himself a very hard challenge (that happens to be used as a sample Google
Interview problem used in a Youtube tutorial) - splitting a sentence with
no spaces up into words.  eg  "IamEricBidong" -> "I am Eric Bidong".

Obviously, lower case -> upper case transitions are one possible useful
heuristic, this is what Howard Brodale lept upon and coded up, but that
could be at most only a part of the solution.  Why, because

**most English sentences don't conveniently uppercase the first letter of
every word**

Here in my experiments, I've chosen to ignore case entirely doing comparisons,
i.e. I do all "is this string a word" lookups with lower-case candidate words,
but then I extract the equivalent length string from the original sentence
for better reporting of the answer(s):-)

To explore the problem space, I tried various experiments first in my
favourite "executable pseudo-code" (Perl) for convenience (directory
perl-versions):


First I wrote "findwords1", a relatively simple algorithm that at each
stages picks-the-longest-prefix-that-is-a-word (ignoring case).  This
works surprisingly well in many cases - although of course the classic
"loiteringwithintent" comes out as "loitering within tent", as "within"
is a longer word with "with".

One problem: initially I used the Linux /usr/share/dict/words as the word
list file, but I found that it contains every single letter a..z, upper
and lower case, and also contains several ridiculous non-real two-letter
words.  This caused findwords1 to split "IamEricBidong" into
"I am Eric B id on g" or some such nonsense.  I explored min len restrictions,
but of course 'I' and 'a' must be words, so forget that.  Instead I copied
dict/words here as "my-dict-words", and removed all single letter entries
(EXCEPT A and I) and also removed a dozen or so two-letter "words" like "er"
and "li".

Specify a different word list file via:

"./findwords1 -w ../my-dict-words SENTENCE"

Also, I gave findwords1 the ability to take any additional arguments (after
the sentence) as extra words to add to the dictionary.  For example:
it happens that tragically "Bidong" is not a word in Linux's dict/words.  So:

./findwords1 -w ../my-dict-words IamEricBidong

fails completely, whereas

./findwords1 -w ../my-dict-words IamEricBidong bidong

succeeds (because the last "bidong" is the extra word, case doesn't matter).


Second, I wrote "findwords2", an extended version which adds an element
of backtracking, in case picking the longest prefix earlier leads to no
solution, but picking a slightly shorter prefix earlier would lead to a
solution.  For example:

./findwords1 -w ../my-dict-words iamericall

fails to find a solution at all, because it commits to "i america" and then
fails with "ll", whereas findword2 delivers "i am eric all".

Note that

./findwords2 -w ../my-dict-words "mostEnglishsentencesdon'tconvenientlyuppercasethefirstletterofeveryword

works to generate the sentence we saw earlier.


Third, I wrote a rather different version called "findallpossible" which
produces ALL possible word sequences (where every candidate word is in the
dictionary).  It's left as an exercise for the reader to work out which is
the most likely, or the most grammatical, or whatever.


Fourth, I translated findwords1 into C, giving c-version/findlongest.c,
using a set-of-strings ADT module that I wrote many years ago - essentially
it's a hash table with no values:-)

Note how 108 words of Perl (findwords1) becomes 764 lines of C (including
set.c and set.h), or 204 lines of C (excluding the set module, which to
be fair I had written years ago).  Actually, many of those lines (in both
Perl and C versions) are comments, without those it's roughly 80 lines of
Perl, 165 lines of C.  Either way, it seems roughly 2 lines of C per line
of Perl (still excluding the set module).

	dcw, May 2017
