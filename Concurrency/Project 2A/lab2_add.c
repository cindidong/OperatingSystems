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

long long counter;
int iterations_number;
pthread_mutex_t lock;
int m_lock_flag;
int s_lock_flag;
int c_lock_flag;
int spin_lock;

int opt_yield;
void add(long long *pointer, long long value) {
    //mutex lock
    if (m_lock_flag == 1)
    {
        pthread_mutex_lock(&lock);
        long long sum = *pointer + value;
        if (opt_yield)
        {
            sched_yield();
        }
        *pointer = sum;
        pthread_mutex_unlock(&lock);
    }
    //spin lock
    else if (s_lock_flag == 1)
    {
        while (__sync_lock_test_and_set(&spin_lock, 1))
        {
            continue;
        }
        long long sum = *pointer + value;
        if (opt_yield)
        {
            sched_yield();
        }
        *pointer = sum;
        __sync_lock_release(&spin_lock);
    }
    //compare and swap
    else if (c_lock_flag == 1)
    {
        long long oldVal;
        do
        {
            oldVal = counter;
            if (opt_yield)
            {
                sched_yield();
            }
        } while (__sync_val_compare_and_swap(pointer, oldVal, oldVal + value) != oldVal);
    }
    //no lock
    else
    {
        long long sum = *pointer + value;
        if (opt_yield)
        {
            sched_yield();
        }
        *pointer = sum;
    }
}

void* thread_func(void)
{
    int i;
    //add to counter
    for (i = 0; i < iterations_number; i++)
    {
        add(&counter, 1);
    }
    //subtract from counter
    for (i = 0; i < iterations_number; i++)
    {
        add(&counter, -1);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int threads_number = 1;
    iterations_number = 1;
    m_lock_flag = 0;
    s_lock_flag = 0;
    c_lock_flag = 0;
    int c;
    struct option long_options[] =
    {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
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
                opt_yield = 1;
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
                if (*optarg == 'c')
                {
                    c_lock_flag = 1;
                }
                break;
            default:
                fprintf(stderr, "Error: Needs to be in the form: lab2-add [--threads thread_number [--iterations iteration_number] [--yield] [--sync type]\n");
                exit(1);
                break;
        }
    }
    counter = 0;
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
    int i;
    struct timespec startTime, endTime;
    if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
    {
        fprintf(stderr, "Start clock error: %s\n", strerror(errno));
        exit(1);
    }
    //create threads
    for (i = 0; i < threads_number; i++)
    {
        if (pthread_create(&threads[i], NULL, (void*)&thread_func, NULL) != 0)
        {
            fprintf(stderr, "Creating threads error: %s\n", strerror(errno));
            exit(1);
        }
    }
    //join threads
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
    //get runtime
    long long runtime = 1000000000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_nsec - startTime.tv_nsec);
    free(threads);
    char message[11];
    //if yield flag
    if (opt_yield == 1)
    {
        if (m_lock_flag == 1)
            sprintf(message, "yield-m");
        else if (s_lock_flag == 1)
            sprintf(message, "yield-s");
        else if (c_lock_flag == 1)
            sprintf(message, "yield-c");
        else
            sprintf(message, "yield-none");
    }
    //if no yield flag
    else
    {
        if (m_lock_flag == 1)
            sprintf(message, "m");
        else if (s_lock_flag == 1)
            sprintf(message, "s");
        else if (c_lock_flag == 1)
            sprintf(message, "c");
        else
            sprintf(message, "none");
    }
    int operation_num = threads_number * iterations_number * 2;
    long long average = runtime/operation_num;
    printf("add-%s,%d,%d,%d,%lld,%lld,%lld\n", message, threads_number, iterations_number, operation_num, runtime, average, counter);
    exit(0);
}
