#include <stdio.h>
#include <stdlib.h>
#include "List.h"

List *list_alloc ();

/**
 * list_append:
 * @list: a #List
 * @data: the data for the new element
 *
 * Adds a new element on to the end of the list.
 *
 * Returns: the new start of the #List
 */
List *
list_append (
    List *list,
    pointer data) 
{
  List *new_list;
  List *last;

  new_list = list_alloc ();
  new_list->data = data;
  new_list->next = NULL;

  if (list) {
    last = list_last (list);
    last->next = new_list;
    new_list->prev = last;

    return list;
  }
  
  new_list->prev = NULL;
  return new_list;
}

List *
_list_remove_unlink (
    List *list,
    List *link)
{
  if (link == NULL)
    return list;

  if (link->prev) {
    link->prev->next = link->next;
  }

  if (link->next) {
    link->next->prev = link->prev;
  }

  if (link == list)
    list = list->next;

  link->next = NULL;
  link->prev = NULL;

  return list;
}

/**
 * list_remove:
 * @list: a #List
 * @data: the data of the element to remove
 *
 * Removes an element from a #List.
 *
 * Returns: the new start of the #List
 */
List *
list_remove (
    List *list,
    pointer data)
{
  List *tmp;

  tmp = list;
  while (tmp) {
    if (tmp->data != data) {
      tmp = tmp->next;
    }
    else {
      list = _list_remove_unlink(list, tmp);
      list_free (tmp);
      break;
    }
  }

  return list;
}

/**
 * list_last:
 * @list: a #List
 *
 * Gets the last element in a #List.
 *
 * Returns: the last element in the #List,
 *  or %NULL if the #List has no elements
 */
List *
list_last (
    List *list)
{
  if (list) {
    while (list->next)
      list = list->next;
  }

  return list;
}

/**
 * list_foreach:
 * @list: a #List
 * @func: the function to call with each element's data
 * @user_data: user data to pass to the function
 *
 * Calls a function for each element of a #List.
 */
void
list_foreach (
    List *list,
    Func func,
    pointer user_data)
{
  while (list) {
    List *next = list->next;
    (*func) (list->data, user_data);
    list = next;
  }
}

/**
 * list_alloc:
 *
 * Allocates space for one #List element.
 *
 * Returns: a pointer to the newly-allocated #List element.
 */
List *
list_alloc ()
{
  return malloc (sizeof (List));
}

/**
 * list_free:
 * @list: a #List
 *
 * Frees all of the memory used by a #List
 */
void
list_free (List *list)
{
  while (list) {
    List *next = list->next;
    free (list);
    list = next;
  }
}

