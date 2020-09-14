// NAME: Cindi Dong
// EMAIL: cindidong@gmail.com
// ID: 405126747

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ulimit.h>
#include <zlib.h>


struct termios saved;
int sockfd;

//function for log write errors
void logfile_write_error(char* log_file);


//shut down socket and reset back into canonical
void reset_mode()
{
  if (shutdown(sockfd, SHUT_RDWR) < 0)
    {
      fprintf(stderr, "Closing client socket error: %s\n", strerror(errno));
      exit(1);
    }
  int termois_status = tcsetattr(STDIN_FILENO, TCSANOW, &saved);
  if (termois_status < 0)
    {
      fprintf(stderr, "Resetting input mode error: %s\n", strerror(errno));
      exit(1);
    }
}

//set to noncanonical
void set_mode()
{
  //checking if stdin is a terminal
  if(!isatty(STDIN_FILENO))
    {
      fprintf(stderr, "Stdin is not a terminal error: %s\n", strerror(errno));
      exit(1);
    }
  struct termios new;
  int termois_status;
  termois_status = tcgetattr(STDIN_FILENO, &saved);
  if (termois_status < 0)
    {
      fprintf(stderr, "Getting mode error: %s\n", strerror(errno));
      exit(1);
    }
  atexit(reset_mode);
  termois_status = tcgetattr(STDIN_FILENO, &new);
  if (termois_status < 0)
    {
      fprintf(stderr, "Getting mode error: %s\n", strerror(errno));
      exit(1);
    }
  new.c_iflag = ISTRIP;
  new.c_oflag = 0;
  new.c_lflag = 0;
  termois_status = tcsetattr(STDIN_FILENO, TCSANOW, &new);
  if (termois_status < 0)
    {
      fprintf(stderr, "Setting new mode error: %s\n", strerror(errno));
      exit(1);
    }
}

void logfile_write_error(char* log_file)
{
  fprintf(stderr, "Writing to log file %s error: %s\n", log_file, strerror(errno));
  exit(1);
}



int main(int argc, char **argv) {
  int port_flag = 0;
  char* port_name = NULL;
  int log_flag = 0;
  char* log_file = NULL;
  int compress_flag = 0;
  int c;
    struct option long_options[] =
      {
        {"port", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"compress", no_argument, 0, 'c'},
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
	  case 'p':
	    port_flag = 1;
	    port_name = optarg;
	    break;
	  case 'l':
	    log_flag = 1;
	    log_file = optarg;
	    break;
	  case 'c':
	    compress_flag = 1;
	    break;
	  default:
	    fprintf(stderr, "Error: Needs to be in the form: lab1a [--port port_number] [--log filename] [--compress]\n");
	    exit(1);
	    break;
	  }
      }
    if (port_flag == 0)
      {
        fprintf(stderr, "Need to specify a port\n");
        exit(1);
      }
    
    //setting to noncanonical
    set_mode();
    //set up socket
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    portno = atoi(port_name);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
      {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        exit(1);
      }
    server = gethostbyname("localhost");
    if (server == NULL)
      {
        fprintf(stderr, "Unable to get host info error: %s\n", strerror(errno));
        exit(1);
      }
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
      {
        fprintf(stderr, "Connection error: %s\n", strerror(errno));
        exit(1);
      }
    //set up log file
    int logfd;
    if (log_flag == 1)
      {
        logfd = creat(log_file, 0666);
        if (logfd < 0)
	  {
            fprintf(stderr, "Error opening log file %s: %s\n", log_file, strerror(errno));
            exit(1);
	  }
        if (ulimit(UL_SETFSIZE, 10000) < 0)
	  {
            fprintf(stderr, "Error setting ulimit: %s\n", strerror(errno));
            exit(1);
	  }
      }
    //set up compress
    z_stream deflate_strm;
    z_stream inflate_strm;
    if (compress_flag == 1)
      {
        int ret;
        //intialize deflate
        deflate_strm.zalloc = Z_NULL;
        deflate_strm.zfree = Z_NULL;
        deflate_strm.opaque = Z_NULL;
        ret = deflateInit(&deflate_strm, Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK)
	  {
            fprintf(stderr, "Deflate initalization error: %s\n", strerror(errno));
            exit(1);
	  }
        
        //intialize inflate
        inflate_strm.zalloc = Z_NULL;
        inflate_strm.zfree = Z_NULL;
        inflate_strm.opaque = Z_NULL;
        ret = inflateInit(&inflate_strm);
        if (ret != Z_OK)
	  {
            fprintf(stderr, "Deflate initalization error: %s\n", strerror(errno));
            exit(1);
	  }
      }
    //set up poll
    struct pollfd poll_list[2];
    poll_list[0].fd = STDIN_FILENO;
    poll_list[1].fd = sockfd;
    poll_list[0].events = POLLIN | POLLHUP | POLLERR;
    poll_list[1].events = POLLIN | POLLHUP | POLLERR;
    int poll_status;
    while (1)
      {
        poll_status = poll(poll_list, 2, 0);
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
                int write_status;
                read_status = read(STDIN_FILENO, b, 256);
                if (read_status < 0)
		  {
                    fprintf(stderr, "Read error: %s\n", strerror(errno));
                    exit(1);
		  }
                //deflate/compress
                if (compress_flag == 1)
		  {
                    int ret;
                    char out[256];
                    deflate_strm.avail_in = read_status;
                    deflate_strm.next_in = (unsigned char *)b;
                    deflate_strm.avail_out = 256;
                    deflate_strm.next_out = (unsigned char *)out;
                    do {
		      ret = deflate(&deflate_strm, Z_SYNC_FLUSH);
		      if (ret != Z_OK)
                        {
			  fprintf(stderr, "Compression error: %s\n", strerror(errno));
			  exit(1);
                        }
                    } while (deflate_strm.avail_in > 0);
                    //write compressed to server
                    write_status = write(sockfd, out, 256 - deflate_strm.avail_out);
                    if (write_status < 0)
		      {
                        fprintf(stderr, "Writing to server error: %s\n", strerror(errno));
                        exit(1);
		      }
                    //write compressed to log
                    if (log_flag == 1)
		      {
                        if (write(logfd, "SENT ", 5) < 0)
			  logfile_write_error(log_file);
                        char num[10];
                        sprintf(num, "%d", 256 - deflate_strm.avail_out);
                        if (write(logfd, num, strlen(num)) < 0)
			  logfile_write_error(log_file);
                        if (write(logfd, " bytes: ", 8) < 0)
			  logfile_write_error(log_file);
                        if (write(logfd, out, 256 - deflate_strm.avail_out) < 0)
			  logfile_write_error(log_file);
                        if (write(logfd, "\n", 1) < 0)
			  logfile_write_error(log_file);
		      }
		  }
                //does not need to be compressed
                else {
		  //write directly to server
		  write_status = write(sockfd, b, read_status);
		  if (write_status < 0)
                    {
		      fprintf(stderr, "Writing to server error: %s\n", strerror(errno));
		      exit(1);
                    }
		  //write directly into log
		  if (log_flag == 1)
                    {
		      if (write(logfd, "SENT ", 5) < 0)
			logfile_write_error(log_file);
		      char num[10];
		      sprintf(num, "%d", read_status);
		      if (write(logfd, num, strlen(num)) < 0)
			logfile_write_error(log_file);
		      if (write(logfd, " bytes: ", 8) < 0)
			logfile_write_error(log_file);
		      if (write(logfd, b, read_status) < 0)
			logfile_write_error(log_file);
		      if (write(logfd, "\n", 1) < 0)
			logfile_write_error(log_file);
                    }
                }
                //write to stdout
                int i = 0;
                while (i < read_status)
		  {
                    char c = b[i];
                    //escape case
                    if (c == 0x03)
		      {
                        write_status = write(STDOUT_FILENO, "^C", 2);
                        if (write_status < 0)
			  {
                            fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                            exit(1);
			  }
		      }
                    //eof case
                    else if (c == 0x04)
		      {
                        write_status = write(STDOUT_FILENO, "^D", 2);
                        if (write_status < 0)
			  {
                            fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                            exit(1);
			  }
		      }
                    //returns case
                    else if (c == '\n' || c == '\r')
		      {
                        write_status = write(STDOUT_FILENO, "\r\n", 2);
                        if (write_status < 0)
			  {
                            fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                            exit(1);
			  }
		      }
                    else
		      {
                        write_status = write(STDOUT_FILENO, &c, 1);
                        if (write_status < 0)
			  {
                            fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                            exit(1);
			  }
		      }
                    i++;
		  }
	      }
            //input from server
            if ((poll_list[1].revents & POLLIN) == POLLIN)
	      {
                char b[256];
                int read_status;
                int write_status;
                read_status = read(sockfd, b, 256);
                //if there is no more to read then exit
                if (read_status == 0)
		  {
                    if (compress_flag == 1)
		      {
                        deflateEnd(&deflate_strm);
                        inflateEnd(&inflate_strm);
		      }
                    exit(0);
		  }
                if (read_status < 0)
		  {
                    fprintf(stderr, "Reading from server error: %s\n", strerror(errno));
                    exit(1);
		  }
                if (compress_flag == 1)
		  {
                    //inflate/decompress
                    int ret;
                    char out[1024];
                    inflate_strm.avail_in = read_status;
                    inflate_strm.next_in = (unsigned char *)b;
                    inflate_strm.avail_out = 1024;
                    inflate_strm.next_out = (unsigned char *)out;
                    do {
		      ret = inflate(&inflate_strm, Z_SYNC_FLUSH);
		      if (ret != Z_OK)
                        {
			  fprintf(stderr, "Decompression error\n");
			  exit(1);
                        }
                    } while (inflate_strm.avail_in > 0);
                    //process input from server
                    int i = 0;
                    while ((unsigned int)i < 1024 - inflate_strm.avail_out)
		      {
                        char c = out[i];
                        //escape case
                        if (c == 0x03)
			  {
                            write_status = write(STDOUT_FILENO, "^C", 2);
                            if (write_status < 0)
			      {
                                fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                                exit(1);
			      }
			  }
                        //eof case
                        else if (c == 0x04)
			  {
                            write_status = write(STDOUT_FILENO, "^D", 2);
                            if (write_status < 0)
			      {
                                fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                                exit(1);
			      }
                            //if there is eof break out of the loop
                            break;
			  }
                        //returns case
                        else if (c == '\n' || c == '\r')
			  {
                            write_status = write(STDOUT_FILENO, "\r\n", 2);
                            if (write_status < 0)
			      {
                                fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                                exit(1);
			      }
			  }
                        else
			  {
                            write_status = write(STDOUT_FILENO, &c, 1);
                            if (write_status < 0)
			      {
                                fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                                exit(1);
			      }
			  }
                        i++;
		      }
		  }
                //no decompressing, write directly to stdout
                //took care of processing on server side
                else
		  {
                    write_status = write(STDOUT_FILENO, b, read_status);
                    if (write_status < 0)
		      {
                        fprintf(stderr, "Writing to stdout error: %s\n", strerror(errno));
                        exit(1);
		      }
		  }
                //keep compressed version in log
                if (log_flag == 1)
		  {
                    if (write(logfd, "RECEIVED ", 9) < 0)
		      logfile_write_error(log_file);
                    char num[10];
                    sprintf(num, "%d", read_status);
                    if (write(logfd, num, strlen(num)) < 0)
		      logfile_write_error(log_file);
                    if (write(logfd, " bytes: ", 8) < 0)
		      logfile_write_error(log_file);
                    if (write(logfd, b, read_status) < 0)
		      logfile_write_error(log_file);
                    if (write(logfd, "\n", 1) < 0)
		      logfile_write_error(log_file);
		  }
	      }
            //error in the stdin
            if (((poll_list[0].revents & POLLERR) == POLLERR))
	      {
                fprintf(stderr, "Polling stdin error: %s\n", strerror(errno));
                exit(1);
	      }
            //error in the server
            if (((poll_list[1].revents & POLLHUP) == POLLHUP) || ((poll_list[1].revents & POLLERR) == POLLERR))
	      {
                if (compress_flag == 1)
		  {
                    deflateEnd(&deflate_strm);
                    inflateEnd(&inflate_strm);
		  }
                exit(0);
	      }
	  }
      }
}
