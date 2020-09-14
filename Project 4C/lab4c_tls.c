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
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mraa.h>


static volatile int run_flag = 1;
int log_file;
int sockfd;
mraa_aio_context sensor;
SSL *sslClient = NULL;


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
        exit(2);
    }
    //print the time with SHUTDOWN
    char temp[23];
    sprintf(temp, "%02d:%02d:%02d SHUTDOWN\n", info->tm_hour, info->tm_min, info->tm_sec);
    size_t len = strlen(temp);
    if (SSL_write(sslClient, temp, len) < 0)
    {
        fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
        exit(2);
    }
    if (write(log_file, temp, len) < 0)
    {
        fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
        exit(2);
    }
    mraa_aio_close(sensor);
    SSL_shutdown(sslClient);
    SSL_free(sslClient);
    close(log_file);
    shutdown(sockfd, 0);
    close(sockfd);
    exit(0);
}


int main(int argc, char **argv) {
    char scale = 'F';
    int period = 1;
    char* filename = NULL;
    int report = 1;
    int port_number = -1;
    int id;
    char* temp_id = NULL;
    char* host_name = NULL;
    int c;
    struct option long_options[] =
    {
        {"scale", required_argument, 0, 's'},
        {"period", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
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
                filename = optarg;
                break;
            case 'i':
                temp_id = optarg;
                id = atoi(optarg);
                break;
            case 'h':
                host_name = optarg;
                break;
            default:
                fprintf(stderr, "Needs to be in the form: lab4b [--scale] [--period] [--log] [--id] [--host] portnumber\n");
                exit(1);
                break;
        }
    }
    if (optind >= argc)
    {
        fprintf(stderr, "Need to provide a port number\n");
        exit(1);
    }
    port_number = atoi(argv[optind]);
    if (port_number == 0)
    {
        fprintf(stderr, "Need to provide a port number\n");
        exit(1);
        
    }
    if (temp_id != NULL)
    {
        if (strlen(temp_id) != 9)
        {
            fprintf(stderr, "ID has be 9 digits long\n");
        }
        while (*temp_id != '\0')
        {
            if (*temp_id < '0' || *temp_id > '9')
            {
                fprintf(stderr, "ID has to contain only numbers\n");
                exit(1);
            }
            temp_id++;
        }
    }
    else
    {
        fprintf(stderr, "Need to provide a ID\n");
        exit(1);
    }
    if (host_name == NULL)
    {
        fprintf(stderr, "Need to provide a host\n");
        exit(1);
    }
    if (filename == NULL)
    {
        fprintf(stderr, "Need to provide a log file\n");
        exit(1);
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
    log_file = creat(filename, 0666);
    if (log_file < 0)
    {
        fprintf(stderr, "Opening log file error: %s\n", strerror(errno));
        exit(1);
    }
    //set up socket
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        exit(2);
    }
    server = gethostbyname(host_name);
    if (server == NULL)
    {
        fprintf(stderr, "Unable to get host info error: %s\n", strerror(errno));
        exit(2);
    }
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port_number);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "Connection error: %s\n", strerror(errno));
        exit(2);
    }
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    SSL_CTX *newContext = SSL_CTX_new(TLSv1_client_method());
    if (newContext == NULL)
    {
        fprintf(stderr, "Unable to get SSL context\n");
        exit(2);
    }
    sslClient = SSL_new(newContext);
    if (sslClient == NULL)
    {
        fprintf(stderr, "Unable to complete SSL setup\n");
        exit(2);
    }
    if (!SSL_set_fd(sslClient, sockfd))
    {
        fprintf(stderr, "Unable to link SSL and socket\n");
        exit(2);
    }
    if (SSL_connect(sslClient) != 1)
    {
        fprintf(stderr, "SSL connection rejected\n");
        exit(2);
    }
    
    char out[50];
    sprintf(out, "ID=%d\n", id);
    int write_status;
    write_status = SSL_write(sslClient, out, strlen(out));
    if (write_status < 0)
    {
        fprintf(stderr, "Writing to server error: %s\n", strerror(errno));
        exit(2);
    }
    write_status = write(log_file, out, strlen(out));
    if (write_status < 0)
    {
        fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
        exit(2);
    }
    //intitalize
    sensor = mraa_aio_init(1);
    if (sensor == NULL)
    {
        fprintf(stderr, "Initialize temperature sensor error\n");
        exit(2);
    }
    int i = 0;
    char temp[257];
    int index = 0;
    //set up poll struct
    struct pollfd poll_list[1];
    poll_list[0].fd = sockfd;
    poll_list[0].events = POLLIN | POLLERR;
    int poll_status;
    
    //set up time to check if a period has passed
    time_t start, end;
    time(&start);
    while (run_flag)
    {
        //check if time to get temperature
        time(&end);
        if (difftime(end, start) >= period)
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
                    exit(2);
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
                char temp[20];
                sprintf(temp, "%02d:%02d:%02d %.1f\n", info->tm_hour, info->tm_min, info->tm_sec, temperature);
                size_t len = strlen(temp);
                write_status = SSL_write(sslClient, temp, len);
                if (write_status < 0)
                {
                    fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
                    exit(2);
                }
                if (write(log_file, temp, len) < 0)
                {
                    fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
                    exit(2);
                }
            }
        }
        //poll for input
        poll_status = poll(poll_list, 1, 0);
        if (poll_status < 0)
        {
            fprintf(stderr, "Polling error: %s\n", strerror(errno));
            exit(2);
        }
        if (poll_status > 0)
        {
            //input from stdin
            if ((poll_list[0].revents & POLLIN) == POLLIN)
            {
                char b[256];
                int read_status;
                read_status = SSL_read(sslClient, b, 256);
                if (read_status < 0)
                {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    exit(2);
                }
                //parse input
                for (i = 0; i < read_status; i++)
                {
                    char c = b[i];
                    //if newline, the command is done being processed
                    if (c == '\n')
                    {
                        temp[index] = '\0';
                        char str[20];
                        sprintf(str, "%s\n", temp);
                        size_t len = strlen(str);
                        if (write(log_file, str, len) < 0)
                        {
                            fprintf(stderr, "Writing to log error: %s\n", strerror(errno));
                            exit(2);
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
                    else if (c == ' ' || c == '\t')
                    {
                        continue;
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
                exit(2);
            }
        }
    }
    mraa_aio_close(sensor);
    SSL_shutdown(sslClient);
    SSL_free(sslClient);
    close(log_file);
    shutdown(sockfd, 0);
    close(sockfd);
    exit(0);
}

