"""
Companion program to "How to Use the RSA306 API in Python"
Author: Morgan Allison
Date Created: 5/5/15
Date edited: 11/18/15
Windows 7 64-bit
Python 2.7.9 64-bit (Anaconda 3.7.0)
NumPy 1.8.1, MatPlotLib 1.3.1
To get Anaconda: http://continuum.io/downloads
Anaconda includes NumPy and MatPlotLib
"""

from ctypes import *
import numpy as np
import matplotlib.pyplot as plt

"""
################################################################
C:\Tektronix\RSA306 API\lib\x64 needs to be added to the 
PATH system environment variable
################################################################
"""
os.chdir("C:\\Tektronix\\RSA306 API\\lib\\x64")
rsa300 = WinDLL("RSA300API.dll")

class IQHeader(Structure):
   _fields_ = [('acqDataStatus', c_uint16),
   ('acquisitionTimestamp', c_uint64),
   ('frameID', c_uint32), ('trigger1Index', c_uint16),
   ('trigger2Index', c_uint16), ('timeSyncIndex', c_uint16)]

#initialize/assign variables
longArray = c_long*10
deviceIDs = longArray()
deviceSerial = c_wchar_p('')
numFound = c_int(0)
serialNum = c_char_p('')
nomenclature = c_char_p('')
header = IQHeader()

refLevel = c_double(0)
cf = c_double(1e9)
iqBandwidth = c_double(40e6)
recordLength = c_long(1024)
mode = c_int(0)
level = c_double(-10)
iqSampleRate = c_double(0)
runMode = c_int(0)
timeoutMsec = c_int(1000)
ready = c_bool(False)

#search for RSA306 and connect to the first one found
rsa300.Search(byref(deviceIDs), byref(deviceSerial), byref(numFound))
if numFound.value == 1:
   rsa300.Connect(deviceIDs[0])
else:
   print('Unexpected number of instruments found.')
   exit()

#initialize
rsa300.GetDeviceSerialNumber(serialNum)
print('Serial Number: ' + str(serialNum.value))
rsa300.GetDeviceNomenclature(nomenclature)
print('Device Nomenclature: ' + str(nomenclature.value))

#configure instrument
rsa300.Preset()
rsa300.SetReferenceLevel(refLevel)
rsa300.SetCenterFreq(cf)
rsa300.SetIQBandwidth(iqBandwidth)
rsa300.SetIQRecordLength(recordLength)
rsa300.SetTriggerMode(mode)
rsa300.SetIFPowerTriggerLevel(level)

#begin acquisition
rsa300.Run()

#get relevant settings values
rsa300.GetReferenceLevel(byref(refLevel))
rsa300.GetCenterFreq(byref(cf))
rsa300.GetIQBandwidth(byref(iqBandwidth))
rsa300.GetIQRecordLength(byref(recordLength))
rsa300.GetTriggerMode(byref(mode))
rsa300.GetIFPowerTriggerLevel(byref(level))
rsa300.GetRunState(byref(runMode))
rsa300.GetIQSampleRate(byref(iqSampleRate))

#for kicks and giggles
print('Run Mode:' + str(runMode.value))
print('Reference level: ' + str(refLevel.value) + 'dBm')
print('Center frequency: ' + str(cf.value) + 'Hz')
print('Bandwidth: ' + str(iqBandwidth.value) + 'Hz')
print('Record length: ' + str(recordLength.value))
print('Trigger mode: ' + str(mode.value))
print('Trigger level: ' + str(level.value) + 'dBm')
print('Sample rate: ' + str(iqSampleRate.value) + 'Samples/sec')

#check for data ready
while ready.value == False:
   ret = rsa300.WaitForIQDataReady(timeoutMsec, byref(ready))
print('IQ Data is Ready')

#as a bonus, get the IQ header even though it's not used
ret = rsa300.GetIQHeader(byref(header))
if ret != 0:
   print('Error in GetIQHeader: ' + str(ret))
print('Got IQ Header')

#initialize data transfer variables
iqArray =  c_float*recordLength.value
iData = iqArray()
qData = iqArray()
startIndex = c_int(0)

#query I and Q data
rsa300.GetIQDataDeinterleaved(byref(iData), byref(qData), startIndex, recordLength)
print('Got IQ data')

#convert ctypes array to numpy array for ease of use
I = np.ctypeslib.as_array(iData)
Q = np.ctypeslib.as_array(qData)

#create time array for plotting
time = np.linspace(0,recordLength.value/iqSampleRate.value,recordLength.value)

plt.figure(1)
plt.subplot(2,1,1)
plt.title('I and Q vs Time')
plt.plot(time,I)
plt.ylabel('I (V)')
plt.subplot(2,1,2)
plt.plot(time,Q)
plt.ylabel('Q (V)')
plt.xlabel('Time (sec)')
plt.show()

print('Disconnecting.')
ret = rsa300.Disconnect()