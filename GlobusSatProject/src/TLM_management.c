/*
 * filesystem.c
 *
 *  Created on: 20 áîøõ 2019
 *      Author: Idan
 */


#include <satellite-subsystems/GomEPS.h>
#include <hal/Timing/Time.h>
#include <hcc/api_fat.h>
#include <hal/errors.h>
#include <hcc/api_hcc_mem.h>
#include <string.h>
#include <hcc/api_mdriver_atmel_mcipdc.h>
#include <hal/Storage/FRAM.h>
#include <at91/utility/trace.h>
#include "TLM_management.h"
#include <stdlib.h>
#include <GlobalStandards.h>

#define SKIP_FILE_TIME_SEC 1000000
#define _SD_CARD 0
#define FIRST_TIME -1
#define FILE_NAME_WITH_INDEX_SIZE MAX_F_FILE_NAME_SIZE+sizeof(int)*2

//struct for filesystem info
typedef struct
{
	int num_of_chains;
} FS;
//TODO remove all 'PLZNORESTART' from code!!
#define PLZNORESTART() gom_eps_hk_basic_t myEpsTelemetry_hk_basic;	(GomEpsGetHkData_basic(0, &myEpsTelemetry_hk_basic)); //todo delete

//struct for chain file info
typedef struct
{
	int size_of_element;
	char name[FILE_NAME_WITH_INDEX_SIZE];
	unsigned int creation_time;
	unsigned int last_time_modified;
	int num_of_files;

} C_FILE;
#define C_FILES_BASE_ADDR (FSFRAM+sizeof(FS))


void delete_allTMFilesFromSD()
{
}
// return -1 for FRAM fail
static int getNumOfChainsInFS()
{
	FS fs;
	FRAM_read(&fs,FSFRAM,sizeof(fs));
	return fs.num_of_chains;
}
//return -1 on fail
static int setNumOfChainsInFS(int new_num_of_chains)
{
	FS fs;
	fs.num_of_chains = new_num_of_chains;
	FRAM_write(&fs,FSFRAM,sizeof(fs));
	return 0;
}
FileSystemResult InitializeFS(Boolean first_time)
{
	return FS_SUCCSESS;
}

//only register the chain, files will create dynamically
FileSystemResult c_fileCreate(char* c_file_name,int size_of_element)
{
	/*
	time_unix currTime = Time_getUnixEpoch();
	C_FILE c_file;
	c_file.name = c_file_name;
	c_file.creation_time = currTime;
	c_file.last_time_modified = -1;
	c_file.num_of_files = 0;
	c_file.size_of_element = size_of_element;
	int err = FRAM_write(&c_file,C_FILES_BASE_ADDR*(getNumOfChainsInFS()+sizeof(c_file)),sizeof(c_file));
	setNumOfChainsInFS(getNumOfChainsInFS()+1);
	if(err < 0)
		return FS_FRAM_FAIL;*/
	return FS_SUCCSESS;
}
//write element with timestamp to file
static void writewithEpochtime(F_FILE* file, byte* data, int size,unsigned int time)
{
}
// get C_FILE struct from FRAM by name
static Boolean get_C_FILE_struct(char* name,C_FILE* c_file,unsigned int *address)
{
	return FALSE;
}
//calculate index of file in chain file by time
static int getFileIndex(unsigned int creation_time, unsigned int current_time)
{
	return (current_time-creation_time)/SKIP_FILE_TIME_SEC;
}
//write to curr_file_name
void get_file_name_by_index(char* c_file_name,int index,char* curr_file_name)
{
	//get
	//int index = getFileIndex()
}
FileSystemResult c_fileReset(char* c_file_name)
{
	return FS_SUCCSESS;
}

FileSystemResult c_fileWrite(char* c_file_name, void* element)
{
	return FS_SUCCSESS;
}
FileSystemResult fileWrite(char* file_name, void* element,int size)
{
	F_FILE *file;
	file=f_open(file_name,"a");
	if (!file)
	{
		int rc = f_getlasterror();
		//optinos : too long / aloocation / return in printf the kind of error
		return FS_FAIL;
	}

	// add timestamp  to the file
	time_unix curr_time;
	int err = Time_getUnixEpoch(&curr_time);
	if (!err){
		return err;
	}

	if(f_write(&curr_time,sizeof(curr_time),1,file)!=sizeof(curr_time) || f_getlasterror()!=F_NO_ERROR){
		f_flush();
		f_close();
		return FS_FAIL;
	}

	// add element to the file
	if(f_write(element,size,1,file)!=size || f_getlasterror()!=F_NO_ERROR){
		f_flush();
		f_close();
		return FS_FAIL;
	}

	if(f_flush(file)!=F_NO_ERROR){
		return FS_FAIL;
	}
	if(f_close(file)!=F_NO_ERROR){
		return FS_FAIL;
	}

	return FS_SUCCSESS;
}
static FileSystemResult deleteElementsFromFile(char* file_name,unsigned long from_time,
		unsigned long to_time,int full_element_size)
{
	return FS_SUCCSESS;
}
FileSystemResult c_fileDeleteElements(char* c_file_name, time_unix from_time,
		time_unix to_time)
{
	return FS_SUCCSESS;
}

/**
 *
 * read = return the number of elements read
 */
FileSystemResult fileRead(char* c_file_name,byte* buffer, int size_of_buffer,
		time_unix from_time, time_unix to_time, int* read, int element_size)
{

	// check from_time < to_time

	F_FILE *file;
	file=f_open(c_file_name,"r");
	if (!file)
	{
		int rc = f_getlasterror();
		//optinos : too long / aloocation / return in printf the kind of error
		return FS_FAIL;
	}

	// get first timestamp
	time_unix temp_time;
	if (f_read(&temp_time,sizeof(time_unix),1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
		f_close();
		return FS_FAIL;
	}

	// seek for the first timestamp that is >= from_time
	while (temp_time<from_time){
		f_seek(file,element_size,SEEK_CUR);

		if (f_read(&temp_time,sizeof(time_unix),1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
			f_close();
			return FS_FAIL;
		}
	}

	*read = 0;
	int bufferPos=0;
	// Continue reading into buffer until timestamp that is > to_time
	while (temp_time<=to_time){

		// add first timestamp into buffer
		memcpy(&buffer[bufferPos],&temp_time,sizeof(time_unix));
		bufferPos += sizeof(time_unix);
		// add first element into buffer
		if (f_read(&buffer[bufferPos],element_size,1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
			f_close();
			return FS_FAIL;
		}
		bufferPos += element_size;
		(*read)++;
		if (f_read(&temp_time,sizeof(time_unix),1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
			f_close();
			return FS_FAIL;
		}
	}

	if(f_close(file)!=F_NO_ERROR){
		return FS_FAIL;
	}

	return FS_SUCCSESS;
}

FileSystemResult c_fileRead(char* c_file_name,byte* buffer, int size_of_buffer,
		time_unix from_time, time_unix to_time, int* read,time_unix* last_read_time)
{

	return FS_SUCCSESS;
}
void print_file(char* c_file_name)
{
}

void DeInitializeFS( void )
{
}
typedef struct{
	int a;
	int b;
}TestStruct ;
