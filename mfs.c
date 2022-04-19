// The MIT License (MIT)
//
// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size



struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};


// read(buffer, size, 1, fp);
// write(buffer, size, 1, ofp);

FILE *fp = NULL; //file pointer
FILE *ofp = NULL; //output file pointer

struct DirectoryEntry dir[16];

bool delValid = false; //true if del was previous function

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your FAT32 functionality

    int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ )
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );
    }


    if(strcmp(token[0], "quit") == 0)
    {
      exit(0);
    }
    else if(strcmp(token[0], "open") == 0)
    {
      if(fp == NULL) //if no file is currently open
      {
        if((fp = fopen(token[1], "r")) != NULL)//if fopen succesfully opened the img
        {
          printf("File %s is now open.\n", token[1]);
        }
        else
        {
          printf("Error: File system image not found\n");
        }
      }
      else
      {
        printf("Error: File system image already open.\n");
      }
    }

    else if(strcmp(token[0], "close") == 0)
    {
      if(fp == NULL)
      {
        printf("Error: File system not open./n");
      }
      else
      {
        fclose(fp);
        printf("File successfully closed.\n");
        fp = NULL;
      }
    }

    else if(fp != NULL) //commands can only run if a file is open
    {
      if(strcmp(token[0], "undel") == 0)
      {
        if(delValid == true)
        {
          //undel
        }
        else
        {
          printf("Error: Can't undel last change.\n");
        }
      }
      delValid = false;

      if(strcmp(token[0],"read") == 0)
      {
        fseek(fp, atoi(token[2]), SEEK_SET);

      }
      else if(strcmp(token[0], "cd") == 0)
      {
        // fseek(fp,)
      }
      else if(strcmp(token[0], "info") == 0)
      {

      }
      else if(strcmp(token[0], "stat") == 0)
      {

      }
      else if(strcmp(token[0], "ls") == 0)
      {

      }
      else if(strcmp(token[0], "get") == 0)
      {
        //compare.c for string name token[1]
        // int cluster = dir[i].DIR_FirstClusterLow;
        // int offset = LBAtoOffset(cluster);
        // int size = dir[i].size;
        // fseek(fp, offset, SEEK_SET);
        // ofp = fopen(token[1], "w");
        //
        // while(size > 512) //might be BPB_BytesPerSec
        // {
        //   uint8_t buffer[512];
        //   read(buffer, 512, 1, fp);
        //   write(buffer, 512, 1, ofp);
        //   size = size - 512;
        //   cluster = NextLB(cluster);
        //   offset = LBAtooffset(cluster);
        //   fseek(fp, offset, SEEK_SET);
        //
        // }
        // if(size > 0)
        // {
        //   read(buffer, size, 1, fp);
        //   write(buffer, size, 1, ofp);
        //   fclose(ofp);
        // }
      }
      else if(strcmp(token[0], "del") == 0)
      {
        delValid = true;
      }

    }
    else
    {
      printf("Error: File system not open.\n");
    }

    free( working_root );

  }
  return 0;
}
