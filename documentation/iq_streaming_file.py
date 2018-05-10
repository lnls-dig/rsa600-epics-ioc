"""
Simple IQ Streaming Example using API for RSA306
Author: Morgan Allison
Date created: 10/5/15
Date edited: 11/18/15
Windows 7 64-bit
Python 2.7.9 64-bit (Anaconda 3.7.0)
NumPy 1.8.1, MatPlotLib 1.3.1
To get Anaconda: http://continuum.io/downloads
Anaconda includes NumPy and MatPlotLib
"""

import os
from ctypes import *
import time

"""
################################################################
C:\Tektronix\RSA306 API\lib\x64 needs to be added to the 
PATH system environment variable
################################################################
"""
os.chdir("C:\\Tektronix\\RSA306 API\\lib\\x64")
rsa300 = WinDLL("RSA300API.dll")

"""search/connect variables"""
longArray = c_long*10
deviceIDs = longArray()
deviceSerial = c_wchar_p('')
numFound = c_int(0)

"""Main SA parameters"""
cf = c_double(1e9)
refLevel = c_double(0)
bwHz_req = c_double(20e6)
bwHz_act = c_double(0)
sRate = c_double(0)
durationMsec = c_int(100)

"""Stream Control Variables"""
filenameBase = c_char_p('C:\SignalVu-PC Files\sample')
#dest: 0 = client, 1 = .tiq, 2 = .siq, 3 = .siqd/.siqh
dest = c_int(1)
#dtype: 0 = single, 1 = int32, 2 = int16
dtype = c_int(2)
#SuffixCtl: 0 = none, 1 = YYYY.MM.DD.hh.mm.ss.msec, 3 = -xxxxx autoincrement
suffixCtl = c_int(3)
#streaming status boolean variable
complete = c_bool(False)
#write status boolean variable (always true if non-triggered acquisition)
writing = c_bool(False)
#time to wait between streaming loopCounts
waitTime = durationMsec.value/2/1e3
#bool used for streaming loopCount control
streaming = True
loopCount = 0

"""Search/Connect"""
ret = rsa300.Search(byref(deviceIDs), byref(deviceSerial), byref(numFound))
if ret != 0:
	print('Error in Search: ' + str(ret))
if numFound.value < 1:
	print('No instruments found.')
	sys.exit()
elif numFound.value == 1:
	print('One device found.')
	print('Device Serial Number: ' + deviceSerial.value)
else:
	print('2 or more instruments found.')
	#note: the API can only access one at a time

#connect to the RSA306
ret = rsa300.Connect(deviceIDs[0])
if ret != 0:
	print('Error in Connect: ' + str(ret))

"""Configure Settings"""
rsa300.Preset()
rsa300.SetCenterFreq(cf)
rsa300.SetReferenceLevel(refLevel)
rsa300.IQSTREAM_SetAcqBandwidth(bwHz_req)
rsa300.IQSTREAM_GetAcqParameters(byref(bwHz_act), byref(sRate))
rsa300.IQSTREAM_SetOutputConfiguration(dest, dtype)
rsa300.IQSTREAM_SetDiskFilenameBase(filenameBase)
rsa300.IQSTREAM_SetDiskFilenameSuffix(suffixCtl)
rsa300.IQSTREAM_SetDiskFileLength(durationMsec)
"""
Note: When the time limit specified by msec is reached, there is a de facto 
IQSTREAM_Stop(). Acquisition can be terminated early by explicitly sending 
IQSTREAM_Stop().
"""

"""
The order of these next two commands is of paramount importance. 
Most problems are caused/solved by switching the order of these commands.
Run() MUST BE SENT before IQSTREAM_Start()
"""
rsa300.Run()
rsa300.IQSTREAM_Start()

#Streaming control loop example, feel free to make your own
while streaming == True:
	time.sleep(waitTime)
	rsa300.IQSTREAM_GetDiskFileWriteStatus(byref(complete), byref(writing))
	#print('Complete? {}'.format(complete.value))
	#print('Writing? {}'.format(writing.value))
	if complete.value == True:
		streaming = False
	loopCount += 1
	
print('# of loops: {}'.format(loopCount))
print('File saved at {}'.format(filenameBase.value))
print('Disconnecting.')
rsa300.Disconnect()