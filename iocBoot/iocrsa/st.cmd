#!../../bin/windows-x64-static/rsa

## You may have to change rsa to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/rsa.dbd"
rsa_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadRecords "db/rsa.db", "P=DIG:, R=RSA:, DEVICE=0"

## Set this to see messages from mySub
#var mySubDebug 1

## Run this to trace the stages of iocInit
#traceIocInit

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
seq sncExample, "P=DIG:, R=RSA:"
