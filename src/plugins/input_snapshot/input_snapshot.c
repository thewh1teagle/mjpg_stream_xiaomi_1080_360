/*******************************************************************************
#                                                                              #
#      MJPG-streamer allows to stream JPG frames from an input-plugin          #
#      to several output plugins                                               #
#                                                                              #
#      Copyright (C) 2007 Tom Stöveken                                         #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; version 2 of the License.                      #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ev.h>

#include "../../mjpg_streamer.h"
#include "../../utils.h"

#define INPUT_PLUGIN_NAME "snapshot input plugin"

/* private functions and variables to this plugin */
static pthread_t   worker;
static globals     *pglobal;

void *worker_thread(void *);
void worker_cleanup(void *);
void help(void);

static int delay = 1000;
static int plugin_number;
static size_t buf_size = 0;

/*** plugin interface functions ***/
int input_init(input_parameter *param, int id)
{
    int i;
    plugin_number = id;

    param->argv[0] = INPUT_PLUGIN_NAME;

    /* show all parameters for DBG purposes */
    for(i = 0; i < param->argc; i++) {
        DBG("argv[%d]=%s\n", i, param->argv[i]);
    }

    reset_getopt();
    while(1) {
        int option_index = 0, c = 0;
        static struct option long_options[] = {
            {"h", no_argument, 0, 0},
            {"help", no_argument, 0, 0},
            {"d", required_argument, 0, 0},
            {"delay", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        c = getopt_long_only(param->argc, param->argv, "", long_options, &option_index);

        /* no more options to parse */
        if(c == -1) break;

        /* unrecognized option */
        if(c == '?') {
            help();
            return ;
        }

        switch(option_index) {
            /* h, help */
        case 0:
        case 1:
            DBG("case 0,1\n");
            help();
            return 1;
            break;

            /* d, delay */
        case 2:
        case 3:
            DBG("case 2,3\n");
            delay = atoi(optarg);
            break;

        default:
            DBG("default case\n");
            help();
            return 1;
        }
    }

    pglobal = param->global;

    IPRINT("forced delay......: %i ms\n", delay);

    param->global->in[id].name = malloc((strlen(INPUT_PLUGIN_NAME) + 1) * sizeof(char));
    sprintf(param->global->in[id].name, INPUT_PLUGIN_NAME);
    
    pglobal->in[id].num_clients = 0;
    
    return 0;
}

int input_stop(int id)
{
    DBG("will cancel input thread\n");
    pthread_cancel(worker);
    return 0;
}

int input_run(int id)
{
    pglobal->in[id].buf = NULL;

    if(pthread_create(&worker, 0, worker_thread, NULL) != 0) {
        free(pglobal->in[id].buf);
        fprintf(stderr, "could not start worker thread\n");
        exit(EXIT_FAILURE);
    }

    pthread_detach(worker);

    return 0;
}

/*** private functions for this plugin below ***/
void help(void)
{
    fprintf(stderr, " ---------------------------------------------------------------\n" \
    " Help for input plugin..: "INPUT_PLUGIN_NAME"\n" \
    " ---------------------------------------------------------------\n" \
    " The following parameters can be passed to this plugin:\n\n" \
    " [-d | --delay ]........: delay to pause between frames in ms\n" \
    " ---------------------------------------------------------------\n");
}

static struct ev_async eva = { 0 };
static struct ev_loop * ev_loop_Ptr= 0;
static unsigned long hdl_VideoMainstream= 0;

static void interrupt_handler(int p)
{
  //printf("interrupt called\n");
   
   ev_async_send( ev_default_loop(0), &eva);
}

static void clock_cb (struct ev_loop *loop, ev_periodic *w, int revents)
{
  if(pglobal->stop) {
    interrupt_handler(2);
    return;
  }

  pthread_mutex_lock(&pglobal->in[plugin_number].mnclients);
  if(pglobal->in[plugin_number].num_clients) {
    int r = shbfev_rcv_send_message(hdl_VideoMainstream, "hello", 5);
    DBG("send_len is %d\n", r);    
  }
  pthread_mutex_unlock(&pglobal->in[plugin_number].mnclients);
}

static void on_recv_photo_stream(unsigned long Parm1,unsigned long *Parm2)
{
  size_t filesize;
  struct timeval timestamp;
  
  DBG("Receiver FrameInfo: %d %d %d %llu\n",
      *Parm2, Parm2[1], Parm2[4], Parm2[2], Parm2[3]);

  //data is at param2 + 8, size is Parm2[4]  

  filesize = Parm2[4];

  /* copy frame from file to global buffer */

  pthread_mutex_lock(&pglobal->in[plugin_number].db);

  /* free &  allocate memory for nee frame */
  
   //try to  be smart and (de)allocate only when buf_size < filesize
#if 0  
  if(pglobal->in[plugin_number].buf != NULL)
    free(pglobal->in[plugin_number].buf);

  pglobal->in[plugin_number].buf = malloc(filesize + (1 << 10));
#else
  if(buf_size < filesize) {    
    if(pglobal->in[plugin_number].buf != NULL)
      free(pglobal->in[plugin_number].buf);

    buf_size = filesize + (1 << 16);
    pglobal->in[plugin_number].buf = malloc(buf_size);
  }  
#endif
  
  if(pglobal->in[plugin_number].buf == NULL) {
    fprintf(stderr, "could not allocate memory\n");
    shbf_free(Parm2);
    return;
  }
  
  pglobal->in[plugin_number].size = filesize;
  memcpy(pglobal->in[plugin_number].buf, Parm2 + 8, filesize);
  
  gettimeofday(&timestamp, NULL);
  pglobal->in[plugin_number].timestamp = timestamp;
  DBG("new frame copied (size: %d)\n", pglobal->in[plugin_number].size);

  /* signal fresh_frame */
  pthread_cond_broadcast(&pglobal->in[plugin_number].db_update);
  pthread_mutex_unlock(&pglobal->in[plugin_number].db);

  shbf_free(Parm2);
}

static void receiver_closed_cb(int *piParm1)
{
    DBG("receiver closed !\n");
}

static void ev_cleanup_cb(void)
{
  //fprintf(stderr, "cleanup called\n");

  ev_break(ev_default_loop(0), 2);
}

static void main_func()
{
  struct sigaction sa;
  struct ev_periodic tick;
 
  ev_periodic_init(&tick, clock_cb, 0., delay/1000.0, 0); //10 sec 
  
  ev_loop_Ptr = ev_default_loop(0);
  unsigned long var0 = 0;
 
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = interrupt_handler;
  
  sigaction(SIGINT, &sa, (struct sigaction *)0);

  hdl_VideoMainstream = shbfev_rcv_create(ev_default_loop(0),"/run/jpeg_snap");
  shbfev_rcv_event(hdl_VideoMainstream,2,on_recv_photo_stream,&var0);
  shbfev_rcv_event(hdl_VideoMainstream,1,receiver_closed_cb,&var0);
  shbfev_rcv_start(hdl_VideoMainstream);
  
  ev_periodic_start(ev_loop_Ptr, &tick);


  ev_async_init (&eva, ev_cleanup_cb);
  ev_async_start(ev_loop_Ptr, &eva);
  
  ev_run(ev_loop_Ptr,0);
  
  DBG("ev_run loop stop");

  shbfev_rcv_destroy(hdl_VideoMainstream);
  ev_loop_destroy(ev_default_loop(0));
}


/* the single writer thread */
void *worker_thread(void *arg)
{
    char buffer[1<<16];
    int file;
    size_t filesize = 0;
    struct stat stats;
    struct dirent **fileList;
    int fileCount = 0;
    int currentFileNumber = 0;
    char hasJpgFile = 0;
    struct timeval timestamp;

    /* set cleanup handler to cleanup allocated ressources */
    pthread_cleanup_push(worker_cleanup, NULL);

    main_func();
    
thread_quit:

    DBG("leaving input thread, calling cleanup function now\n");
    /* call cleanup handler, signal with the parameter */
    pthread_cleanup_pop(1);

    return NULL;
}

void worker_cleanup(void *arg)
{
    static unsigned char first_run = 1;

    if(!first_run) {
        DBG("already cleaned up ressources\n");
        return;
    }

    first_run = 0;
    DBG("cleaning up ressources allocated by input thread\n");

    if(pglobal->in[plugin_number].buf != NULL) free(pglobal->in[plugin_number].buf);

}

