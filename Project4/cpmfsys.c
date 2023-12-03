#define _CMPFSYS_C
#include <ctype.h>
#include <stdio.h> 
#include <stdint.h>
#include "cpmfsys.h"
#include "diskSimulator.h"

/*
 * Matthew Voggel - COMP 7500 - cpmFS a simple file system
 * The accompanying files as well as this file create a simple file system program. (Project 4)
 * This simple file system allows a user to list directory entries, rename, copy, and 
 * delete files, as well as read/write/open/close files. 
 *
 * This also simulates the block management in printing a 16 x 16 square showing the blocks
 * currently in use, and those that are free. Extents for the blocks are outlined below in the 
 * mkDirStruct method, which allocates the memory to be used.
 *
 * This will be the only file uploaded in the tarball for the final submission, plus report. 
 *
 *
 * To run the compilation after using make, type ./cpmRun
 *
 */ 


#define PRINT_ROW_SIZE 16
bool freeList[NUM_BLOCKS];
////////////////////////////////removeWhiteSpaces in the list FUNCTION//////////////////////////////////
void removeWhiteSpace(char *input)
{
  int i = 0;
  for (int j=0; input[j]; j++) {
    input[j] = input[i + j];
    if((input[j]) == ' ') {
      i++;
      j--;
    }
  }
}
////////////////////////////////mkDirStruct FUNCTION//////////////////////////////////
//allocates the memory, gives a pointer for population, and assigns based on extent outline
//extent definitions from lectures - DOUBLE CHECK BEFORE SUBMITTING
DirStructType *mkDirStruct(int index, uint8_t *e) {
  DirStructType *my_struct = malloc(sizeof(DirStructType));
  uint8_t *location = (e + index * EXTENT_SIZE);
  my_struct -> status = location[0];
  //starts with e block, assigning remaining within the for loop
  int i =1;
  for (int j=0; j<8; j++) {
    my_struct-> name[j] = ' ';
    my_struct-> name[j] = location[i++];
  }
  my_struct->name[8] = '\0';
  i = 9;
  for (int j=0; j < 3; j++) {
    my_struct->extension[j] = ' ';
    my_struct->extension[j] = location[i++];
  }
  my_struct->extension[3] = '\0';
  i = 16;
  my_struct->XL = location[12];
  my_struct->BC = location[13];
  my_struct->XH = location[14];
  my_struct->RC = location[15];
  for (int j=0; j < 16; j++) {
    my_struct->blocks[j] = location[i++];
  }
  return my_struct;
}
////////////////////////////////writeDirStruct FUNCTION//////////////////////////////////
//writing of the outlined/designated block allocation above to the block of memory
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e) {
  uint8_t *location = (e + index * EXTENT_SIZE);
  location[0] = d->status;
  int i = 1;
  for (int j = 0; j < 8; j++) {
    location[i++] = d->name[j];
  }
  location[9] = ' ';
  i = 9;
  for (int j=0; j < 3; j++) {
    location[i++] = d->extension[j];
  }
  location[11] = ' ';
  i = 16;
  location[12] = d->XL;
  location[13] = d->BC;
  location[14] = d->XH;
  location[15] = d->RC;
  for (int j=0; j < 16; j++) {
    location[i++] = d->blocks[j];
  }
  blockWrite(e, 0);
  makeFreeList();
}
////////////////////////////////makeFreeList FUNCTION//////////////////////////////////
/* Populates the FreeList data structure that we defined above as global.  freeList[i] = true; means the block i of the
 * disk is free, if it is false means the block is in use. Block 0 is never free since it holds the directory.
 *
 */
void makeFreeList() {
  uint8_t *blockZero = malloc(BLOCK_SIZE);
  freeList[0] = false;
  for (int i=1; i < NUM_BLOCKS; i++) {
    freeList[i] = true;
  }
  blockRead(blockZero, 0);
  for (int i=0; i < EXTENT_SIZE; i++) {
    DirStructType *extent = mkDirStruct(i, blockZero);
    if (extent->status != 0xe5) {
      for (int j=0; j < BLOCKS_PER_EXTENT; j++) {
        if (extent->blocks[j] != 0) {
          freeList[(int)extent->blocks[j]] = false;
        }
      }
    }
  }
}
////////////////////////////////printFreeList FUNCTION//////////////////////////////////
/*
 * This is a debugging function, print out the contents of the free list in 16 rows of 16, with each 
 * row prefixed by the 2-digit hex address of the first block in that row. Denote a used
 * block with a *, a free block with a . 
*/ 
void printFreeList() {
  fprintf(stdout, "FREE LIST PRINTOUT: (* means this block is in use)\n");
  int freeListIndex = 0;
  for (int i=0; i < NUM_BLOCKS/PRINT_ROW_SIZE; i++) {
    if (i == 0) {
      fprintf(stdout, "  %x: ", i);
    }
    else {
      fprintf(stdout, " %x0: ", i);
    }
    for (int j=0; j < PRINT_ROW_SIZE; j++) {
      if (freeList[freeListIndex++] == true) {
        fprintf(stdout, ". ");
      }
      else {
        fprintf(stdout, "* ");
      }
    }
    fprintf(stdout, "\n");
  }
}
////////////////////////////////cpmDir FUNCTION//////////////////////////////////
/* This function prints the file directory to stdout. Each filename should be printed on its own line, 
 * with the file size, in base 10. If a file does not have an extension it will print anyway using 
 * dot notation that shows the byte-size.  would indicate a file whose name was myfile, with no 
 * extension and a size of 234 bytes. 
 */
void cpmDir() {
  uint8_t *blockZero = malloc(BLOCK_SIZE);
  fprintf(stdout, "DIRECTORY LIST\n");
  blockRead(blockZero, 0);
  int blockIndex;
  int fileSize;
  for (int i=0; i < EXTENT_SIZE; i++) {
    DirStructType *directory = mkDirStruct(i, blockZero);
    if (directory->status != 0xe5) {
      blockIndex = 0;
      for (int j=0; j < BLOCKS_PER_EXTENT; j++) {
        if (directory->blocks[j] != 0) {
          blockIndex++;
        }
      }
      fileSize = (blockIndex - 1) * BLOCK_SIZE + directory->RC * 128 + directory->BC;
      if (directory->RC == 0 && directory->BC == 0) {
        fprintf(stdout, "error: RC and BC are both value 0\n");
      }
      else {
        char name[9];
        char extension[4];
        sprintf(name, "%s", directory->name);
        sprintf(extension, "%s", directory->extension);
        removeWhiteSpace(name);
        removeWhiteSpace(extension);
        fprintf(stdout, "%s.%s %d\n", name, extension, fileSize);
      }
    }
  }
}
////////////////////////////////checkLegalName FUNCTION//////////////////////////////////
// Returns true for legal name, false for illegal name which is a name or extension that is 
// too long, blank, or  illegal characters in name or extension.
bool checkLegalName(char *name) {
  int size = strlen(name);
  if (size == 0) {
    return false;
  }
  return isalnum(name[0]);
}

////////////////////////////////findExtentWithName FUNCTION//////////////////////////////////
// Function that returns -1 for illegal name or name not found, else returns extent number 0-31
int findExtentWithName(char *name, uint8_t *block0) {
  if (checkLegalName(name)) {
    for (int i=0; i < EXTENT_SIZE; i++) {
      DirStructType *directory = mkDirStruct(i, block0);
      char local_name[9];
      char extension[4];
      sprintf(local_name, "%s", directory->name);
      sprintf(extension, "%s", directory->extension);
      removeWhiteSpace(local_name);
      removeWhiteSpace(extension);
      char fullName[14];
      if (strchr(name, '.') == NULL) {
        sprintf(fullName, "%s", local_name);
        if (strcmp(fullName, name) == 0) {
          return i;
        }
      }
      else {
        sprintf(fullName, "%s.%s", local_name, extension);
        if (strcmp(fullName, name) == 0) {
          return i;
        }
      }
    }
  }
  return -1;
}

////////////////////////////////cpmDelete FUNCTION//////////////////////////////////
// This one is fairly simple - deletes the file named 'name,' and frees disk blocks in the free list 
int  cpmDelete(char * name) {
  uint8_t *blockZero = malloc(BLOCK_SIZE);
  blockRead(blockZero, 0);
  int delete = findExtentWithName(name, blockZero);
  if (delete == -1) {
    return -1;
  }
  else {
    DirStructType *directory = mkDirStruct(delete, blockZero);
    directory->status = 0xe5;
    writeDirStruct(directory, delete, blockZero);
    return 0;
  }
}

////////////////////////////////cpmRename FUNCTION//////////////////////////////////
// This function reads directory block, modifies the extent for file named 'oldName' with 'newName,' and writes to the disk
int cpmRename(char *oldName, char * newName) {
  if (checkLegalName(newName)) {
    uint8_t *blockZero = malloc(BLOCK_SIZE);
    blockRead(blockZero, 0);
    int foundExtent = findExtentWithName(oldName, blockZero);
    DirStructType *directory  = mkDirStruct(foundExtent, blockZero);
    if (foundExtent == -1) {
      return -1;
    }
    else {
      char name[9];
      char extension[4];
			sprintf(name, "        ");
			sprintf(extension, "   ");

      int i = 0;
      for (int j=0; j < 8; j++) {
        if (newName[j] == '.') {
          i++;
          break;
        }
        else {
          name[j] = newName[j];
          i++;
        }
      }
      if (strchr(oldName, '.') == NULL) {
        i++;
      }
      for (int j=0; j < 3; j++) {
        if (isalnum(newName[i])) {
          extension[j] = newName[i];
        }
        else {
          extension[j] = ' ';
        }
        i++;
      }
      //movement of the old name to the new name.
      sprintf(directory->name, "%s", name);
      sprintf(directory->extension, "%s", extension);
      writeDirStruct(directory, foundExtent, blockZero);
      return 0;
    }
  }
  else {
    return -2;
  }
}

