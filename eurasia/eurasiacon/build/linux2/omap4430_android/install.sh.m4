########################################################################### ###
##@Title         Target install script
##@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
##@License       Strictly Confidential.
### ###########################################################################


## Override the default install functions to stop creating links & version numbers
## on the shared libraries.
## Also overriding to stop chown reporting an error, as the install is now run
## in to the Android tree, and adb sync is used to get the driver files to the
## target system.

pushdivert(49)dnl
chown()
{
	`which chown` -f $*
}

cp()
{
	`which cp` -af $*
}

popdivert

pushdef([[INSTALL_SHARED_LIBRARY]], [[pushdivert(52)dnl
        ifelse($2,,install_file $1 SHLIB_DESTDIR/$1 "shared library" 0644 0:0,install_file $1 SHLIB_DESTDIR/$1 "shared library" 0644 0:0
        )
	popdivert()]])

pushdef([[INSTALL_SHARED_LIBRARY_DESTDIR]], [[pushdivert(52)dnl
        ifelse($2,,install_file $1 $3/$1 "shared library" 0644 0:0,install_file $1 $3/$1 "shared library" 0644 0:0
        )
popdivert()]])

define([[MOD_DESTDIR]], [[/system/modules]])
define([[MOD_ROOTDIR]], [[]])
define([[RC_DESTDIR]], [[]])
define([[SET_DDK_INSTALL_LOG_PATH]], [[pushdivert(49)dnl
# Where we record what we did so we can undo it.
#
DDK_INSTALL_LOG=[[/powervr_ddk_install]]LIB_SGXSUFFIX.log
popdivert]])

##	Set up variables.
##
TARGET_RUNS_DEPMOD_AT_BOOT(1)
DISPLAY_CONTROLLER(DISPLAY_KERNEL_MODULE)
PLATFORM_VERSION($RCSfile: install.sh.m4 $ $Revision: 1.5 $)

STANDARD_HEADER
INTERPRETER
SET_DDK_INSTALL_LOG_PATH

STANDARD_SCRIPTS
STANDARD_KERNEL_MODULES

STANDARD_LIBRARIES
STANDARD_EXECUTABLES
STANDARD_UNITTESTS

INSTALL_ANDROIDPOWERVRINI(powervr.ini.omap4)
INSTALL_ANDROIDPOWERVRINI2(powervr.omap4470.ini, powervr.omap4470.ini)
