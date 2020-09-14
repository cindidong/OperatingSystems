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
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <zlib.h>


int PID;
int p2c[2];  //pipe from parent to child (to shell)
int c2p[2]; //pipe from child to parent (from shell)
int eof_recieved;
int p2c_closed;
int newsockfd;
z_stream deflate_strm;
z_stream inflate_strm;
//global for my exit function
int compress_flag;


void pipe_client(void);
void pipe_shell(void);
void normal_stdin(void);


//SIGPIPE handler
void handler(int signum)
{
  if (signum == SIGPIPE)
    {
      exit(0);
    }
}

//harvest information for exit
void harvest_shell_info()
{
  if (compress_flag == 1)
    {
      deflateEnd(&deflate_strm);
      inflateEnd(&inflate_strm);
    }
  int wait_status;
  if (waitpid(PID, &wait_status, 0) < 0)
    {
      fprintf(stderr, "Child process waiting error: %s\n", strerror(errno));
      exit (1);
    }
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(wait_status), WEXITSTATUS(wait_status));
  //shut down socket
  if (shutdown(newsockfd, SHUT_RDWR) < 0)
    {
      fprintf(stderr, "Closing server socket error: %s\n", strerror(errno));
      exit(1);
    }
}



int main(int argc, char **argv) {
  int shell_flag = 0;
  char* command_name = NULL;
  int port_flag = 0;
  char* port_name = NULL;
  compress_flag = 0;
  int c;
    struct option long_options[] =
      {
        {"shell", required_argument, 0, 's'},
        {"port", required_argument, 0, 'p'},
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
	  case 's':
	    shell_flag = 1;
	    command_name = optarg;
	    break;
	  case 'p':
	    port_flag = 1;
	    port_name = optarg;
	    break;
	  case 'c':
	    compress_flag = 1;
	    break;
	  default:
	    fprintf(stderr, "Error: Needs to be in the form: lab1a [--port port_number] [--shell command_name] [--compress]\n");
	    exit(1);
	    break;
	  }
      }
    //check is there is a port
    if (port_flag == 0)
      {
        fprintf(stderr, "Need to specify a port\n");
        exit(1);
      }
    
    //setting up the sockets
    int sockfd;
    int portno;
    int clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
      {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        exit(1);
      }
    bzero((char*) &serv_addr, sizeof(serv_addr));
    portno = atoi(port_name);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      {
        fprintf(stderr, "Binding error: %s\n", strerror(errno));
        exit(1);
      }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);
    if (newsockfd < 0)
      {
        fprintf(stderr, "Accepting error: %s\n", strerror(errno));
        exit(1);
      }
    //don't need the old socket in this case
    if (shutdown(sockfd, SHUT_RDWR) < 0)
      {
        fprintf(stderr, "Closing server socket error: %s\n", strerror(errno));
        exit(1);
      }
    
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
            fprintf(stderr, "Deflate initalization error\n");
            exit(1);
	  }
        
        //intialize inflate
        inflate_strm.zalloc = Z_NULL;
        inflate_strm.zfree = Z_NULL;
        inflate_strm.opaque = Z_NULL;
        ret = inflateInit(&inflate_strm);
        if (ret != Z_OK)
	  {
            fprintf(stderr, "Inflate initalization error\n");
            exit(1);
	  }
      }
    
    //shell argument
    if (shell_flag == 1)
      {
        signal(SIGPIPE, handler);
        //set up pipes
        if (pipe(p2c) < 0)
	  {
            fprintf(stderr, "Pipe creation error: %s\n", strerror(errno));
            exit(1);
	  }
        if (pipe(c2p) < 0)
	  {
            fprintf(stderr, "Pipe creation error: %s\n", strerror(errno));
            exit(1);
	  }
        PID = fork();
        if (PID < 0)
	  {
            fprintf(stderr, "Forking error: %s\n", strerror(errno));
            exit(1);
	  }
        //there is a child process, so at exit need to harvest info
        atexit(harvest_shell_info);
        //child process
        if (PID == 0)
	  {
            close(p2c[1]);
            close(c2p[0]);
            dup2(p2c[0], STDIN_FILENO);
            close(p2c[0]);
            dup2(c2p[1], STDOUT_FILENO);
            dup2(c2p[1], STDERR_FILENO);
            close(c2p[1]);
            
            //array of arguments
            char *args[2] = { command_name, NULL };
            if (execvp(command_name, args) < 0)
	      {
                fprintf(stderr, "Executing child process error: %s\n", strerror(errno));
                exit(1);
	      }
	  }
        //parent process
        else
	  {
            //don't want to read from parent to child
            close(p2c[0]);
            //don't want to write from the child to the parent
            close(c2p[1]);
            
            //set up poll struct
            struct pollfd poll_list[2];
            poll_list[0].fd = newsockfd;
            poll_list[1].fd = c2p[0];
            poll_list[0].events = POLLIN | POLLHUP | POLLERR;
            poll_list[1].events = POLLIN | POLLHUP | POLLERR;
            int poll_status;
            p2c_closed = 0;
            eof_recieved = 0;
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
                    //input from client
                    if ((poll_list[0].revents & POLLIN) == POLLIN)
		      {
                        pipe_client();
		      }
                    //input from shell/child
                    if ((poll_list[1].revents & POLLIN) == POLLIN)
		      {
                        pipe_shell();
		      }
                    //error from the client
                    if (((poll_list[0].revents & POLLHUP) == POLLHUP) || ((poll_list[0].revents & POLLERR) == POLLERR))
		      {
                        if (p2c_closed == 0)
			  {
                            p2c_closed = 1;
                            close(p2c[1]);
			  }
		      }
                    //error from the shell
                    if (((poll_list[1].revents & POLLHUP) == POLLHUP) || ((poll_list[1].revents & POLLERR) == POLLERR))
		      {
                        eof_recieved = 1;
		      }
		  }
                if (eof_recieved == 1)
		  break;
	      }
            if (p2c_closed == 0)
	      close(p2c[1]);
            close(c2p[0]);
            exit(0);
	  }
      }
    //no shell argument, stdin as normal
    else
      {
        normal_stdin();
      }
    //atexit not called, have to shut down socket manually
    if (shutdown(newsockfd, SHUT_RDWR) < 0)
      {
        fprintf(stderr, "Closing server socket error: %s\n", strerror(errno));
        exit(1);
      }
    exit(0);
}

//no shell argument
void normal_stdin(void)
{
  //setting up poll struct
  struct pollfd poll_list[1];
  poll_list[0].fd = newsockfd;
  poll_list[0].events = POLLIN | POLLHUP | POLLERR;
  eof_recieved = 0;
  int poll_status;
  while (1)
    {
      poll_status = poll(poll_list, 1, 0);
      if (poll_status < 0)
        {
	  fprintf(stderr, "Polling error: %s\n", strerror(errno));
	  exit(1);
        }
      if (poll_status > 0)
        {
	  //input from client
	  if ((poll_list[0].revents & POLLIN) == POLLIN)
            {
	      char b[256];
	      int read_status;
	      int write_status;
	      read_status = read(newsockfd, b, 256);
	      while (read_status > 0)
                {
		  int i = 0;
		  while (i < read_status)
                    {
		      char c = b[i];
		      //eof case
		      if (c == 0x04)
                        {
			  //write ^D to the client and exit
			  char a[2];
			  a[0] = '^';
			  a[1] = 'D';
			  write_status = write(newsockfd, a, 2);
			  if (write_status < 0)
                            {
			      fprintf(stderr, "Write error: %s\n", strerror(errno));
			      exit(1);
                            }
			  //need to shut down the socket before exiting
			  if (shutdown(newsockfd, SHUT_RDWR) < 0)
                            {
			      fprintf(stderr, "Closing server socket error: %s\n", strerror(errno));
			      exit(1);
                            }
			  exit(0);
                        }
		      //escape case
		      else if (c == 0x03)
                        {
			  //write ^C to the client and exit
			  char a[2];
			  a[0] = '^';
			  a[1] = 'C';
			  write_status = write(newsockfd, a, 2);
			  if (write_status < 0)
                            {
			      fprintf(stderr, "Write error: %s\n", strerror(errno));
			      exit(1);
                            }
			  //need to shut down the socket before exiting
			  if (shutdown(newsockfd, SHUT_RDWR) < 0)
                            {
			      fprintf(stderr, "Closing server socket error: %s\n", strerror(errno));
			      exit(1);
                            }
			  exit(0);
                        }
		      //returns case
		      else if (c == '\n' || c == '\r')
                        {
			  char a[2];
			  a[0] = '\r';
			  a[1] = '\n';
			  write_status = write(newsockfd, a, 2);
			  if (write_status < 0)
                            {
			      fprintf(stderr, "Write error: %s\n", strerror(errno));
			      exit(1);
                            }
                        }
		      else
                        {
			  //write the character to the client
			  write_status = write(newsockfd, &c, 1);
			  if (write_status < 0)
                            {
			      fprintf(stderr, "Write error: %s\n", strerror(errno));
			      exit(1);
                            }
                        }
		      i++;
                    }
		  read_status = read(newsockfd, b, 256);
                }
	      if (read_status < 0)
                {
		  fprintf(stderr, "Read error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  //error from the client
	  if (((poll_list[0].revents & POLLHUP) == POLLHUP) || ((poll_list[0].revents & POLLERR) == POLLERR))
            {
	      eof_recieved = 1;
            }
        }
      if (eof_recieved == 1)
	break;
    }
}


//input from the client
void pipe_client()
{
  char b[256];
  int read_status;
  int write_status;
  read_status = read(newsockfd, b, 256);
  if (read_status < 0)
    {
      fprintf(stderr, "Reading from client error: %s\n", strerror(errno));
      exit(1);
    }
  //need to decompress the input from the client
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
      //process decompressed input from client
      int i = 0;
      while ((unsigned int) i < 1024 - inflate_strm.avail_out)
        {
	  char c = out[i];
	  //escape case
	  if (c == 0x03)
            {
	      //kill child process
	      if (kill(PID, SIGINT) < 0)
                {
		  fprintf(stderr, "Kill child process error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  //eof case
	  else if (c == 0x04)
            {
	      //close parent to child (to shell) pipe
	      if (p2c_closed == 0)
                {
		  close(p2c[1]);
		  p2c_closed = 1;
                }
            }
	  //returns case
	  else if (c == '\r' || c == '\n')
            {
	      //just send newline to shell
	      write_status = write(p2c[1], "\n", 1);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to shell error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  else
            {
	      //write the character to the shell
	      write_status = write(p2c[1], &c, 1);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to shell error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  i++;
        }
    }
  //don't need to decompress
  else
    {
      int i = 0;
      while (i < read_status)
        {
	  char c = b[i];
	  //escape case
	  if (c == 0x03)
            {
	      //kill the child process
	      if (kill(PID, SIGINT) < 0)
                {
		  fprintf(stderr, "Kill child process error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  //eof case
	  else if (c == 0x04)
            {
	      //close parent to child (to shell) pipe
	      if (p2c_closed == 0)
                {
		  close(p2c[1]);
		  p2c_closed = 1;
                }
            }
	  //returns case
	  else if (c == '\r' || c == '\n')
            {
	      //send just newline to the shell
	      write_status = write(p2c[1], "\n", 1);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to shell error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  else
            {
	      //send the character to the shell
	      write_status = write(p2c[1], &c, 1);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to shell error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  i++;
        }
    }
}


//input from the shell
void pipe_shell()
{
  char b[256];
  int read_status;
  int write_status;
  read_status = read(c2p[0], b, 256);
  if (read_status < 0)
    {
      fprintf(stderr, "Reading from shell error: %s\n", strerror(errno));
      exit(1);
    }
  //need to deflate/compress
  if (compress_flag == 1)
    {
      //do not process the data, just send
      int ret;
      char out[256];
      deflate_strm.avail_in = read_status;
      deflate_strm.next_in = (unsigned char *) b;
      deflate_strm.avail_out = 256;
      deflate_strm.next_out = (unsigned char *)out;
      do {
	ret = deflate(&deflate_strm, Z_SYNC_FLUSH);
	if (ret != Z_OK)
	  {
	    fprintf(stderr, "Compression error\n");
	    exit(1);
	  }
      } while (deflate_strm.avail_in > 0);
      write_status = write(newsockfd, out, 256 - deflate_strm.avail_out);
      if (write_status < 0)
        {
	  fprintf(stderr, "Writing to server error: %s\n", strerror(errno));
	  exit(1);
        }
      //check for eof for shell shutdown
      int i = 0;
      while (i < read_status)
        {
	  char c = b[i];
	  //eof case
	  if (c == 0x04)
            {
	      //exit from main loop
	      if (eof_recieved == 0)
                {
		  eof_recieved = 1;
		  break;
                }
            }
	  i++;
        }
    }
  //don't need to deflate/compress
  else
    {
      int i = 0;
      while (i < read_status)
        {
	  char c = b[i];
	  //eof case
	  if (c == 0x04)
            {
	      char a[2];
	      a[0] = '^';
	      a[1] = 'D';
	      write_status = write(newsockfd, a, 2);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to client error: %s\n", strerror(errno));
		  exit(1);
                }
	      //flag to exit from loop
	      if (eof_recieved == 0)
                {
		  eof_recieved = 1;
		  break;
                }
            }
	  //newline case
	  else if (c == '\n')
            {
	      char a[2];
	      a[0] = '\r';
	      a[1] = '\n';
	      write_status = write(newsockfd, a, 2);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to client error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  else
            {
	      //write the character to the client
	      write_status = write(newsockfd, &c, 1);
	      if (write_status < 0)
                {
		  fprintf(stderr, "Writing to client error: %s\n", strerror(errno));
		  exit(1);
                }
            }
	  i++;
        }
    }
}
