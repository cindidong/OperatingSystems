// NAME: Cindi Dong
// EMAIL: cindidong@gmail.com
// ID: 405126747

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <mraa.h>


static volatile int run_flag = 1;
int log_flag;
int log_file;
mraa_aio_context sensor;
mraa_gpio_context button;


void do_when_interrupted()
{
    run_flag = 0;
    //get the time
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    if (info == NULL)
    {
        fprintf(stderr, "Getting time error: %s\n", strerror(errno));
        exit(1);
    }
    //print the time with SHUTDOWN
    if (log_flag == 1)
    {
        char temp[23];
        sprintf(temp, "%02d:%02d:%02d SHUTDOWN\n", info->tm_hour, info->tm_min, info->tm_sec);
        size_t len = strlen(temp);
        if (write(log_file, temp, len) < 0)
        {
            fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
            exit(1);
        }
    }
    else
    {
        fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", info->tm_hour, info->tm_min, info->tm_sec);
    }
    if (log_flag == 1)
    {
        close(log_file);
    }
    mraa_aio_close(sensor);
    mraa_gpio_close(button);
    exit(0);
}


int main(int argc, char **argv) {
    char scale = 'F';
    int period = 1;
    //for sanity check; sample temperature first
    int beginning_flag = 1;
    log_flag = 0;
    char* filename = NULL;
    int report = 1;
    int c;
    struct option long_options[] =
    {
        {"scale", required_argument, 0, 's'},
        {"period", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
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
            case 's':
                scale = optarg[0];
                break;
            case 'p':
                period = atoi(optarg);
                break;
            case 'l':
                log_flag = 1;
                filename = optarg;
                break;
            default:
                fprintf(stderr, "Needs to be in the form: lab4b [--scale] [--period] [--log]\n");
                exit(1);
                break;
        }
    }
    //checking for bad arguments
    if (scale != 'C' && scale != 'F')
    {
        fprintf(stderr, "Needs to be --scale=C or --scale=F\n");
        exit(1);
    }
    if (period == 0)
    {
        fprintf(stderr, "Period cannot be 0\n");
        exit(1);
    }
    if (log_flag == 1)
    {
        log_file = creat(filename, 0666);
        if (log_file < 0)
        {
            fprintf(stderr, "Opening log file error: %s\n", strerror(errno));
            exit(1);
        }
    }
    //intitalize
    sensor = mraa_aio_init(1);
    if (sensor == NULL)
    {
        fprintf(stderr, "Initialize temperature sensor error\n");
        exit(1);
    }
    button = mraa_gpio_init(60);
    if (button == NULL)
    {
        fprintf(stderr, "Initialize button error\n");
        exit(1);
    }
    //configure button as input pin
    if (mraa_gpio_dir(button, MRAA_GPIO_IN) != MRAA_SUCCESS)
    {
        fprintf(stderr, "Setting button to input error\n");
        exit(1);
    }
    //configure interrupt handler
    if (mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &do_when_interrupted, NULL) != MRAA_SUCCESS)
    {
        fprintf(stderr, "Seting button interrupt error\n");
        exit(1);
    }
    
    int i = 0;
    char temp[257];
    int index = 0;
    //set up poll struct
    struct pollfd poll_list[1];
    poll_list[0].fd = STDIN_FILENO;
    poll_list[0].events = POLLIN | POLLHUP | POLLERR;
    int poll_status;
    
    //set up time to check if a period has passed
    time_t start, end;
    time(&start);
    while (run_flag)
    {
        //check if time to get temperature
        time(&end);
        if (difftime(end, start) >= period || beginning_flag == 1)
        {
            time(&start);
            //check if report is started
            if (report == 1)
            {
                //get time
                struct tm *info;
                info = localtime(&start);
                if (info == NULL)
                {
                    fprintf(stderr, "Getting time error: %s\n", strerror(errno));
                    exit(1);
                }
                //get temperature
                int temp_value = mraa_aio_read(sensor);
                float R = 1023.0 / (float)temp_value - 1.0;
                R = 100000*R;
                float temperature = 1.0 / (log(R/100000) / 4275 + 1.0 / 298.15) - 273.15;
                if (scale == 'F')
                {
                    temperature = temperature * 9 / 5 + 32;
                }
                if (log_flag == 1)
                {
                    char temp[20];
                    sprintf(temp, "%02d:%02d:%02d %.1f\n", info->tm_hour, info->tm_min, info->tm_sec, temperature);
                    size_t len = strlen(temp);
                    if (write(log_file, temp, len) < 0)
                    {
                        fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
                        exit(1);
                    }
                }
                else
                {
                    fprintf(stdout, "%.2d:%.2d:%.2d %.1f\n", info->tm_hour, info->tm_min, info->tm_sec, temperature);
                }
            }
            //for sanity check
            beginning_flag = 0;
        }
        //poll for input
        poll_status = poll(poll_list, 1, 0);
        if (poll_status < 0)
        {
            fprintf(stderr, "Polling error: %s\n", strerror(errno));
            exit(1);
        }
        if (poll_status > 0)
        {
            //input from stdin
            if ((poll_list[0].revents & POLLIN) == POLLIN)
            {
                char b[256];
                int read_status;
                read_status = read(STDIN_FILENO, b, 256);
                if (read_status < 0)
                {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    exit(1);
                }
                //parse input
                for (i = 0; i < read_status; i++)
                {
                    char c = b[i];
                    //if newline, there is a new command
                    if (c == '\n')
                    {
                        temp[index] = '\0';
                        if (log_flag == 1)
                        {
                            char str[20];
                            sprintf(str, "%s\n", temp);
                            size_t len = strlen(str);
                            if (write(log_file, str, len) < 0)
                            {
                                fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
                                exit(1);
                            }
                        }
                        if (strncmp(temp, "SCALE=F", strlen("SCALE=F")) == 0)
                        {
                            scale = 'F';
                        }
                        else if (strncmp(temp, "SCALE=C", strlen("SCALE=C")) == 0)
                        {
                            scale = 'C';
                        }
                        else if (strncmp(temp, "PERIOD=", strlen("PERIOD=")) == 0)
                        {
                            period = atoi(temp + 7);
                        }
                        else if (strcmp(temp, "STOP") == 0)
                        {
                            report = 0;
                        }
                        else if (strcmp(temp, "START") == 0)
                        {
                            report = 1;
                        }
                        else if (strncmp(temp, "LOG", 3) == 0)
                        {
                        }
                        else if (strcmp(temp, "OFF") == 0)
                        {
                            do_when_interrupted();
                        }
                        index = 0;
                    }
                    else
                    {
                        temp[index] = c;
                        index++;
                    }
                }
            }
            if (((poll_list[0].revents & POLLERR) == POLLERR))
            {
                fprintf(stderr, "Polling stdin error: %s\n", strerror(errno));
                exit(1);
            }
        }
    }
    if (log_flag == 1)
    {
        close(log_file);
    }
    mraa_aio_close(sensor);
    mraa_gpio_close(button);
    exit(0);
}
