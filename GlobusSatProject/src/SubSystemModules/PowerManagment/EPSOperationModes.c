
#include "EPSOperationModes.h"
#include "GlobalStandards.h"

#ifdef ISISEPS
	#include <satellite-subsystems/IsisEPS.h>
#endif
#ifdef GOMEPS
	#include <satellite-subsystems/GomEPS.h>
#endif

//harel's enum. need to change to the right channels
typedef enum __attribute__ ((__packed__)) _channels_options
{
	//channels on
	ch_full_on_3v3 = 0xFF, ///< channels on full mode 3v3
	ch_full_on_5v5 = 0xFF, ///< channels on full mode 5v5
	ch_cruise_on_3v3 =0xFF, ///< channels on cruise mode 3v3
	ch_cruise_on_5v5 = 0xFF, ///< channels on cruise mode 5v5
	ch_safe_on_3v3 = 0xFF, ///< channels on safe mode 3v3
	ch_safe_on_5v5 = 0xFF, ///< channels on safe mode 5v5

	//channels off
	ch_cruise_off_3v3 = 0xFF, ///< channels off cruise mode 3v3
	ch_cruise_off_5v5 = 0xFF, ///< channels off cruise mode 5v5
	ch_safe_off_3v3 = 0xFF, ///< channels off safe mode 3v3
	ch_safe_off_5v5 = 0xFF, ///< channels off safe mode 5v5
	ch_critical_off_3v3 = 0xFF, ///< channels off critical mode 3v3
	ch_critical_off_5v5 = 0xFF ///< channels off critical mode 5v5

} channels_options;


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
	return IsisEPS_outputBusGroupOn(I2C_add , ch_full_on_3v3 , ch_full_on_5v5 , p_rsp_code);
}

int EnterCruiseMode()
{
	state = CruiseMode;
//	int err1 = IsisEPS_outputBusGroupOn(I2C_add , ch_cruise_on_3v3 , ch_cruise_on_5v5 , p_rsp_code);
//	int err2 = IsisEPS_outputBusGroupOff(I2C_add , ch_cruise_off_3v3 , ch_cruise_off_5v5 , p_rsp_code);
	return err1 == 0 ? err2 : err1;
}

int EnterSafeMode()
{
	state = SafeMode;
//	int err1 = IsisEPS_outputBusGroupOn(I2C_add , ch_safe_on_3v3 , ch_safe_on_5v5 , p_rsp_code);
//	int err2 = IsisEPS_outputBusGroupOff(I2C_add , ch_safe_off_3v3 , ch_safe_off_5v5 , p_rsp_code);
	return err1 == 0 ? err2 : err1;
}

int EnterCriticalMode()
{
	state = CriticalMode;
	return 0; //IsisEPS_outputBusGroupOff(I2C_add , ch_critical_off_3v3 , ch_critical_off_5v5 , p_rsp_code);

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

