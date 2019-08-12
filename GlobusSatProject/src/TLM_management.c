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

//struct for filesystem info
typedef struct
{
	int num_of_chains;
} FS;
//TODO remove all 'PLZNORESTART' from code!!
#define PLZNORESTART() gom_eps_hk_basic_t myEpsTelemetry_hk_basic;	(GomEpsGetHkData_basic(0, &myEpsTelemetry_hk_basic)); //todo delete

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
FileSystemResult c_fileCreate(char* c_file_name,int size_of_element, C_FILE* c_file)
{

	time_unix currTime;
	Time_getUnixEpoch(&currTime);

	strcpy(c_file->name,c_file_name);
	c_file->creation_time = currTime;
	c_file->last_time_modified = -1;
	c_file->num_of_files = 0;
	c_file->size_of_element = size_of_element;
	int err = FRAM_write(&c_file,C_FILES_BASE_ADDR*(getNumOfChainsInFS()+sizeof(*c_file)),sizeof(*c_file));
	setNumOfChainsInFS(getNumOfChainsInFS()+1);
	if(err < 0)
		return FS_FRAM_FAIL;
	return FS_SUCCSESS;
}
//write element with timestamp to file
static void writewithEpochtime(F_FILE* file, byte* data, int size,unsigned int time)
{
	int number_of_writes;
	number_of_writes = f_write( &time,sizeof(unsigned int),1, file );
	number_of_writes += f_write( data, size,1, file );
	printf("writing element to file, time is: %u\n",time);
	if(number_of_writes!=2)
	{
		// make sure we wrote twice ! TODO: we si did write once - the whole file will be corrupted because there is a missing data only time
		printf("writewithEpochtime error\n");
	}
	f_flush( file ); /* only after flushing can data be considered safe */
	f_close( file ); /* data is also considered safe when file is closed */
}


// get C_FILE struct from FRAM by name
static Boolean get_C_FILE_struct(char* name,C_FILE* c_file,unsigned int *address)
{
	int i;
	unsigned int c_file_address = 0;
	int err_read=0;
	int num_of_files_in_FS = getNumOfChainsInFS();
	for(i =0; i < num_of_files_in_FS; i++)			//search correct c_file struct
	{
		c_file_address= C_FILES_BASE_ADDR+sizeof(C_FILE)*(i);
		err_read = FRAM_read((unsigned char*)c_file,c_file_address,sizeof(C_FILE));
		if(0 != err_read)
		{
			printf("FRAM error in 'get_C_FILE_struct()' error = %d\n",err_read);
			return FALSE;
		}

		if(!strcmp(c_file->name,name))//if  strcmp equals return 0
		{
			if(address != NULL)
			{
				*address = c_file_address;
			}
			return TRUE;
		}
	}
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
	sprintf(curr_file_name,"%s%d.%s", c_file_name, index, FS_FILE_ENDING); // sentence that include all paramatrim
}

FileSystemResult c_fileReset(char* c_file_name)
{
	return FS_SUCCSESS;
}


FileSystemResult c_fileWrite(char* c_file_name, void* element)
{
	C_FILE c_file;
	unsigned int addr;//FRAM ADDRESS
	F_FILE *file;
	char curr_file_name[MAX_F_FILE_NAME_SIZE];

	unsigned int curr_time;
	Time_getUnixEpoch(&curr_time);
	if(get_C_FILE_struct(c_file_name,&c_file,&addr)!=TRUE)//get c_file
	{
		// no C file found - guess this is the first time we try to save this type, than create it
		c_fileCreate(c_file_name,sizeof(c_file_name),&c_file);
		//return FS_NOT_EXIST;
	}
	int index_current = getFileIndex(c_file.creation_time,curr_time);
	get_file_name_by_index(c_file_name,index_current,curr_file_name);

	file = f_open(curr_file_name,"a+");//a+ append to file readble do write in the end
	writewithEpochtime(file,element,c_file.size_of_element,curr_time);
	c_file.last_time_modified= curr_time;
	if(FRAM_write((unsigned char *)&c_file,addr,sizeof(C_FILE))!=0)//update last written
	{
		return FS_FRAM_FAIL;
	}
	f_close(file);
	f_releaseFS();
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
		f_flush(file);
		f_close(file);
		return FS_FAIL;
	}

	// add element to the file
	if(f_write(element,size,1,file)!=size || f_getlasterror()!=F_NO_ERROR){
		f_flush(file);
		f_close(file);
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
		f_close(file);
		return FS_FAIL;
	}

	// seek for the first timestamp that is >= from_time
	while (temp_time<from_time){
		f_seek(file,element_size,SEEK_CUR);

		if (f_read(&temp_time,sizeof(time_unix),1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
			f_close(file);
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
			f_close(file);
			return FS_FAIL;
		}
		bufferPos += element_size;
		(*read)++;
		if (f_read(&temp_time,sizeof(time_unix),1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
			f_close(file);
			return FS_FAIL;
		}
	}

	if(f_close(file)!=F_NO_ERROR){
		return FS_FAIL;
	}

	return FS_SUCCSESS;
}

/**
 *
 * read = return the number of elements read
 */
FileSystemResult fileReadGeneral(char* file_name,byte* buffer, int size_of_buffer)
{

	// check from_time < to_time

	F_FILE *file;
	file=f_open(file_name,"r");
	if (!file)
	{
		int rc = f_getlasterror();
		//optinos : too long / aloocation / return in printf the kind of error
		return FS_FAIL;
	}

	if (f_read(&buffer,size_of_buffer,1,file)!=1 || f_getlasterror()!=F_NO_ERROR){
		f_close(file);
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
