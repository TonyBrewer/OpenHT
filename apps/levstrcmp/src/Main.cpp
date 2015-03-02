/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <string>
#include <vector>
#include <iostream>

#include "Ht.h"
using namespace Ht;

#include "Main.h"

FILE *hitFp = 0;

int main(int argc, char *argv[])
{
#ifdef EXAMPLE
  return example(argc, argv);
#else
  int srtn = 0;

  extern int editdis(int argc, char *argv[]);

#if 0
  hitFp = fopen("mark_hit.txt", "w");
#endif

  srtn = editdis(argc,argv);

  return srtn;
}

#endif // EXAMPLE

