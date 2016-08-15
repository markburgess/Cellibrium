/*

   Copyright (C) Mark Burgess

   gcc -g uTrans.c -lpcre

   Universal Translator
   
*/

#include <stdio.h>
#include <string.h>
#include <pcre.h>      
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

typedef enum
{
    false,
    true
} bool;

/*******************************************************************/
/* GLOBAL VARIABLES                                                */
/*******************************************************************/

#define OVECCOUNT 30
#define KM_BUFSIZE 4096
#define ITEMTYPES 14

/*******************************************************************/

enum cbrm_markers
{
    cbrm_unknown,
    cbrm_begin,
    cbrm_end,
    cbrm_item,
    cbrm_body
};

/*******************************************************************/

typedef struct 
{
   char *pattern;
   enum cbrm_markers type;
   
} Tokens;

/*******************************************************************/

Tokens PARENTHESES[ITEMTYPES+1] =  // if A is a substring of B, then B must come before A in this list
{
    { "<([^<>/]+)>", cbrm_begin },  // Remember regex matching is greedy - longest viable match
    {  "</([^<>]+)>", cbrm_end },
    {  "\\{", cbrm_begin },
    {  "\\}", cbrm_end },
    {  "\\[", cbrm_begin },
    {  "\\]", cbrm_end },
    { "\\(", cbrm_begin },
    {  "\\)", cbrm_end },
    { NULL,  cbrm_unknown },
};

Tokens ITEMS[3] =  // if A is a substring of B, then B must come before A in this list
{
    { "\\\"([^\\\"])*\\\"", cbrm_item }, // qstring
    { "[^ \\s,;]+", cbrm_item },
    { NULL,  cbrm_unknown },
};


/* It is really hard to protect these regular expressions against
     mismatches */

/*******************************************************************/

#define PMMAXDIMS 1024
#define KM_MAXVARSIZE 1024

typedef struct 
{
   int dimspan;
   int  *coords;     // the numerical depth
   char **delimiter; // the type of parenthesis
   char **cnames;    // a canonical name for each dimension (different from delimiter?)
   int values;       // The number of values
   char **value;     // the value at these spacetime coords
} Tuples;

typedef struct 
{
   int segments;
   char **segment; // array of matches
   enum cbrm_markers *segtype; // array of begin, end, item 
   int *segtoken; // array of patterns, corresponding to patterns
   int level;
   int maxp;
   int maxn;
   Tuples *coordinates;
   
} SpanningTree;

/*******************************************************************/

SpanningTree *ParseInput(char *f);
int FindFirstMarker(Tokens *list, char *input, char **token, int *tokennumber,  int *tokentype, int *start, int *end);
void HandleToken(SpanningTree *stree, char *token, char *name, int index, int id, int start, int end, int seg);
void HandleBody(SpanningTree *stree, char *text, char *token, int index, int id, int pos, int seg);
void HandleItem(SpanningTree *stree, int seg, char *token, char *name, int id, int type, int start, int end, int item);
void PrintCoords(SpanningTree *stree,int segment);
void MarkBodyCoords(SpanningTree *stree, int *current_coords, char **current_types, int dimspan, int segment);
void MapSegment(SpanningTree *stree, int sg);
void PrintSegmentStream(SpanningTree *stree);

char *FirstBackReference(pcre *rx, const char *teststring);
char *ExtractFirstReference(const char *regexp, const char *teststring);
pcre *CompileRegex(const char *regex);
bool StringMatchWithPrecompiledRegex(pcre *regex, const char *str, int *start, int *end);
bool StringMatch(const char *regex, const char *str, int *start, int *end);
bool StringMatchFull(const char *regex, const char *str);
bool StringMatchFullWithPrecompiledRegex(pcre *pattern, const char *str);
bool CompareStringOrRegex(const char *value, const char *compareTo, bool regex);


const int INDENT = 3;
char INPUTSTREAM[KM_BUFSIZE];

/*********************************************************/
// MAIN
/*********************************************************/

int main(int argc, char *argv[])
{
  printf("Universal translator...\n");

  SpanningTree *stree;
   
  stree = ParseInput(argv[1]);
  PrintSegmentStream(stree);
}

/*********************************************************/

SpanningTree *ParseInput(char *filename)
{
 char *input;
 char *token;
 struct stat sb;
 FILE *fp;
 SpanningTree *stree = malloc(sizeof(SpanningTree));

 if (stat(filename, &sb) == -1)
    {
    perror("stat");
    exit(0);
    }

 input = malloc(sb.st_size+1);

 if ((fp = fopen(filename, "r")) == NULL)
    {
    perror("fopen");
    exit(0);
    }
 
 fread(input, sb.st_size+1, 1, fp);

 fclose(fp);
 
 // Init

 int start = -1, end = -1;

 stree->segment = malloc(sizeof(char *)*sb.st_size);
 stree->segtype = malloc(sizeof(int)*sb.st_size);
 stree->segtoken = malloc(sizeof(int)*sb.st_size);
 stree->coordinates = malloc(sizeof(Tuples)*sb.st_size);
 stree->level = 0;
 stree->maxp = 0;
 stree->maxn = 0;
 stree->segments = 0;
 
 int i,j;

 for (i = 0; i < sb.st_size; i++)
    {
    stree->segtype[i] = cbrm_unknown;
    stree->segment[i] = "";
    }
 
 // Probe sensor(s) and segment stream

 char *sp = input;
 int pos = 0;
 int tokenid = 0;
 int tokentype = cbrm_unknown;
 
 while (*sp != '\0')
    {
    pos = FindFirstMarker(PARENTHESES, sp, &token, &tokenid, &tokentype, &start, &end);

    if (pos > 0) // Token is ahead, so we are in the body
       {
       HandleBody(stree, sp, token, tokenid, tokentype, pos, stree->segments);
       sp += pos;
       stree->segments++;
       }
    else
       {
       if (pos < 0)
          {
          start = 0;
          end = sb.st_size;
          tokentype = cbrm_item;
          }

       HandleToken(stree, sp, token, tokenid, tokentype, start, end, stree->segments);
       sp += end-start;
       stree->segments++;
       }
    }

 printf("-----------------------------------------------------\n");
 printf("Stream has %d segments\n", stree->segments);
 printf("Stream has %d (+) dimensions\n", stree->maxp);
 printf("Stream has %d (-) dimensions\n", stree->maxn);
 if (stree->level != 0)
    {
    printf("The stream has inconsistent dimensionality - is not a closed space\n");
    }

 int dimspan = stree->maxp - stree->maxn + 1;
 int start_offset = - stree->maxn;

 printf("Best guess dimensionality span = %d (origin = %d)\n", dimspan, start_offset);

 printf(" - Numerical coordinates are reliable but relative and hard to understand \n"
        " - how can we extract a meaningful name for a path reference in a general file/schema\n");
 
 printf("-----------------------------------------------------\n"); 

 // Now coordinatize the segments
 
 int *current_coords = malloc(sizeof(int)*dimspan); // make ( , , , , ,)
 char **current_types = malloc(sizeof(char *)*dimspan); // make ( , , , , ,)

 int dimpos = start_offset;
 
 for (j = 0; j < dimspan; j++)
    {
    current_coords[j] = 0;
    current_types[j] = strdup("");
    }
    
 for (i = 0; i < stree->segments; i++)
    {
    switch (stree->segtype[i])
       {
       case cbrm_item:
       case cbrm_body:
           current_coords[dimpos]++;
           MarkBodyCoords(stree, current_coords, current_types, dimspan, i);
           break;
           
       case cbrm_begin:
           current_coords[dimpos]++;
           dimpos++;
           current_types[dimpos] = strdup(stree->segment[i]);
           break;
           
       case cbrm_end:
           for (j = dimpos; j < dimspan; j++)
              {
              current_coords[j] = 0;
              current_types[j] = strdup("");
              }
           dimpos--;
           break;

       default:
           break;
       }
    }


//***********************************
// Now dissect the bodies, mehuhuhuh!!

 for (i = 0; i < stree->segments; i++)
    {
    switch (stree->segtype[i])
       {
       case cbrm_item:
       case cbrm_body:
           MapSegment(stree,i);
       default:
           break;

       }
    }

 return stree;
}

/*********************************************************/

void PrintSegmentStream(SpanningTree *stree)
{
 int i;

 for (i = 0; i < stree->segments; i++)
    {
    switch (stree->segtype[i])
       {
       case cbrm_item:
       case cbrm_body:
           PrintCoords(stree, i);
       default:
           break;
       }
    }
}

/*********************************************************/

int FindFirstMarker(Tokens *tokenlist, char *input, char **token, int *tokennumber, int *tokentype, int *start, int *end)
{
 int first = -1, index = -1;
 int match = false;
 int i;
 
 struct TokenPattern
 {
    char *name;
    int start;
    int end;
 } matches[ITEMTYPES];
 
 for (i = 0; tokenlist[i].pattern != NULL; i++)
    {
    matches[i].start = -1;
    matches[i].end = -1;

    if (StringMatch(tokenlist[i].pattern, input, &(matches[i].start), &(matches[i].end)))
       {
       match = true;
       }
    }

 if (!match)
    {
    *start = 0;
    *end = 0;
    *tokennumber = 0;
    *tokentype = cbrm_item;
    return -1;
    }
 
  for (i = 0; tokenlist[i].pattern != NULL; i++)
     {
     if ((first == -1) && (matches[i].start >= 0))
        {
        first = matches[i].start;
        index = i;
        continue;
        }

     if ((matches[i].start >= 0) && (matches[i].start < first))
        {
        first = matches[i].start;
        index = i;
        continue;
        }
     }

  char *text = ExtractFirstReference(tokenlist[index].pattern, input);

  if (strcmp(text,"KM_NOMATCH") == 0)
     {
     *token = "";
     }
  else
     {
     *token = text;
     }

  *start =  matches[index].start;
  *end = matches[index].end;
  *tokennumber = index;
  *tokentype = tokenlist[index].type;
  return first;
}

/*********************************************************/

void HandleToken(SpanningTree *stree, char *token, char *name, int id, int type, int start, int end, int seg)
{
 stree->segment[seg] = strndup(token,end-start);
 stree->segtype[seg] = type;
 stree->segtoken[seg] = id;
 
 switch(stree->segtype[seg])
    {
    case cbrm_item:
        //printf("[ITEM:%s (name=%s)]\n", stree->segment[seg], name);
        break;
    case cbrm_begin:
        stree->level++;
        //printf("[BEGIN(%d):%s (name=%s)]\n", id, stree->segment[seg], name);
        break;
    case cbrm_end:
        stree->level--;
        //printf("[END(%d):%s (name=%s)]\n", id,stree->segment[seg], name);
        break;
    default:
        // shouldn't happen
        //printf("[UFO (=%d):%s (name=%s)]\n", stree->segtype[seg], stree->segment[seg], name);
        break;
    }

 if (stree->level > stree->maxp)
    {
    stree->maxp = stree->level;
    }

 if (stree->level < stree->maxn)
    {
    stree->maxn = stree->level;
    }
}

/*********************************************************/

void HandleBody(SpanningTree *stree, char *text, char *token, int id, int type, int pos, int seg)
{
 stree->segment[seg] = strndup(text,pos);
 stree->segtype[seg] = cbrm_body;
 stree->segtoken[seg] = id;
}

/*********************************************************/

void MarkBodyCoords(SpanningTree *stree, int *current_coords, char **current_types, int dimspan, int segment)
{
 int i;

 stree->coordinates[segment].coords = malloc(sizeof(int)*dimspan);
 stree->coordinates[segment].delimiter = malloc(sizeof(char *)*dimspan);
 stree->coordinates[segment].dimspan = dimspan;

 stree->coordinates[segment].values = 1024; // How do we determine this scale free?

 stree->coordinates[segment].value = malloc(sizeof(char *)*stree->coordinates[segment].values);
  
 for (i = 0; i < dimspan; i++)
    {
    stree->coordinates[segment].coords[i] = current_coords[i];
    stree->coordinates[segment].delimiter[i] = current_types[i];
    }

 for (i = 0; i < stree->coordinates[segment].values; i++)
    {
    stree->coordinates[segment].value[i] = NULL;
    }
}

/*********************************************************/

void PrintCoords(SpanningTree *stree, int segment)
{
 int i;

 printf("\nDimension/Region (");
 
 for (i = 0; i < stree->coordinates[segment].dimspan; i++)
    {
    printf(" %d%s, ", stree->coordinates[segment].coords[i],stree->coordinates[segment].delimiter[i]);
    }
 putchar(')');
 printf("  -> \n");

 for (i = 0; stree->coordinates[segment].value[i] != NULL; i++)
    {
    printf(" - path/proper time location[%d](%s)\n", i, stree->coordinates[segment].value[i]);
    }

}

/************************************************************************/

void MapSegment(SpanningTree *stree, int seg)
{
 int start = -1, end = -1; 
 char *sp = stree->segment[seg];
 int pos = 0;
 int item = 0;
 int tokenid = 0;
 int tokentype = cbrm_unknown;
 char *token;

 while (*sp != '\0')
    {
    pos = FindFirstMarker(ITEMS, sp, &token, &tokenid, &tokentype, &start, &end);

    if (pos < 0)
       {
       break;
       }
    
    if (pos > 0) // Token is ahead, so we are in the body
       {
       sp += pos;
       }
    else
       {
       HandleItem(stree, seg, sp, token, tokenid, tokentype, start, end, item);
       sp += end-start;
       item++;

       // check item > 1024
       }
    }

}

/*********************************************************/

void HandleItem(SpanningTree *stree, int seg, char *token, char *name, int id, int type, int start, int end, int item)
{
 stree->coordinates[seg].value[item] = strndup(token,end-start);
}

/************************************************************************/
/* Lib regex PCRE                                                       */
/************************************************************************/

char *FirstBackReference(pcre *rx, const char *teststring)
{
 static char backreference[KM_BUFSIZE]; /* GLOBAL_R, no initialization needed */
 
 int ovector[OVECCOUNT], i, rc;
 
 memset(backreference, 0, KM_BUFSIZE);
 
 if ((rc = pcre_exec(rx, NULL, teststring, strlen(teststring), 0, 0, ovector, OVECCOUNT)) >= 0)
    {
    for (i = 1; i < rc; i++)        /* make backref vars $(1),$(2) etc */
       {
       const char *backref_start = teststring + ovector[i * 2];
       int backref_len = ovector[i * 2 + 1] - ovector[i * 2];
       
       if (backref_len < KM_MAXVARSIZE)
          {
          strncpy(backreference, backref_start, backref_len);
          }
       
       break;
       }
    }
 
 free(rx);
 
 return backreference;
}

/************************************************************************/

char *ExtractFirstReference(const char *regexp, const char *teststring)
{
 char *backreference;
 
 pcre *rx;
 
 if ((regexp == NULL) || (teststring == NULL))
    {
    return "";
    }
 
 rx = CompileRegex(regexp);

 if (rx == NULL)
    {
    return "";
    }
 
 backreference = FirstBackReference(rx, teststring);
 
 if (strlen(backreference) == 0)
    {
    strncpy(backreference, "KM_NOMATCH", KM_MAXVARSIZE);
    }
 
 return backreference;
}


#define STRING_MATCH_OVECCOUNT 30
#define NULL_OR_EMPTY(str) ((str == NULL) || (str[0] == '\0'))

/**********************************************************************/

pcre *CompileRegex(const char *regex)
{
 const char *errorstr;
 int erroffset;
 
 pcre *rx = pcre_compile(regex, PCRE_MULTILINE | PCRE_DOTALL,
                         &errorstr, &erroffset, NULL);
 
 if (!rx)
    {
    printf("Failed compiling %s\n", regex);
    perror("1");
    /* Log(LOG_LEVEL_ERR,
       "Regular expression error: pcre_compile() '%s' in expression '%s' (offset: %d)",
       errorstr, regex, erroffset); */
    }
 
 return rx;
}

/**********************************************************************/

bool StringMatchWithPrecompiledRegex(pcre *regex, const char *str, int *start, int *end)
{
 assert(regex);
 assert(str);
 
 int ovector[STRING_MATCH_OVECCOUNT] = { 0 };
 int result = pcre_exec(regex, NULL, str, strlen(str), 0, 0, ovector, STRING_MATCH_OVECCOUNT);
 
 if (result)
    {
    if (start)
       {
       *start = ovector[0];
       }
    if (end)
       {
       *end = ovector[1];
       }
    }
 else
    {
    if (start)
       {
       *start = 0;
       }
    if (end)
       {
       *end = 0;
       }
    }
 
 return result >= 0;
}

/**********************************************************************/

bool StringMatch(const char *regex, const char *str, int *start, int *end)
{
 pcre *pattern = CompileRegex(regex);
 
 if (pattern == NULL)
    {
    return false;
    }
 
 bool ret = StringMatchWithPrecompiledRegex(pattern, str, start, end);
 
 pcre_free(pattern);
 return ret;
}

/**********************************************************************/

bool StringMatchFull(const char *regex, const char *str)
{
 pcre *pattern = CompileRegex(regex);
 
 if (pattern == NULL)
    {
    return false;
    }
 
 bool ret = StringMatchFullWithPrecompiledRegex(pattern, str);
 
 pcre_free(pattern);
 return ret;
}

/**********************************************************************/

bool StringMatchFullWithPrecompiledRegex(pcre *pattern, const char *str)
{
 int start = 0, end = 0;
 
 if (StringMatchWithPrecompiledRegex(pattern, str, &start, &end))
    {
    return (start == 0) && (end == strlen(str));
    }
 else
    {
    return false;
    }
}

/**********************************************************************/

bool CompareStringOrRegex(const char *value, const char *compareTo, bool regex)
{
 if (regex)
    {
    if (!NULL_OR_EMPTY(compareTo) && !StringMatchFull(compareTo, value))
       {
       return false;
       }
    }
 else
    {
    if (!NULL_OR_EMPTY(compareTo)  && strcmp(compareTo, value) != 0)
       {
       return false;
       }
    }
 return true;
}
