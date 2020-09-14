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

SortedList_t* list;
SortedListElement_t* elements;

int iterations_number;
pthread_mutex_t lock;
int m_lock_flag;
int s_lock_flag;
int spin_lock;
int i_yield;
int d_yield;
int l_yield;
int opt_yield;

void handler()
{
    fprintf(stderr, "Segfault: Corrupted list\n");
    exit(2);
}

void* thread_func(void* position)
{
    int index = *((int*) position);
    int i;
    int length = 0;
    int delete_status = 0;
    //mutex lock
    if (m_lock_flag == 1)
    {
        //insert
        for (i = index; i < index + iterations_number; i++)
        {
            pthread_mutex_lock(&lock);
            SortedList_insert(list, &elements[i]);
            pthread_mutex_unlock(&lock);
        }
        //get length
        pthread_mutex_lock(&lock);
        length = SortedList_length(list);
        pthread_mutex_unlock(&lock);
        if (length < 0)
        {
            fprintf(stderr, "Insert length error: Corrupted list\n");
            exit(2);
        }
        //delete
        for (i = index; i < index + iterations_number; i++)
        {
            pthread_mutex_lock(&lock);
            //look up element to delete
            SortedListElement_t* temp = SortedList_lookup(list, elements[i].key);
            if (temp == NULL)
            {
                fprintf(stderr, "Unable to find the element to delete error: Corrupted list\n");
                pthread_mutex_unlock(&lock);
                exit(2);
            }
            delete_status = SortedList_delete(temp);
            pthread_mutex_unlock(&lock);
            if (delete_status > 0)
            {
                fprintf(stderr, "List delete error: Corrupted list\n");
                exit(2);
            }
        }
    }
    //spin lock
    else if (s_lock_flag == 1)
    {
        //insert
        for (i = index; i < index + iterations_number; i++)
        {
            while (__sync_lock_test_and_set(&spin_lock, 1))
            {
                continue;
            }
            SortedList_insert(list, &elements[i]);
            __sync_lock_release(&spin_lock);
        }
        //get length
        while (__sync_lock_test_and_set(&spin_lock, 1))
        {
            continue;
        }
        length = SortedList_length(list);
        __sync_lock_release(&spin_lock);
        if (length < 0)
        {
            fprintf(stderr, "Insert length error: Corrupted list\n");
            exit(2);
        }
        //delete
        for (i = index; i < index + iterations_number; i++)
        {
            while (__sync_lock_test_and_set(&spin_lock, 1))
            {
                continue;
            }
            //look up element to delete
            SortedListElement_t* temp = SortedList_lookup(list, elements[i].key);
            if (temp == NULL)
            {
                fprintf(stderr, "Unable to find the element to delete error: Corrupted list\n");
                __sync_lock_release(&spin_lock);
                exit(2);
            }
            delete_status = SortedList_delete(temp);
            __sync_lock_release(&spin_lock);
            if (delete_status > 0)
            {
                fprintf(stderr, "List delete error: Corrupted list\n");
                exit(2);
            }
        }
    }
    //no lock
    else
    {
        //insert
        for (i = index; i < index + iterations_number; i++)
        {
            SortedList_insert(list, &elements[i]);
        }
        //get length
        length = SortedList_length(list);
        if (length < 0)
        {
            fprintf(stderr, "Insert length error: Corrupted list\n");
            exit(2);
        }
        //delete
        for (i = index; i < index + iterations_number; i++)
        {
            //look up element to delete
            SortedListElement_t* temp = SortedList_lookup(list, elements[i].key);
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
            default:
                fprintf(stderr, "Error: Needs to be in the form: lab2-add [--threads thread_number [--iterations iteration_number] [--yield idl] [--sync type]\n");
                exit(1);
                break;
        }
    }
    //initialize an empty list
    //create head
    list = (SortedList_t *) malloc(sizeof(SortedList_t));
    if (list == NULL)
    {
        fprintf(stderr, "Creating list malloc error: %s\n", strerror(errno));
        exit(1);
    }
    //initialize head
    list->key = NULL;
    list->next = list;
    list->prev = list;
    
    //get total number of elements
    int list_length = threads_number * iterations_number;
    elements = (SortedListElement_t*) malloc(list_length * sizeof(SortedListElement_t));
    if (elements == NULL)
    {
        fprintf(stderr, "Creating elements malloc error: %s\n", strerror(errno));
        exit(1);
    }
    //create random keys
    int i;
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
        pthread_mutex_init(&lock, NULL);
    }
    //initialize spin lock
    if (s_lock_flag == 1)
    {
        spin_lock = 0;
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
    int ending_length = SortedList_length(list);
    if (ending_length != 0)
    {
        fprintf(stderr, "Final length error: Corrupted list\n");
        exit(2);
    }
    for (i = 0; i < list_length; i++)
        free((void*)elements[i].key);
    
    free(elements);
    free(threads);
    free(element_index);
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
    printf("%s,%d,%d,1,%d,%lld,%lld\n", message, threads_number, iterations_number, operation_num, runtime, average);
    exit(0);
}
