TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

USR_CXXFLAGS_DEFAULT += -std=c++1y -L/opt/lib -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib
USR_LDFLAGS += -L/opt/lib -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib
USR_INCLUDES += -I/opt/include
#=============================
# Build the IOC support library

LIBRARY_IOC += rsaSupport

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord
# Install devXxxSoft.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Compile and add the code to the support library
rsaSupport_SRCS += xxxRecord.c
rsaSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application.
rsaSupport_SRCS += dbSubExample.c
rsaSupport_SRCS += rsaHello.c
rsaSupport_SRCS += initTrace.c

rsaSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC = rsa
# rsa.dbd will be created and installed
DBD += rsa.dbd

# rsa.dbd will be made up from these files:
rsa_DBD += base.dbd
rsa_DBD += xxxSupport.dbd
rsa_DBD += dbSubExample.dbd
rsa_DBD += rsaHello.dbd
rsa_DBD += initTrace.dbd

# rsa_registerRecordDeviceDriver.cpp derives from rsa.dbd
rsa_SRCS += rsa_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
rsa_SRCS_DEFAULT += rsaMain.cpp
rsa_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#rsa_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from the support library
rsa_LIBS += rsaSupport

# NOTE: To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file

rsa_SYS_LIBS += UnifiedAPI
rsa_SYS_LIBS += RSA300API 
rsa_SYS_LIBS += RSA500API 
rsa_SYS_LIBS += GPMeasDSP
rsa_SYS_LIBS += BaseDSPL
rsa_SYS_LIBS += dl 
rsa_SYS_LIBS += icui18n
rsa_SYS_LIBS += icuuc
rsa_SYS_LIBS += icudata
rsa_SYS_LIBS += ippch
rsa_SYS_LIBS += ippcv
rsa_SYS_LIBS += ippdc
rsa_SYS_LIBS += ippi
rsa_SYS_LIBS += ipps
rsa_SYS_LIBS += ippvm
rsa_SYS_LIBS += ippcore
rsa_SYS_LIBS += boost_date_time
rsa_SYS_LIBS += boost_chrono
rsa_SYS_LIBS += boost_system
rsa_SYS_LIBS += boost_filesystem
rsa_SYS_LIBS += rt
rsa_SYS_LIBS += boost_thread
rsa_SYS_LIBS += boost_regex
rsa_SYS_LIBS += pthread
rsa_SYS_LIBS += udev
rsa_SYS_LIBS += usb-1.0
rsa_SYS_LIBS += ippcc
rsa_SYS_LIBS += boost_log

ifneq ($(SNCSEQ),)
    # Build sncExample into rsaSupport
    sncExample_SNCFLAGS += +r
    rsa_DBD += sncExample.dbd
    rsaSupport_SRCS += sncExample.stt
    rsaSupport_LIBS += seq pv

    rsa_LIBS += seq pv


    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)

endif

# Finally link to the EPICS Base libraries
rsa_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE



