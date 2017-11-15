/*
   Copyright (C) CFEngine AS

   This file is part of CFEngine 3 - written and maintained by CFEngine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#define CF_BUFSIZE 4096
#define CF_UNDEFINED_ITEM (void *)0x1234
#define ProgrammingError printf

#include <assert.h>

#ifdef CYCLE_DETECTION
/* While looping over entry lp in an Item list, advance slow half as
 * fast as lp; let n be the number of steps it has fallen behind; this
 * increases by one every second time round the loop.  If there's a
 * cycle of length M, lp shall run round and round it; once slow gets
 * into the loop, they shall be n % M steps apart; at most 2*M more
 * times round the loop and n % M shall be 0 so lp == slow.  If the
 * lead-in to the loop is of length L, this takes at most 2*(L+M)
 * turns round the loop to discover the cycle.  The added cost is O(1)
 * per time round the loop, so the typical O(list length) user doesn't
 * change order when this is enabled, albeit the constant of
 * proportionality is up.
 *
 * Note, however, that none of this works if you're messing with the
 * structure (e.g. reversing or deleting) of the list as you go.
 *
 * To use the macros: before the loop, declare and initialize your
 * loop variable; pass it as lp to CYCLE_DECLARE(), followed by two
 * names not in use in your code.  Then, in the body of the loop,
 * after advancing the loop variable, CYCLE_CHECK() the same three
 * parameters.  This is apt to require a while loop where you might
 * otherwise have used a for loop; you also need to make sure your
 * loop doesn't continue past the checking.  When you compile with
 * CYCLE_DETECTION defined, your function shall catch cycles, raising
 * a ProgrammingError() if it sees one.
 */
#define CYCLE_DECLARE(lp, slow, toggle) \
    const Item *slow = lp; bool toggle = false
#define CYCLE_VERIFY(lp, slow) if (!lp) { /* skip */ }              \
    else if (!slow) ProgrammingError("Loop-detector bug :-(");      \
    else if (lp == slow) ProgrammingError("Found loop in Item list")
#define CYCLE_CHECK(lp, slow, toggle) \
    CYCLE_VERIFY(lp, slow);                                     \
    if (toggle) { slow = slow->next; CYCLE_VERIFY(lp, slow); }  \
    toggle = !toggle
#else
#define CYCLE_DECLARE(lp, slow, toggle) /* skip */
#define CYCLE_CHECK(lp, slow, toggle) /* skip */
#endif

#ifndef NDEBUG
/* Only intended for use in assertions.  Note that its cost is O(list
 * length), so you don't want to call it inside a loop over the
 * list. */

static bool ItemIsInList(const Item *list, const Item *item)
{
    CYCLE_DECLARE(list, slow, toggle);
    while (list)
    {
        if (list == item)
        {
            return true;
        }
        list = list->next;
        CYCLE_CHECK(list, slow, toggle);
    }
    return false;
}
#endif /* NDEBUG */

/*******************************************************************/

Item *ReverseItemList(Item *list)
{
    /* TODO: cycle-detection, which is somewhat harder here, without
     * turning this into a quadratic-cost function, albeit only when
     * assert() is enabled.
     */
    Item *tail = NULL;
    while (list)
    {
        Item *here = list;
        list = here->next;
        /* assert(!ItemIsInList(here, list)); // quadratic cost */
        here->next = tail;
        tail = here;
    }
    return tail;
}

/*********************************************************************/

int ItemListSize(const Item *list)
{
    int size = 0;
    const Item *ip = list;
    CYCLE_DECLARE(ip, slow, toggle);

    while (ip != NULL)
    {
        if (ip->name)
        {
            size += strlen(ip->name);
        }
        ip = ip->next;
        CYCLE_CHECK(ip, slow, toggle);
    }

    return size;
}

/*********************************************************************/

Item *ReturnItemIn(Item *list, const char *item)
{
    if (item == NULL || item[0] == '\0')
    {
        return NULL;
    }

    Item *ptr = list;
    CYCLE_DECLARE(ptr, slow, toggle);
    while (ptr != NULL)
    {
    if (ptr->name && (strcmp(ptr->name, item) == 0))
        {
            return ptr;
        }
        ptr = ptr->next;
        CYCLE_CHECK(ptr, slow, toggle);
    }

    return NULL;
}

/*********************************************************************/

Item *ReturnItemInClass(Item *list, const char *item, const char *classes)
{
    if (item == NULL || item[0] == '\0')
    {
        return NULL;
    }

    Item *ptr = list;
    CYCLE_DECLARE(ptr, slow, toggle);
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, item) == 0 &&
            strcmp(ptr->classes, classes) == 0)
        {
            return ptr;
        }
        ptr = ptr->next;
        CYCLE_CHECK(ptr, slow, toggle);
    }

    return NULL;
}

/*********************************************************************/

Item *ReturnItemAtIndex(Item *list, int index)
{
    Item *ptr = list;
    int i;
    
    for (i = 0; ptr != NULL && i < index; i++)
    {
        ptr = ptr->next;
    }

    return ptr;
}

/*********************************************************************/

bool IsItemIn(const Item *list, const char *item)
{
    if (item == NULL || item[0] == '\0')
    {
        return true;
    }

    const Item *ptr = list;
    CYCLE_DECLARE(ptr, slow, toggle);
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, item) == 0)
        {
            return true;
        }
        ptr = ptr->next;
        CYCLE_CHECK(ptr, slow, toggle);
    }

    return false;
}

/*********************************************************************/
/* True precisely if the lists are of equal length and every entry of
 * the first appears in the second.  As long as each list is known to
 * have no duplication of its entries, this is equivalent to testing
 * they have the same set of entries (ignoring order).
 *
 * This is not, in general, the same as the lists being equal !  They
 * may have the same entries in different orders.  If the first list
 * has some duplicate entries, the second list can have some entries
 * not in the first, yet compare equal.  Two lists with the same set
 * of entries but with different multiplicities are equal or different
 * precisely if of equal length.
 */

bool ListsCompare(const Item *list1, const Item *list2)
{
    if (ListLen(list1) != ListLen(list2))
    {
        return false;
    }

    const Item *ptr = list1;
    CYCLE_DECLARE(ptr, slow, toggle);
    while (ptr != NULL)
    {
        if (IsItemIn(list2, ptr->name) == false)
        {
            return false;
        }
        ptr = ptr->next;
        CYCLE_CHECK(ptr, slow, toggle);
    }

    return true;
}

/**
 * Checks whether list1 is a subset of list2, i.e. every entry in list1 must
 * be found in list2.
 */
bool ListSubsetOfList(const Item *list1, const Item *list2)
{
    const Item *list1_ptr = list1;
    CYCLE_DECLARE(list1_ptr, slow, toggle);

    while (list1_ptr != NULL)
    {
        if (!IsItemIn(list2, list1_ptr->name))
        {
            return false;
        }

        list1_ptr = list1_ptr->next;
        CYCLE_CHECK(list1_ptr, slow, toggle);
    }

    return true;               /* all elements of list1 were found in list2 */
}

/*********************************************************************/

Item *EndOfList(Item *ip)
{
    Item *prev = CF_UNDEFINED_ITEM;

    CYCLE_DECLARE(ip, slow, toggle);
    while (ip != NULL)
    {
        prev = ip;
        ip = ip->next;
        CYCLE_CHECK(ip, slow, toggle);
    }

    return prev;
}

/*********************************************************************/

Item *IdempPrependItem(Item **liststart, const char *itemstring, const char *classes)
{
    Item *ip = ReturnItemIn(*liststart, itemstring);
    if (ip)
    {
        return ip;
    }

    PrependItem(liststart, itemstring, classes);
    return *liststart;
}

/*********************************************************************/

Item *IdempPrependItemClass(Item **liststart, const char *itemstring, const char *classes)
{
    Item *ip = ReturnItemInClass(*liststart, itemstring, classes);
    if (ip)                     // already exists
    {
        return ip;
    }

    PrependItem(liststart, itemstring, classes);
    return *liststart;
}

/*********************************************************************/

void IdempItemCount(Item **liststart, const char *itemstring, const char *classes)
{
    Item *ip = ReturnItemIn(*liststart, itemstring);

    if (ip)
    {
        ip->counter++;
    }
    else
    {
        PrependItem(liststart, itemstring, classes);
    }

// counter+1 is the histogram of occurrences
}

/*********************************************************************/

Item *PrependItem(Item **liststart, const char *itemstring, const char *classes)
{
    Item *ip = calloc(1, sizeof(Item));

    ip->name = strdup(itemstring);
    if (classes != NULL)
    {
        ip->classes = strdup(classes);
    }

    ip->next = *liststart;
    *liststart = ip;

    return ip;
}

/*********************************************************************/

void PrependFullItem(Item **liststart, const char *itemstring, const char *classes, int counter, time_t t)
{
    Item *ip = calloc(1, sizeof(Item));

    ip->name = strdup(itemstring);
    ip->next = *liststart;
    ip->counter = counter;
    ip->time = t;
    if (classes != NULL)
    {
        ip->classes = strdup(classes);
    }

    *liststart = ip;
}

/*********************************************************************/
/* Warning: doing this a lot incurs quadratic costs, as we have to run
 * to the end of the list each time.  If you're building long lists,
 * it is usually better to build the list with PrependItemList() and
 * then use ReverseItemList() to get the entries in the order you
 * wanted; for modest-sized n, 2*n < n*n, even after you've applied
 * different fixed scalings to the two sides.
 */

void AppendItem(Item **liststart, const char *itemstring, const char *classes)
{
    Item *ip = calloc(1, sizeof(Item));

    ip->name = strdup(itemstring);
    if (classes)
    {
        ip->classes = strdup(classes); /* unused now */
    }

    if (*liststart == NULL)
    {
        *liststart = ip;
    }
    else
    {
        Item *lp = EndOfList(*liststart);
        assert(lp != CF_UNDEFINED_ITEM);
        lp->next = ip;
    }
}

/*********************************************************************/

void PrependItemList(Item **liststart, const char *itemstring)
{
    Item *ip = calloc(1, sizeof(Item));
    ip->name = strdup(itemstring);

    ip->next = *liststart;
    *liststart = ip;
}

/*********************************************************************/

int ListLen(const Item *list)
{
    int count = 0;
    const Item *ip = list;
    CYCLE_DECLARE(ip, slow, toggle);

    while (ip != NULL)
    {
        count++;
        ip = ip->next;
        CYCLE_CHECK(ip, slow, toggle);
    }

    return count;
}

/***************************************************************************/

void CopyList(Item **dest, const Item *source)
/* Copy a list. */
{
    if (*dest != NULL)
    {
        ProgrammingError("CopyList - list not initialized");
    }

    if (source == NULL)
    {
        return;
    }

    const Item *ip = source;
    CYCLE_DECLARE(ip, slow, toggle);
    Item *backwards = NULL;
    while (ip != NULL)
    {
        PrependFullItem(&backwards, ip->name,
                        ip->classes, ip->counter, ip->time);
        ip = ip->next;
        CYCLE_CHECK(ip, slow, toggle);
    }
    *dest = ReverseItemList(backwards);
}

/*********************************************************************/

Item *ConcatLists(Item *list1, Item *list2)
/* Notes: * Refrain from freeing list2 after using ConcatLists
          * list1 must have at least one element in it */
{
    if (list1 == NULL)
    {
        ProgrammingError("ConcatLists: first argument must have at least one element");
    }
    Item *tail = EndOfList(list1);
    assert(tail != CF_UNDEFINED_ITEM);
    assert(tail->next == NULL);
    /* If any entry in list1 is in list2, so is tail; so this is a
     * sufficient check that we're not creating a loop: */
    assert(!ItemIsInList(list2, tail));
    tail->next = list2;
    return list1;
}

void InsertAfter(Item **filestart, Item *ptr, const char *string)
{
    if (*filestart == NULL || ptr == CF_UNDEFINED_ITEM)
    {
        AppendItem(filestart, string, NULL);
        return;
    }

    if (ptr == NULL)
    {
        AppendItem(filestart, string, NULL);
        return;
    }

    Item *ip = calloc(1, sizeof(Item));

    ip->next = ptr->next;
    ptr->next = ip;
    ip->name = strdup(string);
    ip->classes = NULL;
}

Item *SplitString(const char *string, char sep)
 /* Splits a string containing a separator like :
    into a linked list of separate items, */
{
    Item *liststart = NULL;
    const char *sp = string;
    char before[CF_BUFSIZE];
    int i = 0;

    while (*sp != '\0')
    {
        if (*sp != sep)
        {
            before[i] = *sp;
            i++;
        }
        else if (sp > string && sp[-1] == '\\')
        {
            /* Escaped use of list separator; over-write the backslash
             * we copied last time round the loop (and don't increment
             * i, so next time round we'll continue in the right
             * place). */
            before[i - 1] = sep;
        }
        else
        {
            before[i] = '\0';
            PrependItem(&liststart, before, NULL);
            i = 0;
        }

        sp++;
    }

    before[i] = '\0';
    PrependItem(&liststart, before, "");

    return ReverseItemList(liststart);
}

/*********************************************************************/

Item *SplitStringAsItemList(char *string, char sep)
 /* Splits a string containing a separator like :
    into a linked list of separate items, */
{
    Item *liststart = NULL;
    char node[256];
    char format[] = "%255[^\0]";
    char *sp;
    
    /* Overwrite format's internal \0 with sep: */
    format[strlen(format)] = sep;
    assert(strlen(format) + 1 == sizeof(format) || sep == '\0');

    for (sp = string; *sp != '\0'; sp++)
    {
        if (sscanf(sp, format, node) == 1 &&
            node[0] != '\0')
        {
            sp += strlen(node) - 1;
            PrependItem(&liststart, node, NULL);
        }
    }

    return ReverseItemList(liststart);
}

/*********************************************************************/
/* Basic operations                                                  */
/*********************************************************************/

void IncrementItemListCounter(Item *list, const char *item)
{
    if (item == NULL || item[0] == '\0')
    {
        return;
    }

    Item *ptr = list;
    CYCLE_DECLARE(ptr, slow, toggle);
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, item) == 0)
        {
            ptr->counter++;
            return;
        }
        ptr = ptr->next;
        CYCLE_CHECK(ptr, slow, toggle);
    }
}

/*********************************************************************/

void SetItemListCounter(Item *list, const char *item, int value)
{
    if (item == NULL || item[0] == '\0')
    {
        return;
    }

    Item *ptr = list;
    CYCLE_DECLARE(ptr, slow, toggle);
    while (ptr != NULL)
    {
        if (strcmp(ptr->name, item) == 0)
        {
            ptr->counter = value;
            return;
        }
        ptr = ptr->next;
        CYCLE_CHECK(ptr, slow, toggle);
    }
}

/*********************************************************************/
/* Cycle-detection: you'll get a double free if there's a cycle. */

void DeleteItemList(Item *item) /* delete starting from item */
{
    while (item != NULL)
    {
        Item *here = item;
        item = here->next; /* before free()ing here */

        free(here->name);
        free(here->classes);
        free(here);
    }
}

/*********************************************************************/

void DeleteItem(Item **liststart, Item *item)
{
    if (item != NULL)
    {
        if (item == *liststart)
        {
            *liststart = item->next;
        }
        else
        {
            Item *ip = *liststart;
            CYCLE_DECLARE(ip, slow, toggle);
            while (ip && ip->next != item)
            {
                ip = ip->next;
                CYCLE_CHECK(ip, slow, toggle);
            }

            if (ip != NULL)
            {
                assert(ip->next == item);
                ip->next = item->next;
            }
        }

        free(item->name);
        free(item->classes);
        free(item);
    }
}

/*********************************************************************/

/* DeleteItem* function notes:
 * -They all take an item list and an item specification ("string" argument.)
 * -Some of them treat the item spec as a literal string, while others
 *  treat it as a regular expression.
 * -They all delete the first item meeting their criteria, as below.
 *  function   deletes item
 *  ------------------------------------------------------------------------
 *  DeleteItemStarting  start is literally equal to string item spec
 *  DeleteItemLiteral  literally equal to string item spec
 *  DeleteItemMatching  fully matched by regex item spec
 *  DeleteItemContaining containing string item spec
 */

/*********************************************************************/

void DeleteItemLiteral(Item **liststart, char *name)
{
 Item *ip = *liststart;

 if (strcmp(ip->name,name) == 0)
    {
    *liststart = ip->next;
    free(ip->name);
    free(ip->classes);
    free(ip);
    return;
    }

 Item  *prev = *liststart;
 
 for (ip = (*liststart)->next; ip != NULL; ip=ip->next)
    {
    if (strcmp(ip->name,name) == 0)
       {
       prev->next = ip->next;
       free(ip->name);
       free(ip->classes);
       free(ip);
       return;
       }
    
    prev = ip;
    }
}
