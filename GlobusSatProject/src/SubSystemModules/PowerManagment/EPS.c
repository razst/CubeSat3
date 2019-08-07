
#include <satellite-subsystems/IsisSolarPanelv2.h>
#include <hal/errors.h>

#include <string.h>
#include <hal/Storage/FRAM.h>
#include "EPSOperationModes.h"

#include "EPS.h"
#ifdef ISISEPS
	#include <satellite-subsystems/IsisEPS.h>
#endif
#ifdef GOMEPS
	#include <satellite-subsystems/GomEPS.h>
#endif
// y[i] = a * x[i] +(1-a) * y[i-1]
voltage_t prev_avg = 0;		// y[i-1]
float alpha = 0;			//<! smoothing constant

//harel: our last 3 samples of vbatt
voltage_t *vbatt = NULL;
voltage_t eps_threshold_voltages[NUMBER_OF_THRESHOLD_VOLTAGES];	// saves the current EPS logic threshold voltages

int GetBatteryVoltage(voltage_t *vbatt) // harel: what is the importance of recived parameter
{
	ieps_board_t board;
	ieps_enghk_data_cdb_t p_rawhk_data_cdb ;
	ieps_statcmd_t p_rsp_code;

	int err = IsisEPS_getEngHKDataCDB(EPS_I2C_BUS_INDEX , board , &p_rawhk_data_cdb , &p_rsp_code );

	if(err !=0 ){
		return err;
	}
	*vbatt = p_rawhk_data_cdb.fields.bat_voltage;
	return 0;
}

int EPS_Init()
{
	unsigned char I2C_add = EPS_I2C_ADDR;
	int err = IsisEPS_initialize(&I2C_add,1); // TODO address

	if (err != E_NO_SS_ERR && err != E_IS_INITIALIZED){
		return -1;
	}
	err = IsisSolarPanelv2_initialize(slave2_spi); // TODO which slave

	if(ISIS_SOLAR_PANEL_STATE_NOINIT == err){
		return -2;
	}

	IsisSolarPanelv2_sleep();
	err = GetthresholdVoltages(&eps_threshold_voltages);

		if(err != 0 ){
			voltage_t tamp[] = DEFAULT_EPS_THRESHOLD_VOLTAGES;
			memcpy(eps_threshold_voltages , tamp,sizeof(tamp));
			return -3;
		}
	err = GetAlpha(&alpha);

	if(err !=0){
		alpha = DEFAULT_ALPHA_VALUE;
		return -4;
	}

	EPS_Conditioning();
	return 0;

}

int EPS_Conditioning()
{
	voltage_t curr_volt = GetBatteryVoltage(&vbatt);

	// increase volt
	if(curr_volt > prev_avg){
		if(curr_volt > eps_threshold_voltages[INDEX_UP_FULL]){
			EnterFullMode();

		}else if (curr_volt > eps_threshold_voltages[INDEX_UP_CRUISE]) {
			EnterCruiseMode();

		}else if (curr_volt > eps_threshold_voltages[INDEX_UP_SAFE]) {
			EnterSafeMode();

		}
	}
	// decrease volt
	else{
		if(curr_volt < eps_threshold_voltages[INDEX_DOWN_SAFE]){
			EnterCriticalMode();

		}else if (curr_volt < eps_threshold_voltages[INDEX_DOWN_CRUISE]) {
			EnterSafeMode();

		}else if (curr_volt < eps_threshold_voltages[INDEX_DOWN_FULL]) {
			EnterCruiseMode();

		}
	 }
	return 0;
}

int UpdateAlpha(float new_alpha)
{
	if(new_alpha >= 0 && new_alpha <= 1){
		int err = FRAM_write(&new_alpha , EPS_ALPHA_FILTER_VALUE_ADDR , EPS_ALPHA_FILTER_VALUE_SIZE);
		if(err != 0){
			return -1;
		}
		return 0;
	}
	return -2;
}

int UpdateThresholdVoltages(voltage_t thresh_volts[NUMBER_OF_THRESHOLD_VOLTAGES])
{
	//check invalid threshold
		for(int i = 0 ; i<3 ; i++){
			// Upper > Lower
			if(thresh_volts[i] > thresh_volts[i+3]){
				return -2;

			}
			//LowerMode1 < LowerMode2 execpt lower fullmode
			else if(thresh_volts[i] > thresh_volts[i+1] && i!=2){
				return -2;

			}

		}
		return FRAM_write(&thresh_volts , EPS_THRESH_VOLTAGES_ADDR , EPS_THRESH_VOLTAGES_SIZE);
}

int GetthresholdVoltages(voltage_t thresh_volts[NUMBER_OF_THRESHOLD_VOLTAGES])
{
	int err = FRAM_read(&thresh_volts , EPS_THRESH_VOLTAGES_ADDR , EPS_THRESH_VOLTAGES_SIZE);
	if(thresh_volts == NULL){
		return -1;
	}else if (err !=0) {
		return err;
	}
	return 0;
}

int GetAlpha(float *alpha)
{
	FRAM_read(&alpha , EPS_ALPHA_FILTER_VALUE_ADDR , EPS_ALPHA_FILTER_VALUE_SIZE);
	if(alpha == NULL){
		return -1;
	}
	return alpha;
}

int RestoreDefaultAlpha()
{
	float alphaDefault = 0.25;
	return FRAM_write(&alphaDefault , EPS_ALPHA_FILTER_VALUE_ADDR , EPS_ALPHA_FILTER_VALUE_SIZE);
}

int RestoreDefaultThresholdVoltages()
{
	char voltDefault[NUMBER_OF_THRESHOLD_VOLTAGES] = DEFAULT_EPS_THRESHOLD_VOLTAGES;
	return FRAM_write(&voltDefault , EPS_THRESH_VOLTAGES_ADDR , EPS_THRESH_VOLTAGES_SIZE);
}

