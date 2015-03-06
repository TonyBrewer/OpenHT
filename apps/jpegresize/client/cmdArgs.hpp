#pragma once

#include <getopt.h>
#include "job.hpp"

bool parseArgs(job &args, int argc, char *argv[]);
void printArgs(job &args);
  
//This enum defines our command line arguments.  For arguments that have a long
//but not short option, the enumerated value is left to be automatically assigned
//otherwise, the short option is used as the value. Note that "argUnknown" is not 
//actually included for command line parsing, but is included in the enum 
//for completeness.  argMisc is used for arguments that don't begin with an option
//character.
enum cmdLineArg {
  argMisc=1,
  argVersion='v',
  argHelp='h',
  argServer='s',
  argPort='p',
  argNoUpscale='n',
  argServerRead='r',
  argServerWrite='w',
  argQuality='q',
  argScale='c',
  argSize='z',
  argKeepAspect='k',
  argThreads='t',
  argBatch='b',
  argUnknown='?',
  argMagick,
  argStats,
};

//Define the options to be parsed by getopt_long.
const struct option longOpts[] = {
    {"version",   	no_argument,        0, argVersion},
    {"help",      	no_argument,        0, argHelp},
    {"server",     	required_argument,  0, argServer},
    {"port",     	required_argument,  0, argPort},
    {"no_upscale",	no_argument,        0, argNoUpscale},
    {"server_read",     no_argument,	    0, argServerRead},
    {"server_write",    no_argument,  	    0, argServerWrite},
    {"quality",     	required_argument,  0, argQuality},
    {"scale",     	required_argument,  0, argScale},
    {"size",     	required_argument,  0, argSize}, 
    {"keep_aspect",     required_argument,  0, argKeepAspect},    
    {"threads",     	required_argument,  0, argThreads},
    {"batch",     	no_argument,  	    0, argBatch},
    {"magick",		no_argument, 	    0, argMagick},
    {"stats",		no_argument,	    0, argStats},
    {0,0,0,0},
};

//Initialize the short option string with all the arguments that have a short
//option.  Using the enumerated values allows us to change the short option
//character without having to go edit a whole bunch of code in different places.
//We also specify that arguments not starting with '-' will be returned as
//option code 1.
const char shortOpts[]={
  '-',
  argVersion,
  argHelp,
  argServer,':',
  argPort,':',
  argNoUpscale,
  argServerRead,
  argServerWrite,
  argQuality,':',
  argScale,':',
  argSize,':',
  argKeepAspect,':',':',
  argThreads,':',
  argBatch,
  argMagick,
  argStats,
};


 
