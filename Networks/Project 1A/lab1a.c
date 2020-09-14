// NAME: Cindi Dong
// EMAIL: cindidong@gmail.com
// ID: 405126747

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/wait.h>

struct termios saved;
int PID;
int p2c[2];  //pipe from parent to child (to shell)
int c2p[2]; //pipe from child to parent (from shell)
int eof_recieved;

//SIGPIPE handler
void handler(int signum)
{
  if (signum == SIGPIPE)
    {
      eof_recieved = 1;
    }
}

//reset back into canonical
void reset_mode()
{
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


int main(int argc, char **argv) {
  int shell_flag = 0;
  char* command_name = NULL;
  int c;
    struct option long_options[] =
      {
        {"shell", required_argument, 0, 's'},
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
	  default:
	    fprintf(stderr, "Error: Needs to be in the form: lab1a [--shell=command]");
	    exit(1);
	    break;
	  }
      }
    
    //setting to noncanonical
    set_mode();
    
    //shell argument
    if (shell_flag == 1)
      {
        signal(SIGPIPE, handler);
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
            
            struct pollfd poll_list[2];
            poll_list[0].fd = STDIN_FILENO;
            poll_list[1].fd = c2p[0];
            poll_list[0].events = POLLIN | POLLHUP | POLLERR;
            poll_list[1].events = POLLIN | POLLHUP | POLLERR;
            int poll_status;
            int p2c_closed = 0;
            eof_recieved = 0;
            while (1)
	      {
                poll_status = poll(poll_list, 2, 0);
                if (poll_status < 0)
		  {
                    fprintf(stderr, "Polling error: %s\n", strerror(errno));
                    exit (1);
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
                        int i = 0;
                        while (i < read_status)
			  {
                            char c = b[i];
                            //escape case
                            if (c == 0x03)
			      {
                                char a[2];
                                a[0] = '^';
                                a[1] = 'C';
                                write_status = write(STDOUT_FILENO, a, 2);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
                                if (kill(PID, SIGINT) < 0)
				  {
                                    fprintf(stderr, "Kill child process error: %s\n", strerror(errno));
                                    exit(1);
				  }
			      }
                            //eof case
                            else if (c == 0x04)
			      {
                                char a[4];
                                a[0] = '\r';
                                a[1] = '\n';
                                a[2] = '^';
                                a[3] = 'D';
                                write_status = write(STDOUT_FILENO, a, 4);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
                                //close parent to child (to shell) pipe
                                if (p2c_closed == 0)
				  {
                                    close(p2c[1]);
                                    p2c_closed = 1;
				  }
			      }
                            //returns case
                            else if (c == '\n' || c == '\r')
			      {
                                char a[2];
                                a[0] = '\r';
                                a[1] = '\n';
                                write_status = write(STDOUT_FILENO, a, 2);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
                                //only send newline to shell
                                write_status = write(p2c[1], a + 1, 1);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
			      }
                            else
			      {
                                write_status = write(STDOUT_FILENO, &c, 1);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
                                //send to shell as well
                                write_status = write(p2c[1], &c, 1);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
			      }
                            i++;
			  }
		      }
                    //input from shell/child
                    if ((poll_list[1].revents & POLLIN) == POLLIN)
		      {
                        char b[256];
                        int read_status;
                        int write_status;
                        read_status = read(c2p[0], b, 256);
                        if (read_status < 0)
			  {
                            fprintf(stderr, "Read error: %s\n", strerror(errno));
                            exit(1);
			  }
                        int i = 0;
                        while (i < read_status)
			  {
                            char c = b[i];
                            //eof case
                            if (c == 0x04)
			      {
                                char a[4];
                                a[0] = '\r';
                                a[1] = '\n';
                                a[2] = '^';
                                a[3] = 'D';
                                write_status = write(STDOUT_FILENO, a, 4);
                                if (write_status < 0)
				  {
				    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
                                if (eof_recieved == 0)
				  {
                                    eof_recieved = 1;
				  }
			      }
                            //newline case
                            else if (c == '\n')
			      {
                                char a[2];
                                a[0] = '\r';
                                a[1] = '\n';
                                write_status = write(STDOUT_FILENO, a, 2);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
			      }
                            else
			      {
                                write_status = write(STDOUT_FILENO, &c, 1);
                                if (write_status < 0)
				  {
                                    fprintf(stderr, "Write error: %s\n", strerror(errno));
                                    exit(1);
				  }
			      }
                            i++;
			  }
		      }
                    //error in the stdin
                    if (((poll_list[0].revents & POLLHUP) == POLLHUP) || ((poll_list[0].revents & POLLERR) == POLLERR))
		      {
                        eof_recieved = 1;
		      }
                    //error in the shell/child
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
            int wait_status;
            if (waitpid(PID, &wait_status, 0) < 0)
	      {
                fprintf(stderr, "Child process waiting error: %s\n", strerror(errno));
                exit (1);
	      }
            fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n", WTERMSIG(wait_status), WEXITSTATUS(wait_status));
            exit(0);
	  }
      }

    //no shell argument, stdin as normal
    else
      {
        char b[256];
        int read_status;
        int write_status;
        read_status = read(STDIN_FILENO, b, 256);
        while (read_status > 0)
	  {
            int i = 0;
            while (i < read_status)
	      {
                char c = b[i];
                //eof case
                if (c == 0x04)
		  {
                    char a[4];
                    a[0] = '\r';
                    a[1] = '\n';
                    a[2] = '^';
                    a[3] = 'D';
                    write_status = write(STDOUT_FILENO, a, 4);
                    if (write_status < 0)
		      {
                        fprintf(stderr, "Write error: %s\n", strerror(errno));
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
                    write_status = write(STDOUT_FILENO, a, 2);
                    if (write_status < 0)
		      {
                        fprintf(stderr, "Write error: %s\n", strerror(errno));
                        exit(1);
		      }
		  }
                else
		  {
                    write_status = write(STDOUT_FILENO, &c, 1);
                    if (write_status < 0)
		      {
                        fprintf(stderr, "Write error: %s\n", strerror(errno));
                        exit(1);
		      }
		  }
                i++;
	      }
            read_status = read(STDIN_FILENO, b, 256);
	  }
        if (read_status < 0)
	  {
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            exit(1);
	  }
      }
    exit(0);
}
