
#include "EPSOperationModes.h"
#include "GlobalStandards.h"

#ifdef ISISEPS
	#include <satellite-subsystems/IsisEPS.h>
#endif
#ifdef GOMEPS
	#include <satellite-subsystems/GomEPS.h>
#endif


//TODO: update functions to only the relevant channels
channel_t g_system_state;
EpsState_t state; // harel: the EPS state
Boolean g_low_volt_flag = FALSE; // set to true if in low voltage
ieps_obus_channel_t chmask3v3;
ieps_obus_channel_t chmask5v5;
ieps_statcmd_t* p_rsp_code;
unsigned char I2C_add = EPS_I2C_ADDR;


int EnterFullMode()
{
	state = FullMode;
	return IsisEPS_outputBusGroupOn(I2C_add , chmask3v3 , chmask5v5 , p_rsp_code);
}

int EnterCruiseMode()
{
	state = CruiseMode;
	int err1 = IsisEPS_outputBusGroupOn(I2C_add , chmask3v3 , chmask5v5 , p_rsp_code);
	int err2 = IsisEPS_outputBusGroupOff(I2C_add , chmask3v3 , chmask5v5 , p_rsp_code);
	return err1 == 0 ? err2 : err1;
}

int EnterSafeMode()
{
	state = SafeMode;
	int err1 = IsisEPS_outputBusGroupOn(I2C_add , chmask3v3 , chmask5v5 , p_rsp_code);
	int err2 = IsisEPS_outputBusGroupOff(I2C_add , chmask3v3 , chmask5v5 , p_rsp_code);
	return err1 == 0 ? err2 : err1;
}

int EnterCriticalMode()
{
	state = CriticalMode;
	return IsisEPS_outputBusGroupOff(I2C_add , chmask3v3 , chmask5v5 , p_rsp_code);

}

int SetEPS_Channels(channel_t channel)
{
	return 0;
}

EpsState_t GetSystemState()
{
	return state;
}

channel_t GetSystemChannelState()
{
	return g_system_state;
}

Boolean EpsGetLowVoltageFlag()
{
	return FALSE;
}

void EpsSetLowVoltageFlag(Boolean low_volt_flag)
{
}

