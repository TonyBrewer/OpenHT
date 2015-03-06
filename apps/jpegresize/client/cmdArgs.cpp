/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <getopt.h>
#include "job.hpp"
#include "cmdArgs.hpp"

//Hitting a command line argument that has multiple values associated with it will
//trigger a state machine that controls how the next argumenst(s) are handled.
//These are the various states for the state machine.
enum parseArgsState {
  argStateNone=0,
  argStateSize,
  argStateAspect
};

void printHelp(void);
void printVersion(void);

bool parseArgs(job &args, int argc, char *argv[]) {
  int longIndex;
  cmdLineArg opt;
  //We are going to handle the mode bitmask with a local copy.  Once we have processed
  //all the arguments, we can override the mode in the passed job definition as appropriate
  uint32_t mode=0;
  char *endptr;
  //Initilize the argument handling state machine to an idle state.
  parseArgsState argState=argStateNone;
  //If the --keep_aspect argument is specfied then we will capture the optional dimension
  //argument as well
  char aspect=0;

  // set default modes
  mode|=MODE_KEEP_ASPECT;

  //Enable display of invalid argument messages
  opterr=1;
  //Since we may be processing batch mode options, we need to make sure to reset the getopt_long
  //state to the start
  optind=1;
  //Loop while there are still arguments to process.  The various case blocks should be pretty
  //self explanitory.  In cases where integer values as expected, the string to integer conversion
  //is performed and checked to ensure a value numeric value was given.  Each recognized 
  //argument will reset the state machine to idle unless that particular argument expects more
  //that one value.
  while((opt=(cmdLineArg)getopt_long(argc, argv, shortOpts, longOpts, &longIndex ))!=-1) {
    switch(opt) {
      case argVersion:
	printVersion();
	return false;
	break;
      case argUnknown:
      case argHelp:
	printHelp();
	return false;
	break;
      case argServer:
	argState=argStateNone;
	args.server=optarg;
	break;
      case argPort:
	argState=argStateNone;
	args.port=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid port: %s\n", optarg);
	  printHelp();
	  return false;
	} else if (args.port==0) {
	  printf("--port requires a numeric argument\n");
	  printHelp();
	  return false;	  
	}
	break;
      case argNoUpscale:
	argState=argStateNone;
	mode|=MODE_NO_UPSCALE;
	break;
      case argServerRead:
	argState=argStateNone;
	mode|=MODE_SERVER_READ;
	break;
      case argServerWrite:
	argState=argStateNone;
	mode|=MODE_SERVER_WRITE;
	break;
      case argQuality:
	argState=argStateNone;
	args.quality=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid quality value: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (args.quality==0) {
	  printf("--quality requires a numeric argument\n");
	  printHelp();
	  return false;	  
	}	
	break;
      case argScale:
	argState=argStateNone;
	args.scale=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid scale value: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (args.scale==0) {
	  printf("--scale requires a numeric argument\n");
	  printHelp();
	  return false;	  
	}
	break;
      case argSize:
	//If --size is specified then we need an X and Y value.
	//X comes first and will the the string pointed at by
	//optarg.  We convert it first and the set the state
	//machine to handle the Y value next.
	args.sizeX=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid X size: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (args.sizeX==0) {
	  printf("--size requires numeric argumensts\n");
	  printHelp();
	  return false;	  
	}
	argState=argStateSize;
	break;
      case argKeepAspect:
	// 24-Feb-2015:  change default for keep_aspect to true.  This option now
	// requires an x or y argument and specifies which dimension to scale to,
	// preserving aspect ratio for the other one.  If n is specified do not
	// preserve aspect
	argState=argStateNone;
	mode|=MODE_KEEP_ASPECT;
	aspect=*optarg;
        if(aspect == 'n') {
	  mode &= ~MODE_KEEP_ASPECT;
        } else if(aspect != 'x' && aspect !='y') {
	  printf("Please specify x or y to scale by horizontal or vertical size,\n");
	  printf("or n to disable aspect preservation\n");
	  printHelp();
	  return false;	    
	}
	break;
      case argThreads:
	argState=argStateNone;
	args.threads=strtol(optarg, &endptr,0);
	if(*endptr) {
	  printf("Invalid thread count: %s\n", optarg);
	  printHelp();
	  return false;	  
	} else if (args.threads==0) {
	  printf("--threads requires a numeric argument\n");
	  printHelp();
	  return false;	  
	} else if (args.threads>192) {
	  printf("Maximum number of threads allowed is 192\n");
	  return false;
	}
	break;
      case argBatch:
	argState=argStateNone;
	args.batch=1;
	break;
      case argMagick:
	argState=argStateNone;
	mode|=MODE_MAGICK;
	break;
      case argStats:
	argState=argStateNone;	
	args.stats=1;
	break;
      case argMisc:
	//all cmd line arguments that don't start with '-' make it here.  This state machine tries
	//to do the correct thing with them based on the current state of command line processing.
	switch(argState) {
	  case argStateSize:
	    //We have gotten the --size argument and the 'X' value.  Now we should have the 'Y'
	    //value as well.  Do the conversion and check to make sure it was a valid numeric
	    //argument.
	    args.sizeY=strtol(optarg, &endptr,0);
	    if(*endptr) {
	      printf("Invalid Y size: %s\n", optarg);
	      printHelp();
	      return false;		
	    } else if (args.sizeY==0) {
	      printf("--size requires numeric argumensts\n");
	      printHelp();
	      return false;		  
	    }
	    break;
	  case argStateAspect:
	    //We have gotten the --keep_aspect option is a format such the getopt_long didn't
	    //recognize the value.  Now we should have the value as well.  
	    aspect=*optarg;
	    if(aspect != 'x' && aspect !='y') {
	      printf("Please specify x or y dimension for aspect calculations\n");
	      printHelp();
	      return false;
	    }	      
	    break;
	  default:
	    //We have encountered a command line argument value that doesn't below to a defined option.
	    //We will take the first one as the input file and the second as the output file.  Any after
	    //that will generate an error message.  Note that if we are running in batch mode, the batch
	    //input file will be stored in inputFile.
	    if(args.inputFile.empty()) {
	      args.inputFile=optarg;
	    } else if (args.outputFile.empty()) {
	      args.outputFile=optarg;
	    } else {
	      printf("uknown option: %s\n", optarg);
	      printHelp();
	      return false;
	    }
	    break;
	}
	//make sure and reset the state machine on exit.
	argState=argStateNone;
	break;
	  default:
	    break;
    }
  }
  //Store the job mode
  //Need to add override logic if the job definition passed in was already initialized with values as would
  //happen in the argment list we are processing came from a line in the batch mode input file.
  args.jobMode|=mode;
  //Specifying a fixed size will override a scale argument if both are specified.  We could error out here 
  //but for now we just print a warning to let the user know what happened.
  if((args.sizeX || args.sizeY) && args.scale) {
    fprintf(stderr, "warning:  specifying -size will override -scale\n");
    args.scale=0;
  }
  //If we are keeping the apect ratio and have a defined dimension to use as the base for calculation then
  //we go ahead an zero out the other.  The server looks at the values of X and Y to determine which to use
  //for calculation.  If both are set then it will fit the image into the bounding box specified by 
  //X and Y.  If only one is set then that one is used for the base.  If n is specified we will scale
  //in each direction to the specified size and will not preserve the aspect ratio of the original.
  
  if(args.jobMode & MODE_KEEP_ASPECT) {
    if(aspect=='x') args.sizeY=0;
    if(aspect=='y') args.sizeX=0;
  }
  //In batch mode, the output file, if specified, has no meaning and is ignored.
  //
  // Fix this!  When we are processing batch file lines, the output file does have meaning.  Only when 
  //processing the global arguments is the output file ignored.  Can we detect this?
  if(args.batch && !args.outputFile.empty()) {
    fprintf(stderr, "warning: output file will be ignored in batch mode: %s\n", args.outputFile.c_str());
    args.outputFile.clear();
  }
  return true;
}

void printHelp(void) {
  printf("jrcl --server <name> [options...] <infile> <outfile>\n");
  printf("jrcl --server <name> [options...] --batch <file>\n");
  printf("     --version              print version info\n");
  printf("     --help                 print this help\n");
  printf("     --server <name>        name or ip address of server (required)\n");
  printf("     --port <num>           TCP port server is listening on (default 18811)\n");
  printf("     --server_read          send pathnames of input images for server to read directly\n");
  printf("     --server_write         server writes resized images directly\n");
//klc not currently implemented
//  printf("     --quality              quality of output image (default 85)\n");
  printf("     --scale <n>            resize to n%% of original image (integer 1-100)\n");
  printf("     --size <x> <y>         resize to x by y pixels (minimum 16x8)\n");
  printf("     --no_upscale           images that are smaller than the output size are not resized\n");
  printf("     --keep_aspect [x|y|n]    x or y forces output image to corresponding dimension,\n");
  printf("                            scaling the other dimension to preserve aspect.\n");
  printf("                            n forces both dimensions (aspect is not preserved).\n");
  printf("     --threads <n>          number of threads\n");
  printf("     --batch <file>         read a jobs file with resize requests\n");
//klc undocumented option
//  printf("     --magick               use ImageMagick libraries to process images instead of\n");
//  printf("                            sending to the daemon\n");
  printf("\n");
}

void printVersion(void) {
  printf("version 1.2.0\n");
}

void printArgs(job &args) {
  printf(" server: %s\n", args.server.c_str());
  printf("   port: %d\n", args.port);
  printf("quality: %d\n", args.quality);
  printf("  scale: %d\n", args.scale);
  printf("   size: x=%d y=%d\n", args.sizeX, args.sizeY);
  printf("threads: %d\n", args.threads);
  printf("  batch: %d\n", args.batch);
  printf("  stats: %d\n", args.stats); 
  printf(" infile: %s\n", args.inputFile.c_str());
  printf("outfile: %s\n", args.outputFile.c_str());
  printf("operation mode:\n");
  if(!args.jobMode) printf("  default\n");
  if(args.jobMode & MODE_SERVER_READ) printf("  server_read\n");
  if(args.jobMode & MODE_SERVER_WRITE) printf("  server_write\n");
  if(args.jobMode & MODE_KEEP_ASPECT) printf("  keep_aspect\n");
  if(args.jobMode & MODE_MAGICK) printf("  magick\n");
  printf("\n");
  return;
}
