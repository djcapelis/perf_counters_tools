/***********************************************************************
 * event_count.c                                                       *
 *                                                                     *
 * Event Counting - Take a performance counter, record the data        *
 *                                                                     *
 * This is where the program lives                                     *
 * Pretty simple for now                                               *
 **********************************************************************/

/* Includes */
#define _GNU_SOURCE /* strnlen, etc */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdbool.h>
#include<limits.h>
#include<getopt.h>
#include<errno.h>
#include<string.h>
#include<libgen.h>

/* Includes from sys/ */
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>

/* Local includes */
#include"std_djc.h"

/* glibc doesn't have a wrapper for perf_event_open */
#include"perf_event_open.c"

/* Defaults */
#define DEFAULT_BUFFER_SIZE 512 * 1024 /* 512kb */
#define DEFAULT_FREQ 1000 /* 1000hz */
#define DEFAULT_FILENAME "events.count" /* Default filename */

/* Options */
bool OPT_H = false;
bool OPT_E = false;
bool OPT_U = false;
bool OPT_P = false;
bool OPT_F = false;
bool OPT_R = false;
bool OPT_O = false;
bool OPT_MK = false;

/* Functions */
void print_usage();
void err_msg(char * msg);
void perr_msg(char * msg);

/* As per tin */
void print_usage()
{
    fprintf(stderr, "Usage: event_count [options] [command]\n");
    fprintf(stderr, "\t-h Print usage\n");
    fprintf(stderr, "\t-e <hex> Event number (in hex)\n");
    fprintf(stderr, "\t-u <hex> Umask value (in hex)\n");
    fprintf(stderr, "\t-p <pid> Attach to pid <pid>\n");
    fprintf(stderr, "\t-f <hz> Frequency in times per second (default: 1000)\n");
/*  fprintf(stderr, "\t-r Run using highest realtime priority\n"); */ /* Unimplemented */
    fprintf(stderr, "\t-o <file> Output file (default: events.count)\n");
    fprintf(stderr, "\t-m <mb> mmap() buffer in megabytes\n");
    fprintf(stderr, "\t-k <kb> mmap() buffer in kilobytes (default: 512)\n");
}

/* Main program logic */
int main(int argc, char * argv[])
{
    int event = 0;
    int umask = 0;
    pid_t pid = 0;
    int freq = DEFAULT_FREQ;
    int mmap_region = DEFAULT_BUFFER_SIZE;
    char * default_filename = DEFAULT_FILENAME;
    char * out = default_filename;
    bool exit_error = false;

    /* Argument parsing */
    char opt;
    char * strerr = NULL;
    long arg;
    struct stat chkstat;
    int chk;
    while((opt = getopt(argc, argv, "+hre:u:f:p:o:m:k:")) != -1)
    {
        switch(opt)
        {
            /* Set realtime mode (UNIMPLEMENTED) */
            case 'r':
                err_msg("-r is unimplemented in this release.\n\n");
                break; /* unreached */

            /* Specify event code */
            case 'e':
                if(OPT_E)
                    err_msg("This program only monitors one event at a time for now, but two -e args specified!\n\n");
                OPT_E = true;
                arg = strtol(optarg, &strerr, 16);
                if(strerr[0] != 0)
                    err_msg("Unable to parse -e argument correctly, should be event code in hexadecimal\n\n");
                if(arg < 0 || arg > 0xFFF)
                    err_msg("Unable to parse -e argument, event code out of range 0x00-0xFFF\n\n");
                event = arg;
                optarg = NULL;
                break;
            
            /* Specify event code */
            case 'u':
                if(OPT_U)
                    err_msg("This version only monitors one event at a time for now, but two -u args specified!\n\n");
                OPT_U = true;
                arg = strtol(optarg, &strerr, 16);
                if(strerr[0] != 0)
                    err_msg("Unable to parse -u argument correctly, should be umask in hexadecimal\n\n");
                if(arg < 0 || arg > 0xFF)
                    err_msg("Unable to parse -u argument, umask out of range 0x00-0xFF\n\n");
                umask = arg;
                optarg = NULL;
                break;
    
            /* Specify pid to snapshot */
            case 'p':
                if(OPT_P)
                    err_msg("This version does not support more than one -p argument\n\n");
                OPT_P = true;
                arg = strtol(optarg, &strerr, 10);
                if(arg > INT_MAX || arg < 0 || strerr[0] != 0)
                    err_msg("Unable to parse -p argument correctly, should be a pid\n\n");
                pid = arg;
                optarg = NULL;
                break;
    
            /* Specify frequency */
            case 'f':
                if(OPT_F)
                    fprintf(stderr, "-f already specified, ignoring previous value...\n\n");
                OPT_F = true;
                arg = strtol(optarg, &strerr, 10);
                if(strerr[0] != 0)
                    err_msg("Unable to parse -f argument correctly, should be a frequency in whole numbers of hz\n\n");
                if(arg < 1)
                    err_msg("Unable to parse -f argument correctly, frequency must be greater than 1\n\n");
                if(arg > 100000)
                    err_msg("Unable to parse -f argument correctly, frequencies above 100000hz are unsupported\n\n");
                freq = arg;
                optarg = NULL;
                break;
    
            /* Specify output file */
            case 'o':
                if(OPT_O)
                    err_msg("-o already specified\n\n");
                OPT_O = true;
                arg = stat(dirname(optarg), &chkstat);
                if(arg == -1 && errno == ENOENT)
                    err_msg("Unable to parse -o argument correctly, invalid path to file\n\n");
                if(arg == -1)
                    perr_msg("Unable to parse -o argument correctly: ");
                out = optarg;
                optarg = NULL;
                break;
    
            /* Specify mmap() region in megabytes or kilobytes */
            case 'm':
            case 'k':
                if(OPT_MK)
                    fprintf(stderr, "mmap region size already specified in previous -m or -k, ignoring previous...\n\n");
                OPT_MK = true;
                arg = strtol(optarg, &strerr, 10);
                if(arg > INT_MAX || arg < 0 || strerr[0] != 0)
                {
                    if(opt == 'm')
                        err_msg("Unable to parse -m argument correctly, should be mmap buffer size in megabytes\n\n");
                    else
                        err_msg("Unable to parse -k argument correctly, should be mmap buffer size in kilobytes\n\n");
                }
                if(opt == 'm')
                    arg *= 1024;
                if(arg > 128 * 1024)
                    err_msg("mmap buffer specified is larger than 128mb.  That seems large, don't you think?  I refuse to be party to this.\n\n");
                mmap_region = arg * 1024;
                optarg = NULL;
                break;
    
            /* Print usage */
            case 'h':
            default:
                print_usage();
                return 0;
        }
    }

    /* Option validity checks */
    if(!OPT_E)
        err_msg("No event code specified!\n\n");
    if(!OPT_U)
        err_msg("No umask specified!\n\n");

    /* Start a process if specified on the command line */
    if(argc > optind)
    {
        if(OPT_P)
            err_msg("pid already specified, cannot also specify a process to run!\n\n");

        /* Fork */
        pid_t ret = 0;
        ret = fork();
        err_chk(ret == -1);

        /* Which side of the fork are we on? */
        if(ret == 0) /* We are the child */
        {
            int i;
            char ** arglist;
            arglist = calloc((argc - optind) + 1, sizeof(char *)); /* Enough for all the args, plus a null */
            for(i = optind;i <= argc;i++)
                arglist[i - optind] = argv[i];
            arglist[argc - optind] = NULL;
            chk = execvp(argv[optind], arglist); /* Never returns */
            err_chk(chk == -1);
        }
        else /* We are the parent */
        {
            pid = ret;
            OPT_P = true;
        }
    }

    /* Check for pid */
    if(!OPT_P)
        err_msg("No process specified!\n\n");

    /* Configure perf_event_attr */
    struct perf_event_attr e;
    memset(&e, 0, sizeof(struct perf_event_attr));
    e.type = PERF_TYPE_RAW;
    e.size = sizeof(struct perf_event_attr);
    e.config = event | umask << 8;
    e.freq = 1;
    e.sample_freq = freq;
    e.sample_type = PERF_SAMPLE_TID | PERF_SAMPLE_READ;
    e.inherit = 1;
    e.pinned = 1;
    e.exclude_user = 0;
    e.exclude_kernel = 0;
    e.exclude_hv = 1;
    e.exclude_idle = 1;

    /* Initialize performance events */
    int pe_fd;
    pe_fd = perf_event_open(&e, pid, -1, -1, 0);
    err_chk(pe_fd == -1);

    /* Cleanup and exit cleanly */
    goto cleanup;

/* Error handler */
err:
    perror("event_count");
    exit_error = true;

/* Cleanup anything that needs to be cleaned up */
cleanup:
    if(pe_results)
        free(pe_results);

/* Exit */
    if(exit_error)
        exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

/* Error message handler */
void err_msg(char * msg)
{
    fprintf(stderr, msg);
    print_usage();
    exit(EXIT_FAILURE);
}

/* Error message handler using perror */
void perr_msg(char * msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
