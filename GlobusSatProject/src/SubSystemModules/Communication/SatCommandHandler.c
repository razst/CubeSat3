#include <satellite-subsystems/IsisTRXVU.h>
#include <hal/Timing/Time.h>
#include <string.h>
#include <stdlib.h>

#include "GlobalStandards.h"
#include "SatCommandHandler.h"


typedef struct __attribute__ ((__packed__)) delayed_cmd_t
{
	time_unix exec_time;	///< the execution time of the cmd in unix time
	sat_packet_t cmd;		///< command data
} delayed_cmd_t;

int ClearDelayedCMD_FromBuffer(unsigned int start_addr, unsigned int end_addr)
{
	return 0;
}

int ParseDataToCommand(unsigned char * data, unsigned int length, sat_packet_t *cmd)
{
	memcpy(&cmd,data,length);
	return 0;
}

int AssmbleCommand(unsigned char *data, unsigned int data_length, char type,
		char subtype, unsigned int id, sat_packet_t *cmd)
{
	cmd->ID=id;
	cmd->cmd_subtype = subtype;
	cmd->cmd_type = type
	cmd->data = data;
	cmd->length = data_length;
	return 0;
}

// checks if a cmd time is valid for execution -> execution time has passed and command not expired
// @param[in] cmd_time command execution time to check
// @param[out] expired if command is expired the flag will be raised
Boolean isDelayedCommandDue(time_unix cmd_time, Boolean *expired)
{
	int cmdTime;
		for(int i = 0; i < MAX_DELAYED_COMMAND ; i++){
			FRAM_read(&cmdTime , DELAYED_COMMAND_DUE_ADDR + (i*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);
			if(cmdTime == cmd_time){
				return TRUE;
			}
		}
	return FALSE;
}

//TOOD: move delayed cmd logic to the SD and write 'checked/uncheked' bits in the FRAM
int GetDelayedCommand(sat_packet_t *cmd)
{
	time_unix cmdTime,currTime;
	Time_getUnixEpoch(&currTime);
			for(int i = 0; i < MAX_DELAYED_COMMAND ; i++){
				FRAM_read(&cmdTime , DELAYED_COMMAND_DUE_ADDR + (i*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);
				if(cmdTime <= currTime){
					char filename[8];
					GetFileName(cmdTime , filename);
					fileReadGeneral(filename , &cmd , sizeof(cmd));
					return i;

				}
			}
	return -1;

}

int AddDelayedCommand(sat_packet_t *cmd)
{
	time_unix cmdTime;
	for(int i = 0; i < MAX_DELAYED_COMMAND ; i++){
		FRAM_read(&cmdTime , DELAYED_COMMAND_DUE_ADDR + (i*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);
		if(cmdTime == -1){
			time_unix delayTIime;
			memcpy(&delayTIime,&cmd->data,DELAYED_COMMAND_DUE_SIZE);// first 4 bytes of the data is the delay time
			char filename[8];
			GetFileName(delayTIime , filename);

			FRAM_write(&delayTIime , DELAYED_COMMAND_DUE_ADDR + (i*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);
			fileWrite(filename , cmd->data , sizeof(cmd)); // the data of the delayed sat_pakect includes the actual sat_pakect
			return 0;
		}
	}
	return 0;
}

int GetDelayedCommandBufferCount()
{
	int num = 0;
	for(int i = 0; i < MAX_DELAYED_COMMAND ; i++){
		FRAM_read(&delayed_cmd_t , DELAYED_COMMAND_DUE_ADDR + (i*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);
		if(delayed_cmd_t != -1){
			num++;
			}
	}
	return num;

}

int GetOnlineCommand(sat_packet_t *cmd)
{
	unsigned char data[MAX_COMMAND_DATA_LENGTH];
	int addr = I2C_TRXVU_RC_ADDR;
	ISIStrxvuRxFrame frame;
	frame.rx_doppler = 0;
	frame.rx_framedata = 0;
	frame.rx_length = 0;
	frame.rx_rssi = &data;
	int err = IsisTrxvu_rcGetCommandFrame(&addr,&frame);
	if (err !=0)
		return err;
	sat_packet_t command;
	int err = ParseDataToCommand(&data,MAX_COMMAND_DATA_LENGTH,&command);
	if (err !=0)
		return err;
	cmd = &command;
	SendAckPacket(ACK_RECEIVE_COMM,&command,NULL,0);

	return 0;
}

int GetDelayedCommandByIndex(unsigned int index, sat_packet_t *cmd)
{
	time_unix cmdTime;
	char filename[8];
	FRAM_read(&cmdTime , DELAYED_COMMAND_DUE_ADDR + (index*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);

	GetFileName(cmdTime , filename);
	fileReadGeneral(filename , &cmd , sizeof(cmd));
	return 0;
}

int DeleteDelayedCommandByIndex(unsigned int index)
{
	time_unix cmdTime;
	char filename[8];
	int delet = -1;

	FRAM_read(&cmdTime , DELAYED_COMMAND_DUE_ADDR + (index*DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);
	FRAM_write(&delet , DELAYED_COMMAND_DUE_ADDR + (index * DELAYED_COMMAND_DUE_SIZE), DELAYED_COMMAND_DUE_SIZE);

	GetFileName(&cmdTime , filename);
	f_delete(filename);

	return 0;
}

int DeleteDelayedBuffer()
{
	return 0;
}

void GetFileName(time_unix exec_time , char *filename){

	sprintf(filename , "%d.dly" , exec_time);
}
