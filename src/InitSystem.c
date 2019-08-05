#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <hal/Drivers/I2C.h>
#include <hal/Drivers/SPI.h>
#include <hal/Storage/FRAM.h>
#include <hal/Timing/Time.h>
#include <at91/utility/exithandler.h>
#include <string.h>
#include "GlobalStandards.h"
#include "SubSystemModules/PowerManagment/EPS.h"
#include "SubSystemModules/Communication/TRXVU.h"
#include "SubSystemModules/Maintenance/Maintenance.h"
#include "InitSystem.h"
#include "TLM_management.h"
#include "FRAM_FlightParameters.h"

#ifdef GOMEPS
	#include <satellite-subsystems/GomEPS.h>
#endif
#ifdef ISISEPS
	#include <satellite-subsystems/IsisEPS.h>
	#include <satellite-subsystems/IsisAntS.h>
#endif
#define I2c_SPEED_Hz 100000
#define I2c_Timeout 10
#define I2c_TimeoutTest portMAX_DELAY

#define TIME_SAT_LUNCH {\
	.seconds =0,\
	.minutes =35,\
	.hours= 13,\
	.day = 4,\
	.date= 1,\
	.month=8,\
	.year=19,\
	.secondsOfYear=0}

Boolean isFirstActivation()
{
	Boolean flag;
	FRAM_read(&flag,FIRST_ACTIVATION_FLAG_ADDR,FIRST_ACTIVATION_FLAG_SIZE);
	return flag;
}

/*!
 * @brief
 * @return 0 - sucsess
 * 1 - time error (see time.h)
 * -1 - FRAM error (see FRAM.h)
 * -2 - FRAM error (see FRAM.h)
 * -3 - FRAM error (see FRAM.h)
 * < -18 Isis errors (see hal/errors.h)
 */
int firstActivationProcedure()
{
	int err;
	/* wait 30 min */
	time_unix timeFromLounch;
	err = FRAM_read(&timeFromLounch,TIME_FROM_FIRST_LOUNCH_ADDR,TIME_FROM_FIRST_LOUNCH_SIZE);
	if (!err) return err;
	while (timeFromLounch<30*60*1000){
		vTaskDelay(60000*portTICK_RATE_MS); // wait one min
		timeFromLounch = timeFromLounch + 60000;
		err = FRAM_write(&timeFromLounch,TIME_FROM_FIRST_LOUNCH_ADDR,TIME_FROM_FIRST_LOUNCH_SIZE);
		if (!err) return err;
	}



	// open antena
	err = IsisAntS_autoDeployment(1,isisants_sideA,'10'); // Raz: check actual values from demo code from ISIS
	if (!err) return err;
	err = IsisAntS_autoDeployment(1,isisants_sideB,'10'); // Raz: check actual values from demo code from ISIS
	if (!err) return err;


	// set deply flag
	Boolean flag = TRUE;
	err = FRAM_write(&flag,DEPLOY_FLAG_ADDR, sizeof(flag)); // Raz: do we need this flag??? can't we use the FIRST_ACTIVATION_FLAG ??
	if (!err) return err;
	// set deploy time
	Time curr_time;
	err = Time_get(&curr_time);
	if (!err) return err;
	err = FRAM_write(&curr_time,DEPLOYMENT_TIME_ADDR, DEPLOYMENT_TIME_SIZE);
	if (!err) return err;

	// set the first activation flag to FALSE
	flag = FALSE;
	err = FRAM_write(&flag,FIRST_ACTIVATION_FLAG_ADDR,FIRST_ACTIVATION_FLAG_SIZE);
	if (!err) return err;

	return 0;
}

void WriteDefaultValuesToFRAM()
{
	//DEFAULT_NO_COMM_WDT_KICK_TIME = 1;
}

int StartFRAM()
{
	return FRAM_start();
}

int StartI2C()
{
	return I2C_start(I2c_SPEED_Hz,I2c_Timeout);
}

int StartSPI()
{
	return SPI_start(bus1_spi,slave1_spi); // Raz: maybe we need to use bus0_spi???
}

int StartTIME()
{

	if (isFirstActivation()){
		Time curr_time = TIME_SAT_LUNCH;
		return Time_start(&curr_time,0);
	}else{
		time_unix timeBeforeRestart = 0;
		FRAM_read(&timeBeforeRestart,MOST_UPDATED_SAT_TIME_ADDR,MOST_UPDATED_SAT_TIME_SIZE);
		return Time_start(timeBeforeRestart,0);
	}

}

int DeploySystem()
{
	//Raz: after 30 min call firstActivationProcedure
	if (isFirstActivation()){
		return firstActivationProcedure();
	}else
		return 0;
}

#define PRINT_IF_ERR(method) if(0 != err)printf("error in '" #method  "' err = %d\n",err);
int InitSubsystems()
{
	StartSPI();
	StartI2C();
	StartFRAM();
	StartTIME();

	DeploySystem();

	EPS_Init(); // ???
	InitTrxvu();
	// TODO error handaling
	return 0;
}

