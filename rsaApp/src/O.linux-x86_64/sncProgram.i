# 1 "../sncProgram.st"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "../sncProgram.st"
# 1 "./../sncExample.stt" 1


program sncExample


foreign ssId;

%%#include <stdlib.h>
%%#include "/opt/include/RSA_API.h"

%{

static bool spectrum_settings(double span, double rbw, double vbw, int vbwEn, int traceLen)
{
 ReturnStatus rs;

 SpectrumWindows window = SpectrumWindow_Kaiser;
 SpectrumVerticalUnits verticalUnit = SpectrumVerticalUnit_dBm;

 Spectrum_Settings setSettings, getSettings;
 setSettings.span = span;
 setSettings.rbw = rbw;
 setSettings.enableVBW = vbwEn;
 setSettings.vbw = vbw;
 setSettings.traceLength = traceLen;
 setSettings.window = window;
 setSettings.verticalUnit = verticalUnit;

 rs = SPECTRUM_SetSettings(setSettings);


 rs = SPECTRUM_GetSettings(&getSettings);



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

 int traceLength = getSettings.traceLength;
 double freq, stepfreq;
 freq = getSettings.actualStartFreq;
 stepfreq = getSettings.actualFreqStepSize;


 for (n = 0; n < 64001; n++)
 {
  traceFreq[n] = freq/1000000.0;
  if (n < traceLength)
   freq = freq + stepfreq;
 }



 rs = SPECTRUM_SetEnable(true);


 bool isActive = true;
 int waitTimeoutMsec= 500;
 int numTimeouts = 2;

    int timeoutCount = 0;
 int traceCount = 0;
 bool traceReady = false;
 int outTracePoints;

 float* pTraceData = (float*)malloc(sizeof(float)*traceLength);

 while(isActive)
 {

  rs = SPECTRUM_AcquireTrace();


  rs = SPECTRUM_WaitForTraceReady(waitTimeoutMsec,&traceReady);
  printf("\nSPECTRUM_WaitForTraceReady: %s, traceReady: %d", DEVICE_GetErrorString(rs), traceReady);
  if(traceReady)
  {

   rs = SPECTRUM_GetTrace(0,traceLength,traceAmp,&outTracePoints);

   Spectrum_TraceInfo traceInfo;
   rs = SPECTRUM_GetTraceInfo(&traceInfo);

   if (traceInfo.acqDataStatus != 0)
    printf("\nTrace:%i AcqDataStatus:0x%04X\n", traceCount, traceInfo.acqDataStatus);

  traceCount++;

  }
  else timeoutCount++;


  if(timeoutCount == numTimeouts || traceCount == 1)
  {
   if (traceCount == 0)
   {
    printf("\ninit2()\n");
    rs = SPECTRUM_SetEnable(false);
    rs = SPECTRUM_SetEnable(true);

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

 else
  printf("\nCONNECTED TO: %s", devType[0]);

}

void init2(SS_ID ssID, float* traceAmp, VAR_ID traceAmpId, double* traceFreq, VAR_ID traceFreqId)
{

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


 rs = DEVICE_Connect(devID[0]);




 if (rs)
 {
  printf("\nERROR: %s", DEVICE_GetErrorString(rs));
  goto end;
 }

 else
  printf("\nCONNECTED TO: %s", devType[0]);


 bool trkgen_aval, trkgen_en;
 double trkgen_leveldBm = -10.0;
 rs = TRKGEN_GetHwInstalled(&trkgen_aval);
 printf("\nTrack Generator Installed: %d", trkgen_aval);

 if (trkgen_aval)
 {
  rs = TRKGEN_SetEnable(true);
  rs = TRKGEN_GetEnable(&trkgen_en);
  printf("\nTrack gen enable: %d", trkgen_en);

  rs = TRKGEN_SetOutputLevel(trkgen_leveldBm);
  rs = TRKGEN_GetOutputLevel(&trkgen_leveldBm);
  printf("\nTrack Output Level: %f", trkgen_leveldBm);
 }
# 289 "./../sncExample.stt"
 SpectrumTraces traceID = SpectrumTrace1;
 double span = 100e3;
 double rbw = 100;
# 300 "./../sncExample.stt"
 SpectrumWindows window = SpectrumWindow_Kaiser;
 SpectrumDetectors detector = SpectrumDetector_PosPeak;
 SpectrumVerticalUnits vertunits = SpectrumVerticalUnit_dBm;
 int traceLength = 10401;
 int numTraces = 1;
 char fn[10] = "TRACE.txt";


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

 if (span > salimits.maxSpan) span = salimits.maxSpan;

 printf("\nActual span: %f", span);


 Spectrum_Settings setSettings, getSettings;
 rs = SPECTRUM_SetDefault();
 double cf = 500e6;
 rs = CONFIG_SetCenterFreq(cf);
 double refLevel;
 rs = CONFIG_GetReferenceLevel(&refLevel);
 printf("\nRef level: %f\n",refLevel);



 setSettings.span = span;
 setSettings.rbw = rbw;
 setSettings.enableVBW = false;
 setSettings.vbw = 100;
 setSettings.traceLength = traceLength;
 setSettings.window = window;
 setSettings.verticalUnit = vertunits;


 rs = SPECTRUM_SetSettings(setSettings);


 rs = SPECTRUM_GetSettings(&getSettings);
 printf("\nTrace lenght: %d\n", getSettings.traceLength);
 printf("\nStart freq: %f\n", getSettings.actualStartFreq);
 printf("\nStop freq: %f\n", getSettings.actualStopFreq);
 printf("\nFreq step size: %f\n", getSettings.actualFreqStepSize);
 printf("\nFreq span: %f\n", getSettings.span);


 float* pTraceData = (float*)malloc(sizeof(float)*traceLength);


 rs = SPECTRUM_SetEnable(true);
 printf("\nTrace capture is starting...\n");
 bool isActive = true;
 int waitTimeoutMsec= 1000;
 int numTimeouts = 20;

    int timeoutCount = 0;
 int traceCount = 0;
 bool traceReady = false;
 int outTracePoints;

 rs = SPECTRUM_AcquireTrace();

 rs = SPECTRUM_WaitForTraceReady(waitTimeoutMsec,&traceReady);
# 423 "./../sncExample.stt"
 if(pTraceData)
  free(pTraceData);




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

   SetTgEnable(true);
  }
  else
  {
   printf("\nTRKGEN_SetEnable(false)\n");

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

}%

%%
%%
%%

double maxFreq;
assign maxFreq to "{user}:freq.DRVH";

double minFreq;
assign minFreq to "{user}:freq.DRVL";

double freq;
assign freq to "{user}:freq";
monitor freq;
evflag freqFlag;
sync freq freqFlag;

double refLevel;
assign refLevel to "{user}:refLevel";
monitor refLevel;
evflag refLevelFlag;
sync refLevel refLevelFlag;

double StartFreq;
assign StartFreq to "{user}:StartFreq";
monitor StartFreq;

double StopFreq;
assign StopFreq to "{user}:StopFreq";
monitor StopFreq;

int SweepPoints;
assign SweepPoints to "{user}:SweepPoints";
monitor SweepPoints;

double maxSpan;
assign maxSpan to "{user}:span.DRVH";

double minSpan;
assign minSpan to "{user}:span.DRVL";

double hoprSpan;
assign hoprSpan to "{user}:span.HOPR";

double loprSpan;
assign loprSpan to "{user}:span.LOPR";

double span;
assign span to "{user}:span";
monitor span;
evflag spanFlag;
sync span spanFlag;

double maxRBW;
assign maxRBW to "{user}:rbw.DRVH";

double minRBW;
assign minRBW to "{user}:rbw.DRVL";

double rbw;
assign rbw to "{user}:rbw";
monitor rbw;
evflag rbwFlag;
sync rbw rbwFlag;

double maxVBW;
assign maxVBW to "{user}:vbw.DRVH";

double minVBW;
assign minVBW to "{user}:vbw.DRVL";

int vbwEn;
assign vbwEn to "{user}:vbwEn";
monitor vbwEn;
evflag vbwEnFlag;
sync vbwEn vbwEnFlag;

double vbw;
assign vbw to "{user}:vbw";
monitor vbw;
evflag vbwFlag;
sync vbw vbwFlag;

int traceLenOpt, traceLenMenu[8] = {801, 2401, 4001, 8001, 10401, 16001, 32001, 64001};
assign traceLenOpt to "{user}:traceLenOpt";
monitor traceLenOpt;
evflag traceLenFlag;
sync traceLenOpt traceLenFlag;

int getSpec;
assign getSpec to "{user}:getSpec";
monitor getSpec;

int Sweep;
assign Sweep to "{user}:Sweep";
monitor Sweep;

int trkgen;
assign trkgen to "{user}:trkgenEn";
monitor trkgen;
evflag trkgenFlag;
sync trkgen trkgenFlag;

double trkgenLevel;
assign trkgenLevel to "{user}:trkgenLevel";
monitor trkgenLevel;
evflag trkgenLevelFlag;
sync trkgenLevel trkgenLevelFlag;

float traceAmp[64001];
assign traceAmp to "{user}:traceAmp";

double traceFreq[64001];
assign traceFreq to "{user}:traceFreq";

float SweepAmp[1000];
assign SweepAmp to "{user}:SweepAmpData";

double SweepFreq[1000];
assign SweepFreq to "{user}:SweepFreqData";

%%Spectrum_Limits salimits;

ss ss1 {
    state init {

 when (delay(0.1)) {
  init2(ssId, traceAmp, pvIndex(traceAmp), traceFreq, pvIndex(traceFreq));

  %{

  ReturnStatus rs;


  double maxCF, minCF;
  rs = CONFIG_GetMaxCenterFreq(&maxCF);
  rs = CONFIG_GetMinCenterFreq(&minCF);


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

  }%

  maxFreq = maxCF;
  pvPut(maxFreq);

  minFreq = minCF;
  pvPut(minFreq);

  maxSpan = salimits.maxSpan;
  pvPut(maxSpan);

  minSpan = salimits.minSpan;
  pvPut(minSpan);

  hoprSpan = salimits.maxSpan;
  pvPut(hoprSpan);

  loprSpan = salimits.minSpan;
  pvPut(loprSpan);

  maxRBW = salimits.maxRBW;
  pvPut(maxRBW);

  minRBW = salimits.minRBW;
  pvPut(minRBW);

  maxVBW = salimits.maxVBW;
  pvPut(maxVBW);

  minVBW = salimits.minVBW;
  pvPut(minVBW);
# 686 "./../sncExample.stt"
     printf("sncExample: Startup delay over\n");

 } state opr

    }
    state opr {

 when (efTestAndClear(spanFlag)) {
     printf("Span change to: %f\n",span);
     spectrum_settings(span, rbw, vbw, vbwEn, traceLenMenu[traceLenOpt]);
 } state opr

 when (efTestAndClear(rbwFlag)) {
     printf("Rbw change to: %f\n",rbw);
     spectrum_settings(span, rbw, vbw, vbwEn, traceLenMenu[traceLenOpt]);
 } state opr

 when (efTestAndClear(vbwFlag)) {
     printf("Vbw change to: %f\n",vbw);
     spectrum_settings(span, rbw, vbw, vbwEn, traceLenMenu[traceLenOpt]);
 } state opr

 when (efTestAndClear(vbwEnFlag)) {
     printf("Vbw enable: %s\n",vbwEn ? "ON" : "OFF");
     spectrum_settings(span, rbw, vbw, vbwEn, traceLenMenu[traceLenOpt]);
 } state opr

 when (efTestAndClear(traceLenFlag)) {
     printf("TraceLen change to: %d\n",traceLenMenu[traceLenOpt]);
     spectrum_settings(span, rbw, vbw, vbwEn, traceLenMenu[traceLenOpt]);
 } state opr

 when (efTestAndClear(freqFlag)) {
     printf("Frequency change to: %f\n",freq);
     center_freq(freq);
 } state opr

 when (efTestAndClear(refLevelFlag)) {
     printf("Reference level change to: %f\n",refLevel);
     ref_level(refLevel);
 } state opr

 when (efTestAndClear(trkgenFlag)) {
  trkgen = trkgen_enable(trkgen);
  pvPut(trkgen);
 } state opr

 when (efTestAndClear(trkgenLevelFlag)) {
  trkgen_level(trkgenLevel);
 } state opr

 when (getSpec == 1) {
     get_spectrum(ssId, traceAmp, pvIndex(traceAmp), traceFreq, pvIndex(traceFreq));
 } state opr

 when (Sweep == 1) {
     get_spectrum_sweep(ssId, traceAmp, pvIndex(traceAmp), traceFreq, pvIndex(traceFreq), SweepAmp, pvIndex(SweepAmp), SweepFreq, pvIndex(SweepFreq), StartFreq, StopFreq, SweepPoints);
     Sweep = 0;
     pvPut(Sweep);
 } state opr

    }
}
# 1 "../sncProgram.st" 2
