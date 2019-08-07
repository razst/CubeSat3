#include <string.h>
#include <hal/errors.h>

#include "TRXVU.h"
#include "ActUponCommand.h"
#include "CommandDictionary.h"
#include "AckHandler.h"

int ActUponCommand(sat_packet_t *cmd)
{

	switch (cmd->cmd_type){
	case ack_type:
		//???
		break;
	case managment_cmd_type:
		managment_command_router(&cmd);
		break;
	case filesystem_cmd_type:
		filesystem_command_router(&cmd);
		break;
	case telemetry_cmd_type:
		telemetry_command_router(&cmd);
		break;
	case eps_cmd_type:
		eps_command_router(&cmd);
		break;
	case trxvu_cmd_type:
		trxvu_command_router(&cmd);
		break;
	default:
		return
	}
	return 0;
}


