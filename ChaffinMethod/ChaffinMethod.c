//
//  ChaffinMethod.c
//
//	A program based on Benjamin Chaffin's algorithm for finding minimal superpermutations.

/*

From the original version:
--------------------------

This script is used to show that minimal superpermutations on 5 symbols have 153 characters
(it can also compute the length of minimal superpermutations on 3 or 4 symbols).
More specifically, it computes the values in the table from the following blog post:
http://www.njohnston.ca/2014/08/all-minimal-superpermutations-on-five-symbols-have-been-found/

Author: Nathaniel Johnston (nathaniel@njohnston.ca), based on an algorithm by Benjamin Chaffin
Version: 1.00
Last Updated: Aug. 13, 2014

This version:
-------------

This version aspires to give a result for n=6 before the death of the sun,
but whether it can or not is yet to be confirmed.

Author: Greg Egan
Version: 2.00
Last Updated: 27 March 2019

Usage:

	ChaffinMethod n

Computes all strings (starting with 123...n) that contain the maximum possible number of permutations on n symbols while wasting w
characters, for all values of w from 1 up to the point where all permutations are visited (i.e. these strings become
superpermutations).

The strings for each value of w are written to files of the form:

Chaffin_<n>_W_<w>.txt
*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

//	Constants
//	---------

//	Logical truth values

#define TRUE (1==1)
#define FALSE (1==0)

//	Smallest and largest values of n accepted

#define MIN_N 3
#define MAX_N 7

//	Macros
//	------

#define CHECK_MEM(p) if ((p)==NULL) {printf("Insufficient memory\n"); exit(EXIT_FAILURE);};

//	Global variables
//	----------------

int n;				//	The number of symbols in the permutations we are considering
int fn;				//	n!
int nfactor;		//	10^(n-1)
int maxDec;			//	Highest decimal representation of a permutation we can encounter, plus 1
int maxW;			//	Largest number of wasted characters we allow for
char *curstr;		//	Current string
int max_perm;		//	Maximum number of permutations visited by any string seen so far
int *mperm_res;		//	For each number of wasted characters, the maximum number of permutations that can be visited
int *nBest;			//	For each number of wasted characters, the number of strings that achieve mperm_res permutations
int *bestLen;		//	For each number of wasted characters, the lengths of the strings that visit mperm_res permutations
int **bestStrings;	//	For each number of wasted characters, a list of all strings that visit mperm_res permutations
int tot_bl;			//	The total number of wasted characters we are allowing in strings, in current search
char *unvisited;	//	Flags set FALSE when we visit a permutation, indexed by decimal rep of permutation
char *valid;		//	Flags saying whether decimal rep of digit sequence corresponds to a valid permutation
char outputFileName[256];

//	Function definitions
//	--------------------

void fillStr(int pos, int pfound, int partNum, int leftPerm);
void fillStr2(int pos, int pfound, int partNum, char *swap, int *bestStr, int len);
int fac(int k);
void makePerms(int n, int **permTab);
void writeCurrentString(int newFile, int size);

//	Main program
//	------------

int main(int argc, const char * argv[])
{
if (argc==2 && argv[1][0]>='0'+MIN_N && argv[1][0]<='0'+MAX_N)
	{
	n = argv[1][0]-'0';
	}
else
	{
	printf("Please specify n from 1 to %d on the command line\n",MAX_N);
	exit(EXIT_FAILURE);
	};
	
fn=fac(n);

//	Storage for current string

CHECK_MEM( curstr = (char *)malloc(2*fn*sizeof(char)) )

//	Storage for things associated with different numbers of wasted characters

maxW = fn;

CHECK_MEM( nBest = (int *)malloc(maxW*sizeof(int)) )
CHECK_MEM( mperm_res = (int *)malloc(maxW*sizeof(int)) )
CHECK_MEM( bestLen = (int *)malloc(maxW*sizeof(int)) )
CHECK_MEM( bestStrings = (int **)malloc(maxW*sizeof(int *)) )

//	Compute 10^(n-1)

nfactor=10;
for (int k=0;k<n-2;k++) nfactor*=10;

//	We represent permutations p_1 + 10 p_2 + 100 p_3 + .... 10^(n-1) + p_n
//	maxDec is the highest value this can take, plus 1

maxDec = n;
for (int k=0;k<n-1;k++) maxDec = 10*maxDec + n-1-k;
maxDec++;

//	Generate a table of all permutations of n symbols

int **permTab;
CHECK_MEM( permTab = (int **)malloc(n*sizeof(int *)) )
makePerms(n,permTab+n-1);
int *p0 = permTab[n-1];

//	Set up flags that say whether each number is a valid permutation or not,
//	and whether we have visited a given permutation

CHECK_MEM( valid = (char *)malloc(maxDec*sizeof(char)) )
CHECK_MEM( unvisited = (char *)malloc(maxDec*sizeof(char)) )

for (int i=0;i<maxDec;i++) valid[i]=FALSE;
for (int i=0;i<fn;i++)
	{
	int tperm=0, factor=1;
	for (int j0=0;j0<n; j0++)
		{
		tperm+=factor*(p0[n*i+j0]);
		factor*=10;
		};
	valid[tperm]=TRUE;
	};
	
mperm_res[0] = n;		//	With no wasted characters, we can visit n permutations
max_perm = n;			//	Any new maximum (for however many wasted characters) must exceed that;
						//	we don't reset this within the loop, as the true maximum will increase as we increase tot_bl
						
//	Fill the first n entries of the string with [1...n], and compute the
//	associated decimal, as well as the partial decimal for [2...n]

int tperm0=0, factor=1;
for (int j0=0; j0<n; j0++)
	{
	curstr[j0] = j0+1;
	tperm0 += factor*(j0+1);
	factor*=10;
	};
int partNum0 = tperm0/10;
						
//	tot_bl is the total number of wasted characters we are allowing in strings;
//	we loop through increasing the value

for (tot_bl=1; tot_bl<maxW; tot_bl++)
	{
	sprintf(outputFileName,"Chaffin_%d_W_%d.txt",n,tot_bl);

//	Set all flags for unvisited permutations, then clear the flag for [1...n]

	for (int i=0; i<maxDec; i++) unvisited[i] = TRUE;
	unvisited[tperm0]=FALSE;
	
	//	Recursively fill in the string

	fillStr(n,1,partNum0,TRUE);
	
	//	Record maximum number of permutations visited with this many wasted characters

	mperm_res[tot_bl] = max_perm;

	printf("%d wasted characters: at most %d permutations, in %d characters, %d examples\n",
		tot_bl,max_perm,bestLen[tot_bl],nBest[tot_bl]);

	if (max_perm >= fn)
		{
		printf("\n-----\nDONE!\n-----\nMinimal superpermutations on %d symbols have %d wasted characters and a length of %u.\n\n",
			n,tot_bl,fn+tot_bl+n-1);
		break;
		};
		
	//	Read back list of best strings
	
	CHECK_MEM( bestStrings[tot_bl] = (int *)malloc(bestLen[tot_bl]*nBest[tot_bl]*sizeof(int)) )
	
	FILE *fp = fopen(outputFileName,"ra");
	if (fp==NULL)
		{
		printf("Unable to open file %s to read\n",outputFileName);
		exit(EXIT_FAILURE);
		};
		
	int ptr=0;
	for (int i=0;i<nBest[tot_bl];i++)
		{
		char c;
		for (int j=0;j<bestLen[tot_bl];j++)
			{
			fscanf(fp,"%c",&c);
			bestStrings[tot_bl][ptr++] = c-'0';
			};
		fscanf(fp,"%c",&c);
		};
	fclose(fp);
	};

return 0;
}

// this function recursively fills the string

void fillStr(int pos, int pfound, int partNum, int leftPerm)
{
int j1, newperm;
int tperm;
int alreadyWasted = pos - pfound - n + 1;	//	Number of character wasted so far
int spareW = tot_bl - alreadyWasted;		//	Maximum number of further characters we can waste while not exceeding tot_bl

//	If we can only match the current max_perm by using an optimal string for our remaining quota of wasted characters,
//	we try using those strings (swapping digits to make them start from the permutation we just visited).


if	(leftPerm && spareW > 0 && spareW < tot_bl && mperm_res[spareW] + pfound - 1 == max_perm)
	{
	for (int i=0;i<nBest[spareW];i++)
		{
		int len = bestLen[spareW];
		int *bestStr = bestStrings[spareW] + i*len;
		char *swap = curstr + pos - n - 1;
		fillStr2(pos,pfound,partNum,swap,bestStr+n,len-n);
		};
	return;
	};

for	(j1=1; j1<=n; j1++)		//	Loop to try to each possible next character we could append
	{
	// there is never any benefit to having 2 of the same character next to each other
	
	if	(j1 != curstr[pos-1])
		{
		curstr[pos] = j1;
		tperm = partNum + nfactor*j1;

		// Check to see if this contributes a new permutation or not
		
		newperm = valid[tperm] && unvisited[tperm];

		// now go to the next level of the recursion
		
		if (newperm)
			{
			if (pfound+1>max_perm)
				{
				writeCurrentString(TRUE,pos+1);
				nBest[tot_bl]=1;
				bestLen[tot_bl]=pos+1;
				max_perm = pfound+1;
				}
			else if (pfound+1==max_perm)
				{
				writeCurrentString(FALSE,pos+1);
				nBest[tot_bl]++;
				};

			unvisited[tperm]=FALSE;
			fillStr(pos+1, pfound+1, tperm/10, TRUE);
			unvisited[tperm]=TRUE;

		// the quantity alreadyWasted = pos - pfound - n + 1 is the number of already-used blanks
		
			}
		else if	(alreadyWasted < tot_bl)
			{
			if	(mperm_res[tot_bl - (alreadyWasted+1)] + pfound >= max_perm)
				{
				fillStr(pos+1, pfound, tperm/10, valid[tperm]);
				};
			};
		};
	};
}

//	Version that fills in the string when we are following a previously computed best string
//	rather than trying all digits.

void fillStr2(int pos, int pfound, int partNum, char *swap, int *bestStr, int len)
{
if (len<=0) return;		//	No more digits left in the template we are following

int j1, newperm;
int tperm;
int alreadyWasted = pos - pfound - n + 1;

j1 = swap[*bestStr];	//	Get the next digit from the template, swapped to make it start at our chosen permutation

// there is never any benefit to having 2 of the same character next to each other
	
if	(j1 != curstr[pos-1])
	{
	curstr[pos] = j1;
	tperm = partNum + nfactor*j1;

	// Check to see if this contributes a new permutation or not
		
	newperm = valid[tperm] && unvisited[tperm];

	// now go to the next level of the recursion
	
	if (newperm)
		{
		if (pfound+1>max_perm)
			{
			printf("Reached a point in the code that should be impossible!\n");
			exit(EXIT_FAILURE);
			}
		else if (pfound+1==max_perm)
			{
			writeCurrentString(FALSE,pos+1);
			nBest[tot_bl]++;
			};

		unvisited[tperm]=FALSE;
		fillStr2(pos+1, pfound+1, tperm/10, swap, bestStr+1, len-1);
		unvisited[tperm]=TRUE;

	// the quantity alreadyWasted = pos - pfound - n + 1 is the number of already-used blanks
	
		}
	else if	(alreadyWasted < tot_bl)
		{
		if	(mperm_res[tot_bl - (alreadyWasted+1)] + pfound >= max_perm)
			{
			fillStr2(pos+1, pfound, tperm/10, swap, bestStr+1, len-1);
			};
		};
	};
}

// this function computes the factorial of a number

int fac(int k)
{
if (k <= 1) return 1;
else return k*fac(k-1);
}

//	Generate all permutations of 1 ... n;
//	generates lists for lower values of n
//	in the process.
//
//	We sort each table into lexical order after constructing it.

void makePerms(int n, int **permTab)
{
int fn=fac(n);
int size=n*fn;
int *res;

CHECK_MEM( res=(int *)malloc(size*sizeof(int)) )

if (n==1)
	{
	res[0]=1;
	}
else
	{
	makePerms(n-1, permTab-1);
	int *prev=*(permTab-1);
	int p=0;
	int pf=fac(n-1);
	for (int k=0;k<pf;k++)
		{
		for (int q=n-1;q>=0;q--)
			{
			int r=0;
			for (int s=0;s<n;s++)
				{
				if (s==q) res[p++]=n;
				else res[p++]=prev[k*(n-1)+(r++)];
				};
			};
		};
	};

*permTab = res;
}

void writeCurrentString(int newFile, int size)
{
FILE *fp;
fp = fopen(outputFileName,newFile?"wa":"aa");
if (fp==NULL)
	{
	printf("Unable to open file %s to %s\n",outputFileName,newFile?"write":"append");
	exit(EXIT_FAILURE);
	};
for (int k=0;k<size;k++) fprintf(fp,"%c",'0'+curstr[k]);
fprintf(fp,"\n");
fclose(fp);
}
