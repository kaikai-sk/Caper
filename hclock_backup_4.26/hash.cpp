//**************************************
// Name: Hash table functions
// Description:Hash table management by Jerry Coffin, with improvements by HenkJan Wolthuis.
// By: Bob Stout (republished under Open Content License)
//
//
// Inputs:None
//
// Returns:None
//
//Assumes:None
//
//Side Effects:None
//This code is copyrighted and has limited warranties.
//Please see http://www.Planet-Source-Code.com/xq/ASP/txtCodeId.710/lngWId.3/qx/vb/scripts/ShowCode.htm
//for details.
//**************************************

/*
** public domain code by Jerry Coffin, with improvements by HenkJan Wolthuis.
**
** Tested with Visual C 1.0 and Borland C 3.1.
** Compiles without warnings, and seems like it should be pretty
** portable.
*/
/*
** These are used in freeing a table. Perhaps I should code up
** something a little less grungy, but it works, so what the heck.
*/

#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PE(s) {printf(s); exit(1);}

using namespace std;

static void (*function)(void *) = (void (*)(void *))NULL;
static hash_table *the_table = NULL;
/* Initialize the hash_table to the size asked for. Allocates space
** for the correct number of pointers and sets them to NULL. If it
** can't allocate sufficient memory, signals error by setting the size
** of the table to 0.
*/
hash_table *construct_table(hash_table *table, size_t size)
{
    size_t i;
    bucket **temp;
    table -> size = size;
    table -> table = (bucket * *)malloc(sizeof(bucket *) * size);
    if( !table -> table) PE("error: 1， 内存分配错误");
    temp = table -> table;
    if ( temp == NULL )
    {
        table -> size = 0;
        return table;
    }
    for (i=0;i<size;i++)
    temp[i] = NULL;
    return table;
}
/*
** Hashes a string to produce an unsigned short, which should be
** sufficient for most purposes.
*/

static unsigned old_hash(char *string)
{
    //printf("from--\n");
    unsigned ret_val = 0;
    char *p = string;
    int j;

    int i;
    int size = strlen(string);
    if (size < 4)
        printf("size < 4\n");
    for (j=0; j<=size-4; j++)
    {
        memcpy(&i, p, 4);
        //i = *( int *)string;
        //printf("%d\n", i);
        ret_val ^= i;
        ret_val <<= 1;
        p ++;
    }
    //printf("to--\n");
    return ret_val;
}

unsigned int hash ( const unsigned char *name )
{
      unsigned long  h = 0, g;
      char str[32];

      while ( *name )
      {
          h = ( h << 4 ) + *name++;
	  if ( g = h & 0xF0000000 )
	      h ^= g >> 24;
	  h &= ~g;
      }
      return h;
}

/*
** Insert 'key' into hash table.
** Returns pointer to old data associated with the key, if any, or
** NULL if the key wasn't in the table previously.
*/
void *insert(unsigned long key, void *data, hash_table *table)
{
    unsigned val = key % table->size;
    bucket *ptr;
    /*
    ** NULL means this bucket hasn't been used yet. We'll simply
    ** allocate space for our new bucket and put our data there, with
    ** the table pointing at it.
    */
    if (NULL == (table->table)[val])
    {
        (table->table)[val] = (bucket *)malloc(sizeof(bucket));
        if( !(table->table)[val] ) PE("error: 2， 内存分配错误");
        if (NULL==(table->table)[val])
        return NULL;
        (table->table)[val] -> key = key;
        (table->table)[val] -> next = NULL;
        (table->table)[val] -> data = data;
        return (table->table)[val] -> data;
    }
    /*
    ** This spot in the table is already in use. See if the current string
    ** has already been inserted, and if so, increment its count.
    */
    for (ptr = (table->table)[val];NULL != ptr; ptr = ptr -> next)
    if ( key == ptr->key)
    {
        void *old_data;
        old_data = ptr->data;
        ptr -> data = data;
        return old_data;
    }
    /*
    ** This key must not be in the table yet. We'll add it to the head of
    ** the list at this spot in the hash table. Speed would be
    ** slightly improved if the list was kept sorted instead. In this case,
    ** this code would be moved into the loop above, and the insertion would
    ** take place as soon as it was determined that the present key in the
    ** list was larger than this one.
    */
    ptr = (bucket *)malloc(sizeof(bucket));
    if( !ptr ) PE("error: 3， 内存分配错误");
    if (NULL==ptr)
    return 0;
    ptr -> key = key;
    ptr -> data = data;
    ptr -> next = (table->table)[val];
    (table->table)[val] = ptr;
    return data;
}
/*
** Look up a key and return the associated data. Returns NULL if
** the key is not in the table.
*/
void *lookup(unsigned long key, hash_table *table)
{
    unsigned val = key % table->size;
    bucket *ptr;
    if (NULL == (table->table)[val])
    return NULL;
    for ( ptr = (table->table)[val];NULL != ptr; ptr = ptr->next )
    {
        if ( key == ptr -> key  )
        return ptr->data;
    }
    return NULL;
}
/*
** Delete a key from the hash table and return associated
** data, or NULL if not present.
*/
void *del(unsigned long key, hash_table *table)
{
    unsigned val = key % table->size;
    void *data;
    bucket *ptr, *last = NULL;
    if (NULL == (table->table)[val])
    return NULL;
    /*
    ** Traverse the list, keeping track of the previous node in the list.
    ** When we find the node to delete, we set the previous node's next
    ** pointer to point to the node after ourself instead. We then delete
    ** the key from the present node, and return a pointer to the data it
    ** contains.
    */
    for (last = NULL, ptr = (table->table)[val];
    NULL != ptr;
    last = ptr, ptr = ptr->next)
    {
        if ( key == ptr -> key)
        {
            if (last != NULL )
            {
                data = ptr -> data;
                last -> next = ptr -> next;
                free(ptr);
                return data;
            }
            /*
            ** If 'last' still equals NULL, it means that we need to
            ** delete the first node in the list. This simply consists
            ** of putting our own 'next' pointer in the array holding
            ** the head of the list. We then dispose of the current
            ** node as above.
            */
            else
            {
                data = ptr->data;
                (table->table)[val] = ptr->next;
                free(ptr);
                return data;
            }
        }
    }
    /*
    ** If we get here, it means we didn't find the item in the table.
    ** Signal this by returning NULL.
    */
    return NULL;
}
/*
** free_table iterates the table, calling this repeatedly to free
** each individual node. This, in turn, calls one or two other
** functions - one to free the storage used for the key, the other
** passes a pointer to the data back to a function defined by the user,
** process the data as needed.
*/
static void free_node(unsigned long key, void *data)
{
    (void) data;
    if (function)
    function(del(key,the_table));
    else del(key,the_table);
}
/*
** Frees a complete table by iterating over it and freeing each node.
** the second parameter is the address of a function it will call with a
** pointer to the data associated with each node. This function is
** responsible for freeing the data, or doing whatever is needed with
** it.
*/
void free_table(hash_table *table, void (*func)(void *))
{
    function = func;
    the_table = table;
    enumerate( table, free_node);
    free(table->table);
    table->table = NULL;
    table->size = 0;
    the_table = NULL;
    function = (void (*)(void *))NULL;
}
/*
** Simply invokes the function given as the second parameter for each
** node in the table, passing it the key and the associated data.
*/

// old version
/*
void enumerate( hash_table *table, void (*func)(unsigned long, void *))
{
    unsigned i;
    bucket *temp;
    for (i=0;i<table->size; i++)
    {
        if ((table->table)[i] != NULL)
        {
            for (temp = (table->table)[i];
            NULL != temp;
            temp = temp -> next)
            {
                func(temp -> key, temp->data);
            }
        }
    }
}*/

void enumerate( hash_table *table, void (*func)(unsigned long, void *))
{
    unsigned i;
    bucket *temp, *temp2;
    for (i=0;i<table->size; i++)
    {
        if ((table->table)[i] != NULL)
        {
            for (temp = (table->table)[i]; NULL != temp; )
            {
                temp2 = temp -> next;
                func(temp -> key, temp->data);
                temp = temp2;
            }
        }
    }
}

