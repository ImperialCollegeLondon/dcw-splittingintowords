april 2017: on LinkedIn's Plain Old C programming group, Eric Bidong set
himself a very hard challenge (that happens to be used as a sample Google
Interview problem used in a Youtube tutorial) - splitting a sentence with
no spaces up into words.  eg  "IamEricBidong" -> "I am Eric Bidong".

Obviously, lower case -> upper case transitions are a useful heuristic,
but could be at most only a part of the solution.  I've chosen to ignore
case entirely:-)

I tried various experiments first in Perl for convenience (directory
perl-versions):

First I wrote "findwords1", a relatively simple algorithm that at each
stages picks-the-longest-prefix-that-is-a-word-ignoring-case.  This works
pretty well in most cases - although of course the classic
"loiteringwithintent" comes out as "loitering within tent".

Note that the Linux /usr/share/dict/words contains every single letter a..z,
and loads of ridiculous non-real two-letter words.  I copied dict/words here,
removed all single letters (EXCEPT A and I) and a dozen or so non-real
two-letter "words" like "er": that's the "my-dict-words" file here.
Specify a different word list file via:

"./findwords1 -w ../my-dict-words SENTENCE"

Also, I gave findwords1 the ability to take any additional arguments (after
the sentence) as extra words to add to the dictionary.  For example:
tragically "Bidong" is not a word in Linux's dict/words.  So:

./findwords1 -w ../my-dict-words IamEricBidong

fails whereas

./findwords1 -w ../my-dict-words IamEricBidong bidong

succeeds (because the last "bidong" is the extra word).

Second, I wrote "findwords2", an extended version which adds an element
of backtracking, in case picking the longest prefix earlier leads to no
solution, but picking a slightly shorter prefix earlier would lead to a
solution.  For example: "iamericall" fails with findword1, because it
commits to "i america" and then fails with "ll", whereas findword2 delivers
"i am eric all".

Third, I wrote a rather different version called "findallpossible" which
produces ALL possible word sequences (where every element is in the
dictionary).

Fourth, I translated findwords1 into C, giving c-version/findlongest.c,
using a set-of-strings ADT module that I wrote many years ago - essentially
it's a hash table with no values:-)

	dcw, april 2017
