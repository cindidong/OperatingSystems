// NAME: Cindi Dong
// EMAIL: cindidong@gmail.com
// ID: 405126747

#include "SortedList.h"
#include <string.h>
#include <pthread.h>

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *    The specified element will be inserted in to
 *    the specified list, which will be kept sorted
 *    in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
  SortedListElement_t *temp = list->next;
  //if current key is less than element key
  while(temp != list && strcmp(temp->key, element->key) < 0)
    {
      temp = temp->next;
    }
  //yield after finding the proper placement of the element so another thread may run and change the list
  if (opt_yield & INSERT_YIELD)
    {
      sched_yield();
    }
  //check for bad pointers
  if (temp->prev->next != temp || temp->next->prev != temp)
    {
      return;
    }
  //need to insert in front of current node
  element->prev = temp->prev;
  element->next = temp;
  temp->prev->next = element;
  temp->prev = element;
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *    The specified element will be removed from whatever
 *    list it is currently in.
 *
 *    Before doing the deletion, we check to make sure that
 *    next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete(SortedListElement_t *element)
{
  //if the element does not exist
  if (element == NULL)
    {
      return 1;
    }
  //check if head
  if (element->key == NULL)
    {
      return 1;
    }
  //check pointers
  if (element->prev->next == element && element->next->prev == element)
    {
      element->prev->next = element->next;
      //yield to give another thread a chance to edit the list before finishing delete
      if (opt_yield & DELETE_YIELD)
        {
	  sched_yield();
        }
      element->next->prev = element->prev;
      element->prev = NULL;
      element->next = NULL;
      return 0;
    }
  return 1;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *    The specified list will be searched for an
 *    element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
  //if head
  if (key == NULL)
    {
      return NULL;
    }
  SortedListElement_t *temp = list->next;
  //while current key is less than or equal to the key
  while (temp != list && strcmp(temp->key, key) <= 0)
    {
      //yield after finding the element with the key
      if (opt_yield & LOOKUP_YIELD)
        {
	  sched_yield();
        }
      if (strcmp(temp->key, key) == 0)
        {
	  return temp;
        }
      temp = temp->next;
    }
  return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *    While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *       -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
  int count = 0;
  //if list does not exist
  if (list == NULL)
    {
      return -1;
    }
  SortedListElement_t *temp = list->next;
  while (temp != list)
    {
      //check pointers
      if (temp->next->prev != temp)
        {
	  return -1;
        }
      count++;
      temp = temp->next;
      //yield so that another thread may insert or delete elements (possible skipping of a node)
      if (opt_yield & LOOKUP_YIELD)
        {
	  sched_yield();
        }
    }
  return count;
}
