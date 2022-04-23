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
#include <stdint.h>
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 4

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

//method declarations
int16_t NextLB(uint32_t sector);
int LBAtoOffset(int32_t sector);
void initFAT32();

void FAT32info();
void FAT32get(char* name);
void FAT32stat(char* name);
void FAT32cd(char* name);
void FAT32ls();
void FAT32read(char* name, int offset, int numOfBytes);
void FAT32del(char* name);
void FAT32undel(char* name);
bool compare(char* name, char* dirName);

//global variables
FILE *fp = NULL; //file pointer
FILE *ofp = NULL; //output file pointer
uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATs;
uint32_t BPB_FATSz32;
uint8_t buffer[512];
int last_offset = 0x100400;



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

    if(token[0] == NULL)
    {
        continue;
    }

    if(strcmp(token[0], "quit") == 0)
    {
        if(fp != NULL)
          fclose(fp);

      exit(0);
    }
    else if(strcmp(token[0], "open") == 0)
    {
      if(fp == NULL) //if no file is currently open
      {
        if((fp = fopen(token[1], "r")) != NULL)//if fopen succesfully opened the img
        {
          initFAT32();
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
        FAT32undel(token[1]);
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
        //read hamlet.txt 1 2
        //token[1] file file name
        //token[2] offset;
        //token[3] # of bytes

        FAT32read(token[1], atoi(token[2]), atoi(token[3]));

      }
      else if(strcmp(token[0], "cd") == 0)
      {
        FAT32cd(token[1]);
        // fseek(fp,)
        // int entry = findFile(token[1]);
        // int cluster = dir[i].DIR_FirstClusterLow;
        // int offset = LBAtoOffset(cluster);
        // fseek(fp, offset, SEEK_SET);
        // fread(dir, sizeof(dir), 16, fp);

      }
      else if(strcmp(token[0], "info") == 0)
      {
        FAT32info();

      }
      else if(strcmp(token[0], "stat") == 0)
      {
        FAT32stat(token[1]);
      }
      else if(strcmp(token[0], "ls") == 0)
      {
        FAT32ls();
      }
      else if(strcmp(token[0], "get") == 0)
      {
        FAT32get(token[1]);
        // compare.c for string name token[1]
        // int cluster = dir[i].DIR_FirstClusterLow;
        // int offset = LBAtoOffset(cluster);
        // int size = dir[i].size;
        // fseek(fp, offset, SEEK_SET);
        // ofp = fopen(token[1], "w");
        // uint8_t buffer[512];
        // while(size > 512) //might be BPB_BytesPerSec
        // {
        //
        //   fread(buffer, 512, 1, fp);
        //   fwrite(buffer, 512, 1, ofp);
        //   size = size - 512;
        //   cluster = NextLB(cluster);
        //   offset = LBAtooffset(cluster);
        //   fseek(fp, offset, SEEK_SET);
        //
        // }
        // if(size > 0)
        // {
        //   fread(buffer, size, 1, fp);
        //   fwrite(buffer, size, 1, ofp);
        //   fclose(ofp);
        // }
      }
      else if(strcmp(token[0], "del") == 0)
      {
        FAT32del(token[1]);
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

void initFAT32()
{
  fseek(fp, 11, SEEK_SET); //BPB_BytesPerSec offset 11 2 bytes
  fread(&BPB_BytesPerSec, 2, 1, fp);
  fseek(fp, 13, SEEK_SET); //BPB_SecPerClus offset 13 1 bytes
  fread(&BPB_SecPerClus, 1, 1, fp);
  fseek(fp, 14, SEEK_SET); //BPB_BytesPerSec offset 14 2 bytes
  fread(&BPB_RsvdSecCnt, 14, 2, fp);
  fseek(fp, 16, SEEK_SET); //BPB_NumFATs offset 16 1 bytes
  fread(&BPB_NumFATs, 1, 1, fp);
  fseek(fp, 36, SEEK_SET); //BPB_FATSz32 offset 36 4 bytes
  fread(&BPB_FATSz32, 4, 1, fp);

  // get Directory entries
  fseek(fp, 0x100400, SEEK_SET);
  fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);
  for(int i = 0; i < 16; i++)
  {
      printf("Filename: %s\n", dir[i].DIR_Name);
  }

}

int16_t NextLB(uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

int LBAtoOffset(int32_t sector)
{
    // standard for ../
    if(sector == 0)
    {
        sector = 2;
    }
  return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}


void FAT32info()
{
  //prints out info about fat32 to terminal
  printf("BPB_BytesPerSec:\t %d\t%x\n", BPB_BytesPerSec, BPB_BytesPerSec);
  printf("BPB_SecPerClus: \t %d\t%x\n", BPB_SecPerClus, BPB_SecPerClus);
  printf("BPB_RsvdSecCnt: \t %d\t%x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
  printf("BPB_NumFATs:    \t %d\t%x\n", BPB_NumFATs, BPB_NumFATs);
  printf("BPB_FATSz32:    \t %d\t%x\n", BPB_FATSz32, BPB_FATSz32);
}


void FAT32get(char* name)
{
    bool found = false;
    for(int i = 0; i < 16; i++)
    {
        if(compare(name, dir[i].DIR_Name))
        {
            FILE *ofp;
            ofp = fopen(name,"w");
            printf("poop\n");
            int cluster = dir[i].DIR_FirstClusterLow;
            int offset = LBAtoOffset(cluster);
            int size = dir[i].DIR_FileSize;
            fseek(fp, offset, SEEK_SET);
            while(size >= 512)
            {
                printf("poop\n");
               fread(buffer, 512, 1, fp);
               fwrite(buffer, 512, 1, ofp);
               size = size - 512;
               cluster = NextLB(cluster);
               offset = LBAtoOffset(cluster);
               fseek(fp, offset, SEEK_SET);

            }
            if(size > 0)
            {
               fread(buffer, size, 1, fp);
               fwrite(buffer, size, 1, ofp);
            }
            fclose(ofp);

            found = true;
            break;

        }

    }
    if(!found)
    {
        printf("Error: File not found.\n");
    }

}
void FAT32stat(char* name)
{
    bool found = false;
    for(int i = 0; i < 16; i++)
    {
        if(compare(name, dir[i].DIR_Name))
        {
            printf("%-20s %-15s %-25s\n","File Attribute","Size", "Starting Cluster Number");
            printf("%-20d %-15d %-25d\n",dir[i].DIR_Attr,
                dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
            found = true;
            break;
        }

    }
    if(!found)
    {
        printf("Error: File not found.\n");
    }
}
void FAT32cd(char* name)
{
    char * directory;
    directory = strtok(name, "/");
    int offset;
    bool found = false;
    if(strcmp(directory,"..") == 0)
    {
        fseek(fp, last_offset, SEEK_SET);
        fread(&dir[0],sizeof(struct DirectoryEntry), 16, fp);
    }
    else
    {
        for(int i = 0; i < 16; i++)
        {
            if(compare(directory, dir[i].DIR_Name) && dir[i].DIR_Attr == 0x10)
            {
                int cluster = dir[i].DIR_FirstClusterLow;
                if(strcmp(directory,"..") == 0)
                {
                    offset = last_offset;
                }
                else
                {
                    offset = LBAtoOffset(cluster);
                }
                fseek(fp, offset, SEEK_SET);
                fread(&dir[0],sizeof(struct DirectoryEntry), 16, fp);
                found = true;
                break;
            }
        }
    }

    while(directory = strtok(NULL,"/"))
    {
        if(strcmp(directory,"..") == 0)
        {
            fseek(fp, last_offset, SEEK_SET);
            fread(&dir[0],sizeof(struct DirectoryEntry), 16, fp);
        }
        else
        {
            for(int i = 0; i < 16; i++)
            {
                if(compare(directory, dir[i].DIR_Name) && dir[i].DIR_Attr == 0x10)
                {
                    int cluster = dir[i].DIR_FirstClusterLow;
                    if(strcmp(directory,"..") == 0)
                    {
                        offset = last_offset;
                    }
                    else
                    {
                        offset = LBAtoOffset(cluster);
                    }

                    fseek(fp, offset, SEEK_SET);
                    fread(&dir[0],sizeof(struct DirectoryEntry), 16, fp);
                    found = true;
                    break;
                }
            }
        }
    }
    if(!found)
    {
        printf("Error: Directory not found.\n");
    }
    last_offset = offset;
}
void FAT32ls()
{
    for(int i = 0; i < 16; i++)
    {
        if((dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 ||
            dir[i].DIR_Attr == 0x20) && dir[i].DIR_Name[0] != 0xe5)
        {
            char name[12];
            memcpy(name, dir[i].DIR_Name, 11);
            name[11] = '\0';
            printf("%s\n",name);
        }
    }
}

void FAT32read(char* name, int offset, int numOfBytes)
{
  //find
  // int offset = Fileoffset + initOffset;
  // offset += fileOffset;
  bool found = false;
  for(int i = 0; i < 16; i++)
  {
      if(compare(name, dir[i].DIR_Name))
      {
          fseek(fp, offset, SEEK_SET);
          fread(buffer, numOfBytes, 1, fp);

          for(int i = 0; i < numOfBytes; i++)
          {
            printf("%d ", buffer[i]);
          }
          printf("\n");
          found = true;
          break;
      }
  }
  if(!found)
  {
      printf("Error: Directory not found.\n");
  }
}
void FAT32del(char* name)
{

}
void FAT32undel(char* name)
{

}

bool compare(char* name, char* dirName)
{

    char input[12];
    char IMG_Name[12];

    strncpy(input, name, 12);
    strncpy(IMG_Name, dirName, 12);

    char expanded_name[12];
    memset( expanded_name, ' ', 12 );

    char *token = strtok( input, "." );

    strncpy( expanded_name, token, strlen( token ) );

    token = strtok( NULL, "." );

    if( token )
    {
      strncpy( (char*)(expanded_name+8), token, strlen(token ) );
    }

    expanded_name[11] = '\0';

    int i;
    for( i = 0; i < 11; i++ )
    {
      expanded_name[i] = toupper( expanded_name[i] );
    }

    if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
    {
      printf("They matched\n");
      return true;
    }
    else
    {
        return false;
    }
}
