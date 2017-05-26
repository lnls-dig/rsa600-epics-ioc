/* C code for program sncExample, generated by snc from ../sncExample.stt */
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>

#include "seq_snc.h"
# line 8 "../sncExample.stt"
#include <stdlib.h>
# line 9 "../sncExample.stt"
#include "/opt/include/RSA_API.h"
# line 11 "../sncExample.stt"


static bool spectrum_settings(double span, double rbw, double vbw, int vbwEn, int traceLen)
{
	ReturnStatus rs;

	SpectrumWindows window = SpectrumWindow_Kaiser;//Use the default window (Kaiser).
	SpectrumVerticalUnits verticalUnit = SpectrumVerticalUnit_dBm;//Use the default vertical units (dBm).

	Spectrum_Settings setSettings, getSettings;
	setSettings.span = span;
	setSettings.rbw = rbw;
	setSettings.enableVBW = vbwEn;
	setSettings.vbw = vbw;
	setSettings.traceLength = traceLen;
	setSettings.window = window;
	setSettings.verticalUnit = verticalUnit;

	rs = SPECTRUM_SetSettings(setSettings);
	//printf("\nERROR: %s", DEVICE_GetErrorString(rs));
	
	rs = SPECTRUM_GetSettings(&getSettings);
	//printf("\nERROR: %s", DEVICE_GetErrorString(rs));

	
	printf("\nSpan: %f\n", getSettings.span);
	printf("\nRBW: %f\n", getSettings.rbw);
	printf("\nVBW: %f\n", getSettings.vbw);
	printf("\nVBW En: %s\n", getSettings.enableVBW ? "ON" : "OFF");
	printf("\nTrace lenght: %d\n", getSettings.traceLength);
	printf("\nStart freq: %f\n", getSettings.actualStartFreq);
	printf("\nStop freq: %f\n", getSettings.actualStopFreq);
	printf("\nFreq step size: %f\n", getSettings.actualFreqStepSize);
	printf("\nFreq span: %f\n", getSettings.span);
	
	
}

static void center_freq(double freq)
{
	ReturnStatus rs;
	rs = CONFIG_SetCenterFreq(freq);
	printf("\nERROR: %s", DEVICE_GetErrorString(rs));
	rs = CONFIG_GetCenterFreq(&freq);
	printf("\nERROR: %s", DEVICE_GetErrorString(rs));

	printf("\nFrequency: %f\n", freq);
	
}

static void ref_level(double refLevel)
{
	ReturnStatus rs;
	rs = CONFIG_SetReferenceLevel(refLevel);
	printf("\nERROR: %s", DEVICE_GetErrorString(rs));
	rs = CONFIG_GetReferenceLevel(&refLevel);
	printf("\nERROR: %s", DEVICE_GetErrorString(rs));

	printf("\nRef level: %f\n", refLevel);
	
}

float get_spectrum(SS_ID ssID, float* traceAmp, VAR_ID traceAmpId, double* traceFreq, VAR_ID traceFreqId)
{
	ReturnStatus rs;

	int n;

	Spectrum_Settings getSettings;
	rs = SPECTRUM_GetSettings(&getSettings);

	int traceLength = getSettings.traceLength;//801, 2401, 4001, 8001,10401, 16001, 32001, and 64001 points
	double freq, stepfreq;
	freq = getSettings.actualStartFreq;
	stepfreq = getSettings.actualFreqStepSize;
	//printf("\nFreq: %f, Step: %f\n", freq, stepfreq);
	
	for (n = 0; n < 64001; n++)
	{
		traceFreq[n] = freq/1000000.0;
		if (n < traceLength)
			freq = freq + stepfreq;
	}
	//printf("\n\nCenter freq: %f\n\n", traceFreq[(traceLength-1)/2]);

	//Start the trace capture.
	rs = SPECTRUM_SetEnable(true);
	//printf("\nSPECTRUM_SetEnable: %s", DEVICE_GetErrorString(rs));

	bool isActive = true;
	int waitTimeoutMsec= 500;//Maximum allowable wait time for each data acquistion.
	int numTimeouts = 2;//Maximum amount of attempts to acquire data if a timeout occurs.
	//Note: the total wait time to acquire data is waitTimeoutMsec x numTimeouts.
    int timeoutCount = 0;//Variable to track the timeouts.
	int traceCount = 0;
	bool traceReady = false;
	int outTracePoints;

	float* pTraceData = (float*)malloc(sizeof(float)*traceLength);

	while(isActive)
	{
		//printf("\nTrace %d",traceCount);
		rs = SPECTRUM_AcquireTrace();
		//printf("\nSPECTRUM_AcquireTrace: %s", DEVICE_GetErrorString(rs));
		//Wait for the trace to be ready.
		rs = SPECTRUM_WaitForTraceReady(waitTimeoutMsec,&traceReady);
		printf("\nSPECTRUM_WaitForTraceReady: %s, traceReady: %d", DEVICE_GetErrorString(rs), traceReady);
		if(traceReady)
		{
			//Get spectrum trace data.
			rs = SPECTRUM_GetTrace(0,traceLength,traceAmp,&outTracePoints);
			//Get traceInfo struct.
			Spectrum_TraceInfo traceInfo;
			rs = SPECTRUM_GetTraceInfo(&traceInfo);
			//You can use this information to report any non-zero bits in AcqDataStatus word, for example.
			if (traceInfo.acqDataStatus != 0)
				printf("\nTrace:%i AcqDataStatus:0x%04X\n", traceCount, traceInfo.acqDataStatus);
			
		traceCount++;

		}
		else timeoutCount++;

		//Stop acquiring traces when the limit is reached or the wait time is exceeded.
		if(timeoutCount == numTimeouts || traceCount == 1)
		{
			if (traceCount == 0)
			{
				printf("\ninit2()\n");
				rs = SPECTRUM_SetEnable(false);
				rs = SPECTRUM_SetEnable(true);
				//init2(ssID, traceAmp, traceAmpId, traceFreq, traceFreqId);
			}
			isActive = false;
		}
	}

	for (n = traceLength; n < 64001; n++)
	{
		traceAmp[n] = traceAmp[traceLength-1];
	}

	seq_pvPut(ssID, traceAmpId, SYNC);

	seq_pvPut(ssID, traceFreqId, SYNC);

	return traceAmp[(traceLength-1)/2];

	//Disconnect the device and finish up.
	//rs = SPECTRUM_SetEnable(false);
	
}

float get_spectrum_sweep(SS_ID ssID, float* traceAmp, VAR_ID traceAmpId, double* traceFreq, VAR_ID traceFreqId, float* SweepAmp, VAR_ID SweepAmpId, double* SweepFreq, VAR_ID SweepFreqId, double startfreq, double stopfreq, int points)
{
	ReturnStatus rs;

	int n;
	int dfreq = (int)((stopfreq-startfreq)/(points-1));
	printf("\nStep freq: %d, Points: %d\n", dfreq, points);
	double freq;
	int numpoint = 0;
	float maxAmp;
	int maxAmpIndex;

	for(freq = startfreq; freq <= stopfreq; freq=freq+dfreq)
	{
		rs = CONFIG_SetCenterFreq(freq);
		
		SweepAmp[numpoint] = get_spectrum(ssID, traceAmp, traceAmpId, traceFreq, traceFreqId);
		SweepFreq[numpoint] = freq;
		printf("\nFreq: %f, Amp: %f, Point: %d\n", SweepAmp[numpoint], SweepFreq[numpoint], numpoint);
		
		seq_pvPut(ssID, SweepAmpId, SYNC);
	
		seq_pvPut(ssID, SweepFreqId, SYNC);
		numpoint++;
	}
	
	for (n = numpoint; n < 1000; n++)
	{
		SweepFreq[n] = SweepFreq[numpoint-1];
		SweepAmp[n] = SweepAmp[numpoint-1];
	}

	seq_pvPut(ssID, SweepAmpId, SYNC);

	seq_pvPut(ssID, SweepFreqId, SYNC);
}

static void init()
{
	ReturnStatus rs;
	int numDev;
	int devID[DEVSRCH_MAX_NUM_DEVICES];
	char devSN[DEVSRCH_MAX_NUM_DEVICES][DEVSRCH_SERIAL_MAX_STRLEN];
	char devType[DEVSRCH_MAX_NUM_DEVICES][DEVSRCH_TYPE_MAX_STRLEN];
	rs = DEVICE_Search(&numDev, devID, devSN, devType);
	
	printf("\nDevices detected: %d\n",numDev);

	rs = DEVICE_Connect(devID[0]);
	
	if (rs)
	{
		printf("\nERROR: %s", DEVICE_GetErrorString(rs));
	}
	//If there is no error connecting, then print the name of the connected device.
	else
		printf("\nCONNECTED TO: %s", devType[0]);
	
}

void init2(SS_ID ssID, float* traceAmp, VAR_ID traceAmpId, double* traceFreq, VAR_ID traceFreqId)
{
	//Search for devices.
	int numDev=2;
	int devID[DEVSRCH_MAX_NUM_DEVICES];
	char devSN[DEVSRCH_MAX_NUM_DEVICES][DEVSRCH_SERIAL_MAX_STRLEN];
	char devType[DEVSRCH_MAX_NUM_DEVICES][DEVSRCH_TYPE_MAX_STRLEN];
	wchar_t **deviceSerial;
	ReturnStatus rs;
	rs = DEVICE_Search(&numDev, devID, devSN, devType);
	
	printf("\nDevices detected: %d\n",numDev);
	printf("\nDevID: %d, Serial Number: %s, Device Type: %s\n",devID[0],devSN[0],devType[0]);
	
	if (numDev == 0)
	{
		printf("\nCan't find device connected");
		goto end;
	}
	
	//Connect to the first device found.
	rs = DEVICE_Connect(devID[0]);
	
	//The following is an example on how to use the return status of an API function.
	//For simplicity, it will not be used in the rest of the program.
	//This is a fatal error: the device could not be connected.
	if (rs)
	{
		printf("\nERROR: %s", DEVICE_GetErrorString(rs));
		goto end;
	}
	//If there is no error connecting, then print the name of the connected device.
	else
		printf("\nCONNECTED TO: %s", devType[0]);
	
	// Tracking Generator Setup
	bool trkgen_aval, trkgen_en;
	double trkgen_leveldBm = -10.0;
	rs = TRKGEN_GetHwInstalled(&trkgen_aval);
	printf("\nTrack Generator Installed: %d", trkgen_aval);
	
	if (trkgen_aval)
	{
		rs = TRKGEN_SetEnable(true);
		rs = TRKGEN_GetEnable(&trkgen_en);
		printf("\nTrack gen enable: %d", trkgen_en);
		// Range: 43 dBm to –3dBm
		rs = TRKGEN_SetOutputLevel(trkgen_leveldBm);
		rs = TRKGEN_GetOutputLevel(&trkgen_leveldBm);
		printf("\nTrack Output Level: %f", trkgen_leveldBm);
	}
	
	//bool TgBrEnable = false;
	//rs = SetTgBridgeEnable(TgBrEnable);
	//rs = GetTgBridgeEnable(&TgBrEnable);
	//printf("\nTgBrEnable %d\n",TgBrEnable);
	//rs = SetTgSweepParams(450e6, 550e6, 101, 10);
	//printf("\nSetTgSweepParams: %s", DEVICE_GetErrorString(rs));
	//       Track Generator Sweep not working
	//rs = TgSweepRun();
	//printf("\nTgSweepRun: %s", DEVICE_GetErrorString(rs));
	
	
	//Assign a trace to use. In this example, use trace 1 of 3.
	SpectrumTraces traceID = SpectrumTrace1;
	double span = 100e3;//The span of the trace.
	double rbw = 100; //Resolution bandwidth.

	//SpectrumWindow_Kaiser = 0,
	//SpectrumWindow_Mil6dB = 1,
	//SpectrumWindow_BlackmanHarris = 2,
	//SpectrumWindow_Rectangle = 3,
	//SpectrumWindow_FlatTop = 4,
	//SpectrumWindow_Hann = 5

	SpectrumWindows window = SpectrumWindow_Kaiser;//Use the default window (Kaiser).
	SpectrumDetectors detector = SpectrumDetector_PosPeak;//Use the default detector (positive peak).
	SpectrumVerticalUnits vertunits = SpectrumVerticalUnit_dBm;//Use the default vertical units (dBm).
	int traceLength = 10401;//801, 2401, 4001, 8001,10401, 16001, 32001, and 64001 points
	int numTraces = 1;//This will be the number of traces to acquire.
	char fn[10] = "TRACE.txt";//This will be the output filename.

	//Get the limits for the spectrum acquisition control settings.
	Spectrum_Limits salimits;
	rs = SPECTRUM_GetLimits(&salimits);
	
	printf("\nSpectrum limits\n");
	printf("\nMax span: %f", salimits.maxSpan);
	printf("\nMin span: %f", salimits.minSpan);
	printf("\nMax rbw: %f", salimits.maxRBW);
	printf("\nMin rbw: %f", salimits.minRBW);
	printf("\nMax vbw: %f", salimits.maxVBW);
	printf("\nMin vbw: %f", salimits.minVBW);
	printf("\nMax trace length: %d", salimits.maxTraceLength);
	printf("\nMin trace length: %d", salimits.minTraceLength);

	if (span > salimits.maxSpan) span = salimits.maxSpan;//You can use this information to check the limits.

	printf("\nActual span: %f", span);

	// Set SA controls to default, and get the control values.
	Spectrum_Settings setSettings, getSettings;
	rs = SPECTRUM_SetDefault();
	double cf = 500e6;
	rs = CONFIG_SetCenterFreq(cf);
	double refLevel;
	rs = CONFIG_GetReferenceLevel(&refLevel);
	printf("\nRef level: %f\n",refLevel);
	//rs = SPECTRUM_GetSettings(&setSettings);

	//Assign user settings to settings struct.
	setSettings.span = span;
	setSettings.rbw = rbw;
	setSettings.enableVBW = false;
	setSettings.vbw = 100;
	setSettings.traceLength = traceLength;
	setSettings.window = window;
	setSettings.verticalUnit = vertunits;

	//Register the settings.
	rs = SPECTRUM_SetSettings(setSettings);
	
	//Retrieve the settings info.
	rs = SPECTRUM_GetSettings(&getSettings);
	printf("\nTrace lenght: %d\n", getSettings.traceLength);
	printf("\nStart freq: %f\n", getSettings.actualStartFreq);
	printf("\nStop freq: %f\n", getSettings.actualStopFreq);
	printf("\nFreq step size: %f\n", getSettings.actualFreqStepSize);
	printf("\nFreq span: %f\n", getSettings.span);

	//Allocate memory array for spectrum output vector.
	float* pTraceData = (float*)malloc(sizeof(float)*traceLength);
	
	//Start the trace capture.
	rs = SPECTRUM_SetEnable(true);
	printf("\nTrace capture is starting...\n");
	bool isActive = true;
	int waitTimeoutMsec= 1000;//Maximum allowable wait time for each data acquistion.
	int numTimeouts = 20;//Maximum amount of attempts to acquire data if a timeout occurs.
	//Note: the total wait time to acquire data is waitTimeoutMsec x numTimeouts.
    int timeoutCount = 0;//Variable to track the timeouts.
	int traceCount = 0;
	bool traceReady = false;
	int outTracePoints;

	rs = SPECTRUM_AcquireTrace();
	//Wait for the trace to be ready.
	rs = SPECTRUM_WaitForTraceReady(waitTimeoutMsec,&traceReady);

	/*
	while(isActive)
	{
		printf("\nTrace %d\n",traceCount);
		rs = SPECTRUM_AcquireTrace();
		//Wait for the trace to be ready.
		rs = SPECTRUM_WaitForTraceReady(waitTimeoutMsec,&traceReady);
		printf("\nSPECTRUM_WaitForTraceReady: %s, traceReady: %d", DEVICE_GetErrorString(rs), traceReady);
		if(traceReady)
		{
			//Get spectrum trace data.
			rs = SPECTRUM_GetTrace(traceID,traceLength,traceAmp,&outTracePoints);
			//Get traceInfo struct.
			Spectrum_TraceInfo traceInfo;
			rs = SPECTRUM_GetTraceInfo(&traceInfo);
			//You can use this information to report any non-zero bits in AcqDataStatus word, for example.
			if (traceInfo.acqDataStatus != 0)
				printf("\nTrace:%i AcqDataStatus:0x%04X\n", traceCount, traceInfo.acqDataStatus);
			
			//Write data to the open file.

		traceCount++;

		}
		else timeoutCount++;

		//Stop acquiring traces when the limit is reached or the wait time is exceeded.
		if(numTraces == traceCount || timeoutCount == numTimeouts)
			isActive = false;
	}

	int n;
	double freq, stepfreq;
	freq = getSettings.actualStartFreq;
	stepfreq = getSettings.actualFreqStepSize;
	printf("\nFreq: %f, Step: %f\n", freq, stepfreq);
	
	for (n = 0; n < traceLength; n++)
	{
		traceFreq[n] = freq;
		freq = freq + stepfreq;
	}

	seq_pvPut(ssID, traceAmpId, SYNC);

	seq_pvPut(ssID, traceFreqId, SYNC);
	*/
	//Disconnect the device and finish up.
	//rs = SPECTRUM_SetEnable(false);
	if(pTraceData)
		free(pTraceData);

	//rs = DEVICE_Stop();
	//rs = DEVICE_Disconnect();

	end:
	printf("\nSpectrum trace acquisition routine complete.\n");
}

int trkgen_enable(int enable)
{
	bool getenable;
	bool trkgen_aval;
	TRKGEN_GetHwInstalled(&trkgen_aval);
	if (trkgen_aval)
	{
		if (enable == 1)
		{
			printf("\nTRKGEN_SetEnable(true)\n");
			//TRKGEN_SetEnable(true);
			SetTgEnable(true);
		}
		else
		{
			printf("\nTRKGEN_SetEnable(false)\n");
			//TRKGEN_SetEnable(false);
			SetTgEnable(false);
		}
		GetTgEnable(&getenable);
		printf("\nTRKGEN_GetEnable %d\n", getenable);
		return (int)getenable;
	}
	else
	{
		printf("\nTracking generator hardware not installed\n");
		return 0;
	}
}

void trkgen_level(double trkgenLevel)
{
	bool trkgen_aval;
	TRKGEN_GetHwInstalled(&trkgen_aval);
	if (trkgen_aval)
	{
		TRKGEN_SetOutputLevel(trkgenLevel);
		TRKGEN_GetOutputLevel(&trkgenLevel);
		printf("\nTracking generator level output %f dBm\n", trkgenLevel);
	}
}


# line 477 "../sncExample.stt"
//----------------------------------------
# line 478 "../sncExample.stt"
// 	      Monitored PVs
# line 479 "../sncExample.stt"
//----------------------------------------
# line 490 "../sncExample.stt"
static const EF_ID freqFlag = 1;
# line 496 "../sncExample.stt"
static const EF_ID refLevelFlag = 2;
# line 526 "../sncExample.stt"
static const EF_ID spanFlag = 3;
# line 538 "../sncExample.stt"
static const EF_ID rbwFlag = 4;
# line 550 "../sncExample.stt"
static const EF_ID vbwEnFlag = 5;
# line 556 "../sncExample.stt"
static const EF_ID vbwFlag = 6;
# line 562 "../sncExample.stt"
static const EF_ID traceLenFlag = 7;
# line 576 "../sncExample.stt"
static const EF_ID trkgenFlag = 8;
# line 582 "../sncExample.stt"
static const EF_ID trkgenLevelFlag = 9;
# line 597 "../sncExample.stt"
Spectrum_Limits salimits;

/* Variable declarations */
struct seqg_vars {
# line 481 "../sncExample.stt"
	double maxFreq;
# line 484 "../sncExample.stt"
	double minFreq;
# line 487 "../sncExample.stt"
	double freq;
# line 493 "../sncExample.stt"
	double refLevel;
# line 499 "../sncExample.stt"
	double StartFreq;
# line 503 "../sncExample.stt"
	double StopFreq;
# line 507 "../sncExample.stt"
	int SweepPoints;
# line 511 "../sncExample.stt"
	double maxSpan;
# line 514 "../sncExample.stt"
	double minSpan;
# line 517 "../sncExample.stt"
	double hoprSpan;
# line 520 "../sncExample.stt"
	double loprSpan;
# line 523 "../sncExample.stt"
	double span;
# line 529 "../sncExample.stt"
	double maxRBW;
# line 532 "../sncExample.stt"
	double minRBW;
# line 535 "../sncExample.stt"
	double rbw;
# line 541 "../sncExample.stt"
	double maxVBW;
# line 544 "../sncExample.stt"
	double minVBW;
# line 547 "../sncExample.stt"
	int vbwEn;
# line 553 "../sncExample.stt"
	double vbw;
# line 559 "../sncExample.stt"
	int traceLenOpt;
# line 559 "../sncExample.stt"
	int traceLenMenu[8];
# line 565 "../sncExample.stt"
	int getSpec;
# line 569 "../sncExample.stt"
	int Sweep;
# line 573 "../sncExample.stt"
	int trkgen;
# line 579 "../sncExample.stt"
	double trkgenLevel;
# line 585 "../sncExample.stt"
	float traceAmp[64001];
# line 588 "../sncExample.stt"
	double traceFreq[64001];
# line 591 "../sncExample.stt"
	float SweepAmp[1000];
# line 594 "../sncExample.stt"
	double SweepFreq[1000];
};


/* Function declarations */

#define seqg_var (*(struct seqg_vars *const *)seqg_env)

/* Program init func */
static void seqg_init(PROG_ID seqg_env)
{
	{
# line 559 "../sncExample.stt"
	static int seqg_initvar_traceLenMenu[8] = {801, 2401, 4001, 8001, 10401, 16001, 32001, 64001};
	memcpy(&seqg_var->traceLenMenu, &seqg_initvar_traceLenMenu, sizeof(seqg_initvar_traceLenMenu));
	}
}

/****** Code for state "init" in state set "ss1" ******/

/* Event function for state "init" in state set "ss1" */
static seqBool seqg_event_ss1_0_init(SS_ID seqg_env, int *seqg_ptrn, int *seqg_pnst)
{
# line 602 "../sncExample.stt"
	if (seq_delay(seqg_env, 0.1))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 0;
		return TRUE;
	}
	return FALSE;
}

/* Action function for state "init" in state set "ss1" */
static void seqg_action_ss1_0_init(SS_ID seqg_env, int seqg_trn, int *seqg_pnst)
{
	switch(seqg_trn)
	{
	case 0:
		{
# line 603 "../sncExample.stt"
			init2(ssId, seqg_var->traceAmp, seq_pvIndex(seqg_env, 24/*traceAmp*/), seqg_var->traceFreq, seq_pvIndex(seqg_env, 25/*traceFreq*/));
			
		
		ReturnStatus rs;
		// Get device limits

		double maxCF, minCF;
		rs = CONFIG_GetMaxCenterFreq(&maxCF);
		rs = CONFIG_GetMinCenterFreq(&minCF);

		//Spectrum_Limits salimits;
		rs = SPECTRUM_GetLimits(&salimits);
		
		printf("\nSpectrum limits\n");
		printf("\nMax span: %f", salimits.maxSpan);
		printf("\nMin span: %f", salimits.minSpan);
		printf("\nMax rbw: %f", salimits.maxRBW);
		printf("\nMin rbw: %f", salimits.minRBW);
		printf("\nMax vbw: %f", salimits.maxVBW);
		printf("\nMin vbw: %f", salimits.minVBW);
		printf("\nMax trace length: %d", salimits.maxTraceLength);
		printf("\nMin trace length: %d", salimits.minTraceLength);
	
		// Set SA controls to default, and get the control values.
		Spectrum_Settings getSettings;
		rs = SPECTRUM_SetDefault();
		SPECTRUM_GetSettings(&getSettings);
		printf("\n********* Initial parameters *********\n");
		printf("\nSpan: %f\n", getSettings.span);
		printf("\nRBW: %f\n", getSettings.rbw);
		printf("\nVBW: %f\n", getSettings.vbw);
		printf("\nTrace lenght: %d\n", getSettings.traceLength);
		printf("\nStart freq: %f\n", getSettings.actualStartFreq);
		printf("\nStop freq: %f\n", getSettings.actualStopFreq);
		printf("\nFreq step size: %f\n", getSettings.actualFreqStepSize);
		printf("\nFreq span: %f\n", getSettings.span);
		printf("\n********* Initial parameters *********\n");
		
		
# line 644 "../sncExample.stt"
			seqg_var->maxFreq = maxCF;
# line 645 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 0/*maxFreq*/, DEFAULT, DEFAULT_TIMEOUT);
# line 647 "../sncExample.stt"
			seqg_var->minFreq = minCF;
# line 648 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 1/*minFreq*/, DEFAULT, DEFAULT_TIMEOUT);
# line 650 "../sncExample.stt"
			seqg_var->maxSpan = salimits.maxSpan;
# line 651 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 7/*maxSpan*/, DEFAULT, DEFAULT_TIMEOUT);
# line 653 "../sncExample.stt"
			seqg_var->minSpan = salimits.minSpan;
# line 654 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 8/*minSpan*/, DEFAULT, DEFAULT_TIMEOUT);
# line 656 "../sncExample.stt"
			seqg_var->hoprSpan = salimits.maxSpan;
# line 657 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 9/*hoprSpan*/, DEFAULT, DEFAULT_TIMEOUT);
# line 659 "../sncExample.stt"
			seqg_var->loprSpan = salimits.minSpan;
# line 660 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 10/*loprSpan*/, DEFAULT, DEFAULT_TIMEOUT);
# line 662 "../sncExample.stt"
			seqg_var->maxRBW = salimits.maxRBW;
# line 663 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 12/*maxRBW*/, DEFAULT, DEFAULT_TIMEOUT);
# line 665 "../sncExample.stt"
			seqg_var->minRBW = salimits.minRBW;
# line 666 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 13/*minRBW*/, DEFAULT, DEFAULT_TIMEOUT);
# line 668 "../sncExample.stt"
			seqg_var->maxVBW = salimits.maxVBW;
# line 669 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 15/*maxVBW*/, DEFAULT, DEFAULT_TIMEOUT);
# line 671 "../sncExample.stt"
			seqg_var->minVBW = salimits.minVBW;
# line 672 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 16/*minVBW*/, DEFAULT, DEFAULT_TIMEOUT);
# line 686 "../sncExample.stt"
			printf("sncExample: Startup delay over\n");
		}
		return;
	}
}

/****** Code for state "opr" in state set "ss1" ******/

/* Event function for state "opr" in state set "ss1" */
static seqBool seqg_event_ss1_0_opr(SS_ID seqg_env, int *seqg_ptrn, int *seqg_pnst)
{
# line 693 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, spanFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 0;
		return TRUE;
	}
# line 698 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, rbwFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 1;
		return TRUE;
	}
# line 703 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, vbwFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 2;
		return TRUE;
	}
# line 708 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, vbwEnFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 3;
		return TRUE;
	}
# line 713 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, traceLenFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 4;
		return TRUE;
	}
# line 718 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, freqFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 5;
		return TRUE;
	}
# line 723 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, refLevelFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 6;
		return TRUE;
	}
# line 728 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, trkgenFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 7;
		return TRUE;
	}
# line 733 "../sncExample.stt"
	if (seq_efTestAndClear(seqg_env, trkgenLevelFlag))
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 8;
		return TRUE;
	}
# line 737 "../sncExample.stt"
	if (seqg_var->getSpec == 1)
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 9;
		return TRUE;
	}
# line 741 "../sncExample.stt"
	if (seqg_var->Sweep == 1)
	{
		*seqg_pnst = 1;
		*seqg_ptrn = 10;
		return TRUE;
	}
	return FALSE;
}

/* Action function for state "opr" in state set "ss1" */
static void seqg_action_ss1_0_opr(SS_ID seqg_env, int seqg_trn, int *seqg_pnst)
{
	switch(seqg_trn)
	{
	case 0:
		{
# line 694 "../sncExample.stt"
			printf("Span change to: %f\n", seqg_var->span);
# line 695 "../sncExample.stt"
			spectrum_settings(seqg_var->span, seqg_var->rbw, seqg_var->vbw, seqg_var->vbwEn, seqg_var->traceLenMenu[seqg_var->traceLenOpt]);
		}
		return;
	case 1:
		{
# line 699 "../sncExample.stt"
			printf("Rbw change to: %f\n", seqg_var->rbw);
# line 700 "../sncExample.stt"
			spectrum_settings(seqg_var->span, seqg_var->rbw, seqg_var->vbw, seqg_var->vbwEn, seqg_var->traceLenMenu[seqg_var->traceLenOpt]);
		}
		return;
	case 2:
		{
# line 704 "../sncExample.stt"
			printf("Vbw change to: %f\n", seqg_var->vbw);
# line 705 "../sncExample.stt"
			spectrum_settings(seqg_var->span, seqg_var->rbw, seqg_var->vbw, seqg_var->vbwEn, seqg_var->traceLenMenu[seqg_var->traceLenOpt]);
		}
		return;
	case 3:
		{
# line 709 "../sncExample.stt"
			printf("Vbw enable: %s\n", seqg_var->vbwEn ? "ON" : "OFF");
# line 710 "../sncExample.stt"
			spectrum_settings(seqg_var->span, seqg_var->rbw, seqg_var->vbw, seqg_var->vbwEn, seqg_var->traceLenMenu[seqg_var->traceLenOpt]);
		}
		return;
	case 4:
		{
# line 714 "../sncExample.stt"
			printf("TraceLen change to: %d\n", seqg_var->traceLenMenu[seqg_var->traceLenOpt]);
# line 715 "../sncExample.stt"
			spectrum_settings(seqg_var->span, seqg_var->rbw, seqg_var->vbw, seqg_var->vbwEn, seqg_var->traceLenMenu[seqg_var->traceLenOpt]);
		}
		return;
	case 5:
		{
# line 719 "../sncExample.stt"
			printf("Frequency change to: %f\n", seqg_var->freq);
# line 720 "../sncExample.stt"
			center_freq(seqg_var->freq);
		}
		return;
	case 6:
		{
# line 724 "../sncExample.stt"
			printf("Reference level change to: %f\n", seqg_var->refLevel);
# line 725 "../sncExample.stt"
			ref_level(seqg_var->refLevel);
		}
		return;
	case 7:
		{
# line 729 "../sncExample.stt"
			seqg_var->trkgen = trkgen_enable(seqg_var->trkgen);
# line 730 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 22/*trkgen*/, DEFAULT, DEFAULT_TIMEOUT);
		}
		return;
	case 8:
		{
# line 734 "../sncExample.stt"
			trkgen_level(seqg_var->trkgenLevel);
		}
		return;
	case 9:
		{
# line 738 "../sncExample.stt"
			get_spectrum(ssId, seqg_var->traceAmp, seq_pvIndex(seqg_env, 24/*traceAmp*/), seqg_var->traceFreq, seq_pvIndex(seqg_env, 25/*traceFreq*/));
		}
		return;
	case 10:
		{
# line 742 "../sncExample.stt"
			get_spectrum_sweep(ssId, seqg_var->traceAmp, seq_pvIndex(seqg_env, 24/*traceAmp*/), seqg_var->traceFreq, seq_pvIndex(seqg_env, 25/*traceFreq*/), seqg_var->SweepAmp, seq_pvIndex(seqg_env, 26/*SweepAmp*/), seqg_var->SweepFreq, seq_pvIndex(seqg_env, 27/*SweepFreq*/), seqg_var->StartFreq, seqg_var->StopFreq, seqg_var->SweepPoints);
# line 743 "../sncExample.stt"
			seqg_var->Sweep = 0;
# line 744 "../sncExample.stt"
			seq_pvPutTmo(seqg_env, 21/*Sweep*/, DEFAULT, DEFAULT_TIMEOUT);
		}
		return;
	}
}

#undef seqg_var

/************************ Tables ************************/

/* Channel table */
static seqChan seqg_chans[] = {
	/* chName, offset, varName, varType, count, eventNum, efId, monitored, queueSize, queueIndex */
	{"{user}:freq.DRVH", offsetof(struct seqg_vars, maxFreq), "maxFreq", P_DOUBLE, 1, 10, 0, 0, 0, 0},
	{"{user}:freq.DRVL", offsetof(struct seqg_vars, minFreq), "minFreq", P_DOUBLE, 1, 11, 0, 0, 0, 0},
	{"{user}:freq", offsetof(struct seqg_vars, freq), "freq", P_DOUBLE, 1, 12, 1, 1, 0, 0},
	{"{user}:refLevel", offsetof(struct seqg_vars, refLevel), "refLevel", P_DOUBLE, 1, 13, 2, 1, 0, 0},
	{"{user}:StartFreq", offsetof(struct seqg_vars, StartFreq), "StartFreq", P_DOUBLE, 1, 14, 0, 1, 0, 0},
	{"{user}:StopFreq", offsetof(struct seqg_vars, StopFreq), "StopFreq", P_DOUBLE, 1, 15, 0, 1, 0, 0},
	{"{user}:SweepPoints", offsetof(struct seqg_vars, SweepPoints), "SweepPoints", P_INT, 1, 16, 0, 1, 0, 0},
	{"{user}:span.DRVH", offsetof(struct seqg_vars, maxSpan), "maxSpan", P_DOUBLE, 1, 17, 0, 0, 0, 0},
	{"{user}:span.DRVL", offsetof(struct seqg_vars, minSpan), "minSpan", P_DOUBLE, 1, 18, 0, 0, 0, 0},
	{"{user}:span.HOPR", offsetof(struct seqg_vars, hoprSpan), "hoprSpan", P_DOUBLE, 1, 19, 0, 0, 0, 0},
	{"{user}:span.LOPR", offsetof(struct seqg_vars, loprSpan), "loprSpan", P_DOUBLE, 1, 20, 0, 0, 0, 0},
	{"{user}:span", offsetof(struct seqg_vars, span), "span", P_DOUBLE, 1, 21, 3, 1, 0, 0},
	{"{user}:rbw.DRVH", offsetof(struct seqg_vars, maxRBW), "maxRBW", P_DOUBLE, 1, 22, 0, 0, 0, 0},
	{"{user}:rbw.DRVL", offsetof(struct seqg_vars, minRBW), "minRBW", P_DOUBLE, 1, 23, 0, 0, 0, 0},
	{"{user}:rbw", offsetof(struct seqg_vars, rbw), "rbw", P_DOUBLE, 1, 24, 4, 1, 0, 0},
	{"{user}:vbw.DRVH", offsetof(struct seqg_vars, maxVBW), "maxVBW", P_DOUBLE, 1, 25, 0, 0, 0, 0},
	{"{user}:vbw.DRVL", offsetof(struct seqg_vars, minVBW), "minVBW", P_DOUBLE, 1, 26, 0, 0, 0, 0},
	{"{user}:vbwEn", offsetof(struct seqg_vars, vbwEn), "vbwEn", P_INT, 1, 27, 5, 1, 0, 0},
	{"{user}:vbw", offsetof(struct seqg_vars, vbw), "vbw", P_DOUBLE, 1, 28, 6, 1, 0, 0},
	{"{user}:traceLenOpt", offsetof(struct seqg_vars, traceLenOpt), "traceLenOpt", P_INT, 1, 29, 7, 1, 0, 0},
	{"{user}:getSpec", offsetof(struct seqg_vars, getSpec), "getSpec", P_INT, 1, 30, 0, 1, 0, 0},
	{"{user}:Sweep", offsetof(struct seqg_vars, Sweep), "Sweep", P_INT, 1, 31, 0, 1, 0, 0},
	{"{user}:trkgenEn", offsetof(struct seqg_vars, trkgen), "trkgen", P_INT, 1, 32, 8, 1, 0, 0},
	{"{user}:trkgenLevel", offsetof(struct seqg_vars, trkgenLevel), "trkgenLevel", P_DOUBLE, 1, 33, 9, 1, 0, 0},
	{"{user}:traceAmp", offsetof(struct seqg_vars, traceAmp), "traceAmp", P_FLOAT, 64001, 34, 0, 0, 0, 0},
	{"{user}:traceFreq", offsetof(struct seqg_vars, traceFreq), "traceFreq", P_DOUBLE, 64001, 35, 0, 0, 0, 0},
	{"{user}:SweepAmpData", offsetof(struct seqg_vars, SweepAmp), "SweepAmp", P_FLOAT, 1000, 36, 0, 0, 0, 0},
	{"{user}:SweepFreqData", offsetof(struct seqg_vars, SweepFreq), "SweepFreq", P_DOUBLE, 1000, 37, 0, 0, 0, 0},
};

/* Event masks for state set "ss1" */
static const seqMask seqg_mask_ss1_0_init[] = {
	0x00000000,
	0x00000000,
};
static const seqMask seqg_mask_ss1_0_opr[] = {
	0xc00003fe,
	0x00000000,
};

/* State table for state set "ss1" */
static seqState seqg_states_ss1[] = {
	{
	/* state name */        "init",
	/* action function */   seqg_action_ss1_0_init,
	/* event function */    seqg_event_ss1_0_init,
	/* entry function */    0,
	/* exit function */     0,
	/* event mask array */  seqg_mask_ss1_0_init,
	/* state options */     (0)
	},
	{
	/* state name */        "opr",
	/* action function */   seqg_action_ss1_0_opr,
	/* event function */    seqg_event_ss1_0_opr,
	/* entry function */    0,
	/* exit function */     0,
	/* event mask array */  seqg_mask_ss1_0_opr,
	/* state options */     (0)
	},
};

/* State set table */
static seqSS seqg_statesets[] = {
	{
	/* state set name */    "ss1",
	/* states */            seqg_states_ss1,
	/* number of states */  2
	},
};

/* Program table (global) */
seqProgram sncExample = {
	/* magic number */      2002001,
	/* program name */      "sncExample",
	/* channels */          seqg_chans,
	/* num. channels */     28,
	/* state sets */        seqg_statesets,
	/* num. state sets */   1,
	/* user var size */     sizeof(struct seqg_vars),
	/* param */             "",
	/* num. event flags */  9,
	/* encoded options */   (0 | OPT_CONN | OPT_NEWEF | OPT_REENT),
	/* init func */         seqg_init,
	/* entry func */        0,
	/* exit func */         0,
	/* num. queues */       0
};

/* Register sequencer commands and program */
#include "epicsExport.h"
static void sncExampleRegistrar (void) {
    seqRegisterSequencerCommands();
    seqRegisterSequencerProgram (&sncExample);
}
epicsExportRegistrar(sncExampleRegistrar);
