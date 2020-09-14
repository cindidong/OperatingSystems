// NAME: Cindi Dong
// EMAIL: cindidong@gmail.com
// ID: 405126747

#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "SortedList.h"

SortedList_t* heads;
SortedListElement_t* elements;

int iterations_number;
int m_lock_flag;
int s_lock_flag;
int i_yield;
int d_yield;
int l_yield;
int opt_yield;
int lists_flag;
int lists_number;
long long *lock_time;
pthread_mutex_t *locks;
int *spin_locks;

void handler()
{
  fprintf(stderr, "Segfault: Corrupted list\n");
  exit(2);
}

//hash function
int hash(const char* string)
{
  unsigned int i;
  int hash_value = 0;
  for (i = 0; i < strlen(string); i++)
    {
      hash_value += string[i];
    }
  return (hash_value % lists_number);
}

void* thread_func(void* position)
{
  int index = *((int*) position);
  int i;
  int length = 0;
  int delete_status = 0;
  struct timespec startTime, endTime;
  long long runtime = 0;
  int heads_index;
  int sublist_length = 0;
  //mutex lock
  if (m_lock_flag == 1)
    {
      //insert
      for (i = index; i < index + iterations_number; i++)
        {
	  //getting the index of the head
	  //if there is 1 list, there is only 1 head
	  if (lists_number == 1)
            {
	      heads_index = 0;
            }
	  else
            {
	      heads_index = hash(elements[i].key);
            }
	  if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
            {
	      fprintf(stderr, "Start clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  pthread_mutex_lock(&locks[heads_index]);
	  if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
            {
	      fprintf(stderr, "End clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  runtime += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
	  SortedList_insert(&heads[heads_index], &elements[i]);
	  pthread_mutex_unlock(&locks[heads_index]);
        }
      //get length
      for (i = 0; i < lists_number; i++)
        {
	  if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
            {
	      fprintf(stderr, "Start clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  pthread_mutex_lock(&locks[i]);
	  if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
            {
	      fprintf(stderr, "End clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  runtime += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
	  sublist_length = SortedList_length(&heads[i]);
	  pthread_mutex_unlock(&locks[i]);
	  //check each sublist's length
	  if (sublist_length < 0)
            {
	      fprintf(stderr, "Insert length error: Corrupted list\n");
	      exit(2);
            }
	  length = length + sublist_length;
        }
      //check overall length
      if (length < 0)
        {
	  fprintf(stderr, "Insert length error: Corrupted list\n");
	  exit(2);
        }
      //delete
      for (i = index; i < index + iterations_number; i++)
        {
	  //get index of head
	  if (lists_number == 1)
            {
	      heads_index = 0;
            }
	  else
            {
	      heads_index = hash(elements[i].key);
            }
	  if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
            {
	      fprintf(stderr, "Start clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  pthread_mutex_lock(&locks[heads_index]);
	  if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
            {
	      fprintf(stderr, "End clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  runtime += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
	  //look up element to delete
	  SortedListElement_t* temp = SortedList_lookup(&heads[heads_index], elements[i].key);
	  if (temp == NULL)
            {
	      fprintf(stderr, "Unable to find the element to delete error: Corrupted list\n");
	      pthread_mutex_unlock(&locks[heads_index]);
	      exit(2);
            }
	  delete_status = SortedList_delete(temp);
	  pthread_mutex_unlock(&locks[heads_index]);
	  if (delete_status > 0)
            {
	      fprintf(stderr, "List delete error: Corrupted list\n");
	      exit(2);
            }
        }
      //store lock wait time in lock_time with thread's index
      lock_time[index/iterations_number] = runtime;
    }
  //spin lock
  else if (s_lock_flag == 1)
    {
      //insert
      for (i = index; i < index + iterations_number; i++)
        {
	  //get index of head
	  if (lists_number == 1)
            {
	      heads_index = 0;
            }
	  else
            {
	      heads_index = hash(elements[i].key);
            }
	  if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
            {
	      fprintf(stderr, "Start clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  while (__sync_lock_test_and_set(&spin_locks[heads_index], 1))
            {
	      continue;
            }
	  if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
            {
	      fprintf(stderr, "End clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  runtime += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
	  SortedList_insert(&heads[heads_index], &elements[i]);
	  __sync_lock_release(&spin_locks[heads_index]);
        }
      //get length
      for (i = 0; i < lists_number; i++)
        {
	  if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
            {
	      fprintf(stderr, "Start clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  while (__sync_lock_test_and_set(&spin_locks[i], 1))
            {
	      continue;
            }
	  if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
            {
	      fprintf(stderr, "End clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  runtime += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
	  sublist_length = SortedList_length(&heads[i]);
	  __sync_lock_release(&spin_locks[i]);
	  //check sublist's length
	  if (sublist_length < 0)
            {
	      fprintf(stderr, "Insert length error: Corrupted list\n");
	      exit(2);
            }
	  length = length + sublist_length;
        }
      //check total length
      if (length < 0)
        {
	  fprintf(stderr, "Insert length error: Corrupted list\n");
	  exit(2);
        }
      //delete
      for (i = index; i < index + iterations_number; i++)
        {
	  //get index of head
	  if (lists_number == 1)
            {
	      heads_index = 0;
            }
	  else
            {
	      heads_index = hash(elements[i].key);
            }
	  if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
            {
	      fprintf(stderr, "Start clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  while (__sync_lock_test_and_set(&spin_locks[heads_index], 1))
            {
	      continue;
            }
	  if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
            {
	      fprintf(stderr, "End clock error: %s\n", strerror(errno));
	      exit(1);
            }
	  runtime += 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
	  //look up element to delete
	  SortedListElement_t* temp = SortedList_lookup(&heads[heads_index], elements[i].key);
	  if (temp == NULL)
            {
	      fprintf(stderr, "Unable to find the element to delete error: Corrupted list\n");
	      __sync_lock_release(&spin_locks[heads_index]);
	      exit(2);
            }
	  delete_status = SortedList_delete(temp);
	  __sync_lock_release(&spin_locks[heads_index]);
	  if (delete_status > 0)
            {
	      fprintf(stderr, "List delete error: Corrupted list\n");
	      exit(2);
            }
        }
      //store lock wait time in lock_time with thread's index
      lock_time[index/iterations_number] = runtime;
    }
  //no lock
  else
    {
      //insert
      for (i = index; i < index + iterations_number; i++)
        {
	  //get index of head
	  if (lists_number == 1)
            {
	      heads_index = 0;
            }
	  else
            {
	      heads_index = hash(elements[i].key);
            }
	  SortedList_insert(&heads[heads_index], &elements[i]);
        }
      //get length
      for (i = 0; i < lists_number; i++)
        {
	  sublist_length = SortedList_length(&heads[i]);
	  //check sublist's length
	  if (sublist_length < 0)
            {
	      fprintf(stderr, "Insert length error: Corrupted list\n");
	      exit(2);
            }
	  length = length + sublist_length;
        }
      //check total length
      if (length < 0)
        {
	  fprintf(stderr, "Insert length error: Corrupted list\n");
	  exit(2);
        }
      //delete
      for (i = index; i < index + iterations_number; i++)
        {
	  //get index of head
	  if (lists_number == 1)
            {
	      heads_index = 0;
            }
	  else
            {
	      heads_index = hash(elements[i].key);
            }
	  //look up element to delete
	  SortedListElement_t* temp = SortedList_lookup(&heads[heads_index], elements[i].key);
	  if (temp == NULL)
            {
	      fprintf(stderr, "Unable to find the element to delete error: Corrupted list\n");
	      exit(2);
            }
	  delete_status = SortedList_delete(temp);
	  if (delete_status == 1)
            {
	      fprintf(stderr, "List delete error: Corrupted list\n");
	      exit(2);
            }
        }
    }
  return NULL;
}


    
int main(int argc, char **argv) {
  int threads_number = 1;
  lists_flag = 0;
  lists_number = 1;
  iterations_number = 1;
  m_lock_flag = 0;
  s_lock_flag = 0;
  int yield_flag = 0;
  int c;
    struct option long_options[] =
      {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"lists", required_argument, 0, 'l'},
        {0, 0, 0, 0}
      };
    while (1)
      {
        int option_index = 0;
        c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1)
	  {
            break;
	  }
        switch (c)
	  {
	  case 't':
	    threads_number = atoi(optarg);
	    break;
	  case 'i':
	    iterations_number = atoi(optarg);
	    break;
	  case 'y':
            {
	      yield_flag = 1;
	      int j;
	      j = 0;
	      while (j < (int)strlen(optarg))
                {
		  if (optarg[j] == 'i')
                    {
		      opt_yield |= INSERT_YIELD;
		      i_yield = 1;
                    }
		  else if (optarg[j] == 'd')
                    {
		      opt_yield |= DELETE_YIELD;
		      d_yield = 1;
                    }
		  else if (optarg[j] == 'l')
                    {
		      opt_yield |= LOOKUP_YIELD;
		      l_yield = 1;
                    }
		  j++;
                }
            }
	    break;
	  case 's':
	    if (*optarg == 'm')
	      {
		m_lock_flag = 1;
	      }
	    if (*optarg == 's')
	      {
		s_lock_flag = 1;
	      }
	    break;
	  case 'l':
	    lists_flag = 1;
	    lists_number = atoi(optarg);
	    break;
	  default:
	    fprintf(stderr, "Error: Needs to be in the form: lab2-list [--threads thread_number [--iterations iteration_number] [--yield idl] [--sync type]\n");
	    exit(1);
	    break;
	  }
      }
    //create array of heads
    int i;
    heads = (SortedList_t *) malloc(lists_number * sizeof(SortedList_t));
    if (heads == NULL)
      {
        fprintf(stderr, "Creating heads list malloc error: %s\n", strerror(errno));
        exit(1);
      }
    //initialize each head
    for (i = 0; i < lists_number; i++)
      {
        heads[i].key = NULL;
        heads[i].next = &heads[i];
        heads[i].prev = &heads[i];
      }
    //get total number of elements
    int list_length = threads_number * iterations_number;
    elements = (SortedListElement_t*) malloc(list_length * sizeof(SortedListElement_t));
    if (elements == NULL)
      {
        fprintf(stderr, "Creating elements malloc error: %s\n", strerror(errno));
        exit(1);
      }
    //create random keys
    int len;
    int m;
    for (i = 0; i < list_length; i++)
      {
        //getting variable length
        len = (rand() % 10) + 1;
        char* key = (char*) malloc((len + 1) * sizeof(char));
        //getting variable characters
        for (m = 0; m < len; m++) {
	  key[m] = rand() % 74 + '0';
        }
        key[len] = '\0';
        elements[i].key = key;
        elements[i].prev = NULL;
        elements[i].next = NULL;
      }
    //initialize mutex lock
    if (m_lock_flag == 1)
      {
        locks = (pthread_mutex_t*) malloc(lists_number * sizeof(pthread_mutex_t));
        for (i = 0; i < lists_number; i++)
	  {
            pthread_mutex_init(&locks[i], NULL);
	  }
      }
    //initialize spin lock
    if (s_lock_flag == 1)
      {
        spin_locks = (int*) malloc(lists_number * sizeof(int));
        for (i = 0; i < lists_number; i++)
	  {
            spin_locks[i] = 0;
	  }
      }
    pthread_t* threads = (pthread_t*) malloc(threads_number * sizeof(pthread_t));
    if (threads == NULL)
      {
        fprintf(stderr, "Creating threads malloc error: %s\n", strerror(errno));
        exit(1);
      }
    //array of element indexes for each thread
    int *element_index = (int*) malloc(threads_number * sizeof(int));
    if (element_index == NULL)
      {
        fprintf(stderr, "Malloc error: %s\n", strerror(errno));
        exit(1);
      }
    //create array of lock times for each thread
    lock_time = (long long*) malloc(threads_number * sizeof(long long));
    signal(SIGSEGV, handler);
    struct timespec startTime, endTime;
    if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
      {
        fprintf(stderr, "Start clock error: %s\n", strerror(errno));
        exit(1);
      }
    //create threads
    for (i = 0; i < threads_number; i++)
      {
        //get the index of the elements the current thread needs
        element_index[i] = i * iterations_number;
        if (pthread_create(&threads[i], NULL, (void*)&thread_func, (void*)&element_index[i]) != 0)
	  {
            fprintf(stderr, "Creating threads error: %s\n", strerror(errno));
            exit(1);
	  }
      }
    //join the threads
    for (i = 0; i < threads_number; i++)
      {
        if (pthread_join(threads[i], NULL) != 0)
	  {
            fprintf(stderr, "Joining threads error: %s\n", strerror(errno));
            exit(1);
	  }
      }
    if (clock_gettime(CLOCK_REALTIME, &endTime) < 0)
      {
        fprintf(stderr, "End clock error: %s\n", strerror(errno));
        exit(1);
      }
    long long runtime = 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
    //check if all elements have been deleted
    int ending_length = 0;
    for (i = 0; i < lists_number; i++)
      {
        ending_length += SortedList_length(&heads[i]);
      }
    if (ending_length != 0)
      {
        fprintf(stderr, "Final length error: Corrupted list\n");
        exit(2);
      }
    long long total_lock_time = 0;
    //get total lock times for each thread only if there was a lock
    if (m_lock_flag == 1 || s_lock_flag == 1)
      {
        for (i = 0; i < threads_number; i++)
	  {
            total_lock_time += lock_time[i];
	  }
      }
    //free the keys
    for (i = 0; i < list_length; i++)
      {
        free((void*)elements[i].key);
      }
    //free the mutex locks
    if (m_lock_flag == 1)
      {
        for (i = 0; i < lists_number; i++)
	  {
            pthread_mutex_destroy(&locks[i]);
	  }
        free(locks);
      }
    //free the spin locks
    if (s_lock_flag == 1)
      {
        free(spin_locks);
      }
    free(heads);
    free(elements);
    free(threads);
    free(element_index);
    free(lock_time);
    //printing section
    char message[25];
    strcpy(message, "list");
    if (yield_flag == 1)
      {
        //yield options
        strcat(message, "-");
        if (i_yield == 1 || d_yield == 1 || l_yield == 1)
	  {
            if (i_yield == 1)
	      strcat(message, "i");
            if (d_yield == 1)
	      strcat(message, "d");
            if (l_yield == 1)
	      strcat(message, "l");
	  }
        //sync options
        else
	  strcat(message, "none");
        
        if (m_lock_flag == 1)
	  strcat(message, "-m");
        else if (s_lock_flag == 1)
	  strcat(message, "-s");
        else
	  strcat(message, "-none");
      }
    else
      {
        //sync options
        strcat(message, "-none");
        if (m_lock_flag == 1)
	  strcat(message, "-m");
        else if (s_lock_flag == 1)
	  strcat(message, "-s");
        else
	  strcat(message, "-none");
      }
    int operation_num = threads_number * iterations_number * 3;
    long long average = runtime/operation_num;
    long long lock_average_wait = 0;
    //only calculate wait times if there is a lock
    if (m_lock_flag == 1 || s_lock_flag == 1)
      {
        lock_average_wait = total_lock_time/operation_num;
      }
    printf("%s,%d,%d,%d,%d,%lld,%lld,%lld\n", message, threads_number, iterations_number, lists_number, operation_num, runtime, average, lock_average_wait);
    exit(0);
}
