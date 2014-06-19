divert(-1)
dnl File		install.sh.m4
dnl Title		Target install script
dnl Author		Aidan Dixon
dnl
dnl Copyright	2004,2005 by Imagination Technologies Limited.
dnl				All rights reserved.  No part of this software, either
dnl				material or conceptual may be copied or distributed,
dnl				transmitted, transcribed, stored in a retrieval system
dnl				or translated into any human or computer language in any
dnl				form by any means, electronic, mechanical, manual or
dnl				other-wise, or disclosed to third parties without the
dnl				express written permission of Imagination Technologies
dnl				Limited, Unit 8, HomePark Industrial Estate,
dnl				King's Langley, Hertfordshire, WD4 8LZ, U.K.
dnl
dnl $Log: install.sh.m4 $

include(../scripts/common.m4)
VERSION($RCSfile: install.sh.m4 $ $Revision: 1.142 $)


###############################################################################
##
## Diversion discipline
##
## 49    variables
## 50    generic functions
## 51    function header of install_pvr function
## 52    body of install_pvr function
## 53    function trailer of install_pvr function
## 54    function header of install_X function
## 55    body of install_X function
## 56    function trailer of install_X function
## 80    script body
## 200   main case statement for decoding arguments.
##
###############################################################################

###############################################################################
##
## Diversion #49 - Variables that might be overridden
##
###############################################################################
pushdivert(49)dnl
# PVR Consumer services version number
#
[[PVRVERSION]]="PVRVERSION"

popdivert

define([[SET_DDK_INSTALL_LOG_PATH]], [[pushdivert(49)dnl
# Where we record what we did so we can undo it.
#
DDK_INSTALL_LOG=/etc/powervr_ddk_install.log
popdivert]])

###############################################################################
##
## Diversion #50 - Some generic functions
##
###############################################################################
pushdivert(50)dnl

[[# basic installation function
# $1=blurb
#
bail()
{
    echo "$1" >&2
    echo "" >&2
    echo "Installation failed" >&2
    exit 1
}

# basic installation function
# $1=fromfile, $2=destfilename, $3=blurb, $4=chmod-flags, $5=chown-flags
#
install_file()
{
	DESTFILE=${DISCIMAGE}$2
	DESTDIR=`dirname $DESTFILE`

	if [ ! -e $1 ]; then
		 	[ -n "$VERBOSE" ] && echo "skipping file $1 -> $2"
		 return
	fi
	
	# Destination directory - make sure it's there and writable
	#
	if [ -d "${DESTDIR}" ]; then
		if [ ! -w "${DESTDIR}" ]; then
			bail "${DESTDIR} is not writable."
		fi
	else
		$DOIT mkdir -p ${DESTDIR} || bail "Couldn't mkdir -p ${DESTDIR}"
		[ -n "$VERBOSE" ] && echo "Created directory `dirname $2`"
	fi

	# Delete the original so that permissions don't persist.
	#
	$DOIT rm -f $DESTFILE

	$DOIT cp -f $1 $DESTFILE || bail "Couldn't copy $1 to $DESTFILE"
	$DOIT chmod $4 ${DESTFILE}
	$DOIT chown $5 ${DESTFILE}

	echo "$3 `basename $1` -> $2"
	$DOIT echo "file $2" >>${DISCIMAGE}${DDK_INSTALL_LOG}
}

# Install a symbolic link
# $1=fromfile, $2=destfilename
#
install_link()
{
	DESTFILE=${DISCIMAGE}$2
	DESTDIR=`dirname $DESTFILE`

	if [ ! -e ${DESTDIR}/$1 ]; then
		 [ -n "$VERBOSE" ] && echo $DOIT "skipping link ${DESTDIR}/$1"
		 return
	fi

	# Destination directory - make sure it's there and writable
	#
	if [ -d "${DESTDIR}" ]; then
		if [ ! -w "${DESTDIR}" ]; then
			bail "${DESTDIR} is not writable."
		fi
	else
		$DOIT mkdir -p ${DESTDIR} || bail "Couldn't mkdir -p ${DESTDIR}"
		[ -n "$VERBOSE" ] && echo "Created directory `dirname $2`"
	fi

	# Delete the original so that permissions don't persist.
	#
	$DOIT rm -f $DESTFILE

	$DOIT ln -s $1 $DESTFILE || bail "Couldn't link $1 to $DESTFILE"
	$DOIT echo "link $2" >>${DISCIMAGE}${DDK_INSTALL_LOG}
	[ -n "$VERBOSE" ] && echo " linked `basename $1` -> $2"
}

# Tree-based installation function
# $1 = fromdir $2=destdir $3=blurb
#
install_tree()
{
	# Make the destination directory if it's not there
	#
	if [ ! -d ${DISCIMAGE}$2 ]; then
		$DOIT mkdir -p ${DISCIMAGE}$2 || bail "Couldn't mkdir -p ${DISCIMAGE}$2"
	fi
	if [ "$DONTDOIT" ]; then
		echo "### tar -C $1 -cf - . | tar -C ${DISCIMAGE}$2 -x${VERBOSE}f -" 
	else
		tar -C $1 -cf - . | tar -C ${DISCIMAGE}$2 -x${VERBOSE}f -
	fi
	if [ $? = 0 ]; then
		echo "Installed $3 in ${DISCIMAGE}$2"
		$DOIT echo "tree $2" >>${DISCIMAGE}${DDK_INSTALL_LOG}
	else
		echo "Failed copying $3 from $1 to ${DISCIMAGE}$2"
	fi
}

# Uninstall something.
#
uninstall()
{
	if [ ! -f ${DISCIMAGE}${DDK_INSTALL_LOG} ]; then
		echo "Nothing to un-install."
		return;
	fi

	BAD=0
	VERSION=""
	while read type data; do
		case $type in
		version)	# do nothing
			echo "Uninstalling existing version $data"
			VERSION="$data"
			;;
		link|file) 
			if [ -z "$VERSION" ]; then
				BAD=1;
				echo "No version record at head of ${DISCIMAGE}${DDK_INSTALL_LOG}"
			elif ! $DOIT rm -f ${DISCIMAGE}${data}; then
				BAD=1;
			else
				[ -n "$VERBOSE" ] && echo "Deleted $type $data"
			fi
			;;
		tree)		# so far, do nothing
			;;
		esac
	done < ${DISCIMAGE}${DDK_INSTALL_LOG};

	if [ $BAD = 0 ]; then
		echo "Uninstallation completed."
		$DOIT rm -f ${DISCIMAGE}${DDK_INSTALL_LOG}
	else
		echo "Uninstallation failed!!!"
	fi
}

# Help on how to invoke
#
usage()
{
	echo "usage: $0 [options...]"
	echo ""
	echo "Options: -v            verbose mode"
	echo "         -n            dry-run mode"
	echo "         -u            uninstall-only mode"
	echo "         --no-pvr      don't install PowerVR driver components"
	echo "         --no-x        don't install X window system"
	echo "         --no-display  don't install integrated PowerVR display module"
	echo "         --no-bcdevice don't install buffer class device module"
	echo "         --root path   use path as the root of the install file system"
	exit 1
}

]]popdivert

###############################################################################
##
## Diversion 51 - the start of the install_pvr() function
##
###############################################################################
pushdivert(51)dnl
install_pvr()
{
	$DOIT echo "version PVRVERSION" >${DISCIMAGE}${DDK_INSTALL_LOG}
popdivert

pushdivert(53)dnl
}

popdivert

pushdivert(80)[[
# Work out if there are any special instructions.
#
while [ "$1" ]; do
	case "$1" in
	-v|--verbose)
		VERBOSE=v;
		;;
	-r|--root)
		DISCIMAGE=$2;
		shift;
		;;
	-u|--uninstall)
		UNINSTALL=y
		;;
	-n)	DOIT=echo
		;;
	--no-pvr)
		NO_PVR=y
		;;
	--no-x)
		NO_X=y
		;;
	--no-display)
		NO_DISPLAYMOD=y
		;;
	--no-bcdevice)
		NO_BCDEVICE=y
		;;
	-h | --help | *)	
		usage
		;;
	esac
	shift
done

# Find out where we are?  On the target?  On the host?
#
case `uname -m` in
arm*)	host=0;
		from=target;
		DISCIMAGE=/;
		;;
sh*)	host=0;
		from=target;
		DISCIMAGE=/;
		;;
i?86*)	host=1;
		from=host;
		if [ -z "$DISCIMAGE" ]; then	
			echo "DISCIMAGE must be set for installation to be possible." >&2
			exit 1
		fi
		;;
x86_64*)	host=1;
		from=host;
		if [ -z "$DISCIMAGE" ]; then	
			echo "DISCIMAGE must be set for installation to be possible." >&2
			exit 1
		fi
		;;
*)		echo "Don't know host to perform on machine type `uname -m`" >&2;
		exit 1
		;;
esac

if [ ! -d "$DISCIMAGE" ]; then
	echo "$0: $DISCIMAGE does not exist." >&2
	exit 1
fi

echo
echo "Installing PowerVR Consumer/Embedded DDK ']]PVRVERSION[[' on $from"
echo
echo "Target SGX: $SGXCORE $SGX_CORE_REV"
echo
echo "File system installation root is $DISCIMAGE"
echo

]]popdivert


###############################################################################
##
## Main Diversion - call the install_pvr function unless NO_PVR is non-null
##
###############################################################################
pushmain()dnl
# Uninstall whatever's there already.
#
uninstall
[ -n "$UNINSTALL" ] && exit

#  Now start installing things we want.
#
[ -z "$NO_PVR" ] && install_pvr

#  We need a gralloc.omap4430.so in daily builds
#  same gralloc lib as 4460 so just copy
if [ "$SGXCORE" = 540 ]; then
	if [ "$SGX_CORE_REV" = 120 ]; then
		cp ${DISCIMAGE}/system/vendor/lib/hw/gralloc.omap4460.so ${DISCIMAGE}/system/vendor/lib/hw/gralloc.omap4430.so
	fi
fi
popdivert


###############################################################################
##
## Diversion 200 -
## Divert some text to appear when the final input stream is closed.
##
###############################################################################
pushdivert(200)dnl

# All done...
#
echo 
echo "Installation complete!"
if [ "$host" = 0 ]; then
   echo "You may now reboot your target."
fi
echo
popdivert


###############################################################################
##
## INSTALL_KERNEL_MODULE -
## install a kernel loadable module in the correct place in the file system,
## updating the module dependencies file(s) if necessary.
##
## Parameters:
## $1 - name of kernel module file without leading path components
## $2 - name of any other kernel module upon which $1 is dependent.
##
###############################################################################
define([[INSTALL_KERNEL_MODULE]], [[pushdivert(52)dnl
	install_file $1 MOD_DESTDIR/$1 "kernel module" 0644 0:0
ifelse(TARGET_RUNS_DEPMOD_AT_BOOT,TRUE,,TARGET_HAS_DEPMOD,FALSE,[[dnl
FIXUP_MODULES_DEP($1, $2)dnl
]],[[dnl
	if [ "$host" = 1 ]; then
		FIXUP_MODULES_DEP($1, $2)dnl
	fi
]])dnl
popdivert()]])


###############################################################################
##
## FIXUP_MODULES_DEP -
## Fix up the modules dependency file.
##
## Parameters:
## $1 - name of kernel module file without leading path components
## $2 - name of any other kernel module upon which $1 is dependent.
##
###############################################################################
define([[FIXUP_MODULES_DEP]], [[pushdivert(52)dnl
	grep -v -e "extra/$1" MODULES_DEP >MODULES_TMP
ifelse([[$2]], [[]],dnl
	echo "MOD_DESTDIR/$1:" >>MODULES_TMP,
	echo "MOD_DESTDIR/$1: MOD_DESTDIR/$2" >>MODULES_TMP)
	cp MODULES_TMP MODULES_DEP
popdivert()]])


###############################################################################
##
## INSTALL_SHARED_LIBRARY -
## Install a shared library (in /usr/lib) with the correct version number and
## links.
##
## Parameters:
## $1 - name of shared library to install
## $2 - optional version suffix.
##
###############################################################################
define([[INSTALL_SHARED_LIBRARY]], [[pushdivert(52)dnl
	ifelse($2,,install_file $1 SHLIB_DESTDIR/$1 "shared library" 0644 0:0,install_file $1 SHLIB_DESTDIR/$1.$2 "shared library" 0644 0:0
	install_link $1.$2 SHLIB_DESTDIR/$1)
popdivert()]])


###############################################################################
##
## INSTALL_SHARED_LIBRARY_DESTDIR -
## Install a shared library in a non-standard library directory, with the
## correct version number and links.
##
## Parameters:
## $1 - name of shared library to install
## $2 - optional version suffix
## $3 - name of a subdirectory to install to
##
###############################################################################
define([[INSTALL_SHARED_LIBRARY_DESTDIR]], [[pushdivert(52)dnl
	ifelse($2,,install_file $1 $3/$1 "shared library" 0644 0:0,install_file $1 $3/$1.$2 "shared library" 0644 0:0
	install_link $1.$2 $3/$1)
popdivert()]])


###############################################################################
##
## INSTALL_BINARY -
## Install a binary file.  These always go in /usr/local/bin presently.
##
## Parameters:
## $1 - name of file in local directory which is to be copied to /usr/local/bin
##
###############################################################################
define([[INSTALL_BINARY]], [[pushdivert(52)dnl
	install_file $1 DEMO_DESTDIR/$1 "binary" 0755 0:0
popdivert()]])

###############################################################################
##
## INSTALL_ANDROIDPOWERVRINI -
## Install a file in /system/etc/power.ini
##
## Parameters:
## $1 - name of file in local directory which is to be copied
##
###############################################################################
define([[INSTALL_ANDROIDPOWERVRINI]], [[pushdivert(52)dnl
	install_file $1 /system/etc/powervr.ini "binary" 0444 0:0
popdivert()]])

###############################################################################
##
## INSTALL_ANDROIDPOWERVRINI2 -
## Install a file in /system/etc/
##
## Parameters:
## $1 - name of file in local directory which is to be copied
## $2 - name of file in /system/etc/ on the target device
##
###############################################################################
define([[INSTALL_ANDROIDPOWERVRINI2]], [[pushdivert(52)dnl
	install_file $1 /system/etc/$2 "binary" 0444 0:0
popdivert()]])


###############################################################################
##
## INSTALL_APK -
## Install an .apk file. These are only installed on Android.
##
## Parameters:
## $1 - name of file in local directory which is to be copied to /data/app
##
###############################################################################
define([[INSTALL_APK]], [[pushdivert(52)dnl
	install_file $1.apk APK_DESTDIR/$1.apk "binary" 0644 1000:1000
popdivert()]])

###############################################################################
##
## INSTALL_SHADER -
## Install a binary file.  These always go in /usr/local/bin presently.
##
## Parameters:
## $1 - name of file in local directory which is to be copied to /usr/local/bin
##
###############################################################################
define([[INSTALL_SHADER]], [[pushdivert(52)dnl
	install_file $1 DEMO_DESTDIR/$1 "shader" 0644 0:0
popdivert()]])


###############################################################################
##
## INSTALL_INITRC -
## Install a run-time configuration file.  This goes in /etc/init.d unless
## RC_DESTDIR has been updated.
##
## No parameters.
##
###############################################################################
define([[INSTALL_INITRC]], [[pushdivert(52)dnl
	install_file $1 RC_DESTDIR/$1 "boot script" 0755 0:0
popdivert()]])

###############################################################################
##
## INSTALL_HEADER -
## Install a header. These go in HEADER_DESTDIR, which is normally
## /usr/include.
##
## Parameters:
## $1 - name of header to install
## $2 - subdirectory of /usr/include to install it to
##
###############################################################################
define([[INSTALL_HEADER]], [[pushdivert(52)dnl
	install_file $1 HEADER_DESTDIR/$2/$1 "header" 0644 0:0
popdivert()]])

###############################################################################
##
## Some defines for general expansion
##
## No parameters.
##
###############################################################################
define([[SHARED_LIBRARY]],			[[INSTALL_SHARED_LIBRARY($1,$2)]])
define([[SHARED_LIBRARY_DESTDIR]],	[[INSTALL_SHARED_LIBRARY_DESTDIR($1, $2, $3)]])
define([[APK]],						[[INSTALL_APK($1)]])
define([[UNITTEST]],				[[INSTALL_BINARY($1)]])
define([[EXECUTABLE]],				[[INSTALL_BINARY($1)]])
define([[GLES1UNITTEST]],			[[INSTALL_BINARY($1)]])
define([[GLES2UNITTEST]],			[[INSTALL_BINARY($1)]])
define([[GLES2UNITTEST_SHADER]],	[[INSTALL_SHADER($1)]])
define([[GLUNITTEST_SHADER]],		[[INSTALL_SHADER($1)]])
define([[HEADER]],					[[INSTALL_HEADER($1, $2)]])
define([[KERNEL_MODULE]],			[[INSTALL_KERNEL_MODULE($1, $2)]])
# FIXME:Services 4.0:
# These should be installed somewhere else
define([[INITBINARY]],				[[INSTALL_BINARY($1)]])


###############################################################################
##
## STANDARD_SCRIPTS -
## Install all standard script parts of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_SCRIPTS]], [[pushdivert(52)dnl
	# Install the standard scripts
	#
INSTALL_INITRC(rc.pvr)dnl

popdivert()]])


###############################################################################
##
## STANDARD_LIBRARIES
## Install all standard parts of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_LIBRARIES]], [[pushdivert(52)dnl
	# Install the standard libraries
	#
ifelse(SUPPORT_OPENGLES1_V1_ONLY,0,
ifelse(SUPPORT_OPENGLES1,1, [[SHARED_LIBRARY_DESTDIR([[OGLES1_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]]))dnl

ifelse(SUPPORT_OPENGLES1_V1,1,[[SHARED_LIBRARY_DESTDIR([[OGLES1_V1_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl
ifelse(SUPPORT_OPENGLES1_V1_ONLY,1,[[SHARED_LIBRARY_DESTDIR([[OGLES1_V1_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl

ifelse(FFGEN_UNIFLEX,1,[[SHARED_LIBRARY([[[[libusc]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_OPENGLES2,1,[[SHARED_LIBRARY_DESTDIR([[OGLES2_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl

ifelse(SUPPORT_OPENGLES2,1,
ifelse(SUPPORT_SOURCE_SHADER,1,[[SHARED_LIBRARY([[[[libglslcompiler]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]]))dnl

ifelse(SUPPORT_OPENVG,1,[[SHARED_LIBRARY([[[[libOpenVG]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENVG,1,[[SHARED_LIBRARY([[[[libOpenVGU]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENVGX,1,[[SHARED_LIBRARY([[[[libOpenVG]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENVGX,1,[[SHARED_LIBRARY([[[[libOpenVGU]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENVGX,1,[[SHARED_LIBRARY([[[[libvgxfw]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_OPENCL,1,[[SHARED_LIBRARY([[[[libPVROCL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENCL,1,[[SHARED_LIBRARY([[[[liboclcompiler]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

SHARED_LIBRARY([[[[libIMGegl]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])dnl
ifelse(SUPPORT_LIBEGL,1,[[SHARED_LIBRARY_DESTDIR([[EGL_MODULE]],[[SOLIB_VERSION]],[[EGL_DESTDIR]])]])dnl
ifelse(SUPPORT_LIBPVR2D,1,[[SHARED_LIBRARY([[[[libpvr2d]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]],
						  [[SHARED_LIBRARY([[[[libnullws]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_NULL_PVR2D_BLIT,1,[[SHARED_LIBRARY([[[[libpvrPVR2D_BLITWSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_PVR2D_FLIP,1,[[SHARED_LIBRARY([[[[libpvrPVR2D_FLIPWSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_PVR2D_FRONT,1,[[SHARED_LIBRARY([[[[libpvrPVR2D_FRONTWSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_NULL_PVR2D_LINUXFB,1,[[SHARED_LIBRARY([[[[libpvrPVR2D_LINUXFBWSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_EWS,1,[[SHARED_LIBRARY([[[[libpvrEWS_WSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_DRI_DRM,1,[[SHARED_LIBRARY([[[[libpvrPVR2D_DRIWSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

SHARED_LIBRARY([[[[libsrv_um]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])dnl
ifelse(SUPPORT_SRVINIT,1,[[SHARED_LIBRARY([[[[libsrv_init]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_SGX_HWPERF,1,[[SHARED_LIBRARY([[[[libPVRScopeServices]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_IMGTCL,1,[[SHARED_LIBRARY([[[[libimgtcl]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_OPENGL,1,[[SHARED_LIBRARY([[[[libPVROGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_OPENGL,1,[[SHARED_LIBRARY([[[[libPVROGL_MESA]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl
ifelse(SUPPORT_XORG,1,[[SHARED_LIBRARY([[[[libegl4ogl]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[GRALLOC_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY_DESTDIR([[HWCOMPOSER_MODULE]],[[SOLIB_VERSION]],[[HAL_DESTDIR]])]])dnl
ifelse(SUPPORT_GRAPHICS_HAL,1,
	[[SHARED_LIBRARY([[[[libpvrANDROID_WSEGL]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

ifelse(SUPPORT_EWS,1,[[SHARED_LIBRARY([[[[libews]]LIB_SGXSUFFIX.LIB_SUFFIX]],[[SOLIB_VERSION]])]])dnl

popdivert()]])



###############################################################################
##
## STANDARD_EXECUTABLES
## Install all standard executables of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_EXECUTABLES]], [[pushdivert(52)dnl
	# Install the standard executables
	#

#install pvrsrvinit wrapper
ifelse(SUPPORT_SRVINIT,1,[[INITBINARY(pvrsrvinit)]])dnl
#install arch dependent pvrsrvinit
ifelse(SUPPORT_SRVINIT,1,[[INITBINARY([[pvrsrvinit]]LIB_SGXSUFFIX)]])dnl
ifelse(SUPPORT_SRVINIT,1,[[INITBINARY(pvrsrvctl)]])dnl
ifelse(SUPPORT_SRVINIT,1,[[UNITTEST([[sgx_init_test]]LIB_SGXSUFFIX)]])dnl

ifelse(PDUMP,1,[[UNITTEST(pdump)]])dnl

ifelse(SUPPORT_EWS,1,[[EXECUTABLE(ews_server)]])dnl
ifelse(SUPPORT_EWS,1,
	ifelse(SUPPORT_OPENGLES2,1,[[EXECUTABLE(ews_server_es2)]]))dnl
ifelse(SUPPORT_EWS,1,
	ifelse(SUPPORT_LUA,1,[[EXECUTABLE(ews_wm)]]))dnl

popdivert()]])
 
###############################################################################
##
## STANDARD_UNITTESTS
## Install all standard unitests built as part of a DDK distribution.
##
## No parameters.
##
###############################################################################
define([[STANDARD_UNITTESTS]], [[pushdivert(52)dnl
	# Install the standard unittests
	#

ifelse(SUPPORT_ANDROID_PLATFORM,1,[[
	if [ ! -d ${DISCIMAGE}/data ]; then
		mkdir ${DISCIMAGE}/data
		chown 1000:1000 ${DISCIMAGE}/data
		chmod 0771 ${DISCIMAGE}/data
	fi

	if [ ! -d ${DISCIMAGE}/data/app ]; then
		mkdir ${DISCIMAGE}/data/app
		chown 1000:1000 ${DISCIMAGE}/data/app
		chmod 0771 ${DISCIMAGE}/data/app
	fi
]])dnl

UNITTEST([[services_test]]LIB_SGXSUFFIX)dnl
UNITTEST([[sgx_blit_test]]LIB_SGXSUFFIX)dnl
UNITTEST([[sgx_clipblit_test]]LIB_SGXSUFFIX)dnl
UNITTEST([[sgx_flip_test]]LIB_SGXSUFFIX)dnl
UNITTEST([[sgx_render_flip_test]]LIB_SGXSUFFIX)dnl
UNITTEST([[pvr2d_test]]LIB_SGXSUFFIX)dnl

ifelse(SUPPORT_ANDROID_PLATFORM,1,[[
UNITTEST(testwrap)dnl
ifelse(SUPPORT_OPENGLES1,1,[[
APK(gles1test1)dnl
APK(gles1_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
APK(gles2test1)dnl
APK(gles2_texture_stream)dnl
]])dnl
APK(eglinfo)dnl
APK(launcher)dnl
]],[[
ifelse(SUPPORT_OPENGLES1,1,[[
GLES1UNITTEST(gles1test1)dnl
GLES1UNITTEST(gles1_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
GLES2UNITTEST(gles2test1)dnl
GLES2UNITTEST_SHADER(glsltest1_vertshader.txt)dnl
GLES2UNITTEST_SHADER(glsltest1_fragshaderA.txt)dnl
GLES2UNITTEST_SHADER(glsltest1_fragshaderB.txt)dnl
GLES2UNITTEST(gles2_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENVG,1,[[
UNITTEST(ovg_unit_test)dnl
]])dnl
ifelse(SUPPORT_OPENGL,1,[[
UNITTEST(gltest1)dnl
]])dnl
UNITTEST(eglinfo)dnl
ifelse(SUPPORT_OPENCL,1,[[
UNITTEST(ocl_unit_test)dnl
]])dnl
]])dnl

ifelse(SUPPORT_ANDROID_PLATFORM,1,[[
UNITTEST(hal_client_test)dnl
UNITTEST(hal_server_test)dnl
UNITTEST(framebuffer_test)dnl
UNITTEST(texture_benchmark)dnl
]])dnl

ifelse(SUPPORT_XUNITTESTS,1,[[
UNITTEST(xtest)dnl
ifelse(SUPPORT_OPENGLES1,1,[[
GLES1UNITTEST(xgles1test1)dnl
GLES1UNITTEST(xmultiegltest)dnl
GLES1UNITTEST(xgles1_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
GLES2UNITTEST(xgles2test1)dnl
GLES2UNITTEST(xgles2_texture_stream)dnl
]])dnl
ifelse(SUPPORT_OPENVG,1,[[
UNITTEST(xovg_unit_test)dnl
]])dnl
ifelse(SUPPORT_OPENGL,1,[[
UNITTEST(xgltest1)dnl
UNITTEST(xgltest2)dnl
GLUNITTEST_SHADER(gltest2_vertshader.txt)dnl
GLUNITTEST_SHADER(gltest2_fragshaderA.txt)dnl
GLUNITTEST_SHADER(gltest2_fragshaderB.txt)dnl
]])dnl
]])dnl

ifelse(SUPPORT_EWS,1,[[
ifelse(SUPPORT_OPENGLES1,1,[[
GLES1UNITTEST(ews_test_gles1)dnl
]])dnl
ifelse(SUPPORT_OPENGLES2,1,[[
GLES2UNITTEST(ews_test_gles2)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_main.vert)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_main.frag)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_pp.vert)dnl
GLES2UNITTEST_SHADER(ews_test_gles2_pp.frag)dnl
]])dnl
UNITTEST(ews_test_swrender)dnl
]])dnl

popdivert()]])


define([[NON_SHIPPING_TESTS]], [[pushdivert(52)dnl
	# Install internal unittests
	#
	mkdir -p $DISCIMAGE/usr/local/bin/gles2tests
	if test -d $EURASIAROOT/eurasiacon/opengles2/tests; then
		for test in $(find $EURASIAROOT/eurasiacon/opengles2/tests -regex '.*makefile.linux' -exec cat {} \;|egrep 'MODULE\W*='|dos2unix|cut -d '=' -f2)
		do
			if test -f $test; then
				install_file $test /usr/local/bin/gles2tests/$test "binary" 0755 0:0
			fi
		done

		echo "Shaders -> /usr/local/bin/glestests"
		find $EURASIAROOT/eurasiacon/opengles2/tests -regex '.*\.vert' -exec cp -f {} $DISCIMAGE/usr/local/bin/gles2tests \;
		find $EURASIAROOT/eurasiacon/opengles2/tests -regex '.*\.frag' -exec cp -f {} $DISCIMAGE/usr/local/bin/gles2tests \;
	fi
	
	mkdir -p $DISCIMAGE/usr/local/bin/gles1tests
	if test -d $EURASIAROOT/eurasiacon/opengles1/tests; then

		for test in $(find $EURASIAROOT/eurasiacon/opengles1/tests -regex '.*makefile.linux' -exec cat {} \;|egrep -r 'MODULE\W*='|dos2unix|cut -d '=' -f2)
		do
			if test -f $test; then
				install_file $test /usr/local/bin/gles1tests/$test "binary" 0755 0:0
			fi
		done
	fi

popdivert()]])


###############################################################################
##
## STANDARD_KERNEL_MODULES
## Install all regular kernel modules - the PVR serviecs module (includes the
## SGX driver) and the display driver, if one is defined.  We also do a run of
## depmod(8) if the mode requires.
##
## No parameters.
##
###############################################################################
define([[STANDARD_KERNEL_MODULES]], [[pushdivert(52)dnl
	# Check the kernel module directory is there
	#
	if [ ! -d "${DISCIMAGE}MOD_ROOTDIR" ]; then
		echo ""
		echo "Can't find MOD_ROOTDIR on file system installation root"
		echo -n "There is no kernel module area setup yet. "
		if [ "$from" = target ]; then
			echo "On your build machine you should invoke:"
			echo
			echo " $ cd \$KERNELDIR"
			echo " $ make INSTALL_MOD_PATH=\$DISCIMAGE modules_install"
		else
			echo "You should invoke:"
			echo
			echo " $ cd $KERNELDIR"
			echo " $ make INSTALL_MOD_PATH=$DISCIMAGE modules_install"
		fi
		echo
		exit 1;
	fi

	# Install the standard kernel modules
	# Touch some files that might not exist so that busybox/modprobe don't complain
	#
	ifelse(TARGET_HAS_DEPMOD,FALSE,[[cp MODULES_DEP MODULES_DEP[[.old]]
]])dnl

ifelse(SUPPORT_DRI_DRM_NOT_PCI,1,[[dnl
KERNEL_MODULE(DRM_MODNAME.KM_SUFFIX)dnl
KERNEL_MODULE(PVRSRV_MODNAME.KM_SUFFIX, [[DRM_MODNAME.KM_SUFFIX]])dnl
]],[[dnl
KERNEL_MODULE(PVRSRV_MODNAME.KM_SUFFIX)dnl
ifelse(PDUMP,1,[[KERNEL_MODULE([[dbgdrv.KM_SUFFIX]])]])dnl
]])dnl

ifdef([[_DISPLAY_CONTROLLER_]],
	if [ -z "$NO_DISPLAYMOD" ]; then
	KERNEL_MODULE(_DISPLAY_CONTROLLER_[[.]]KM_SUFFIX, [[PVRSRV_MODNAME.KM_SUFFIX]])
fi)

ifdef([[_BUFFER_CLASS_DEVICE_]],
	if [ -z "$NO_BCDEVICE" ]; then
	KERNEL_MODULE(_BUFFER_CLASS_DEVICE_[[.]]KM_SUFFIX, [[PVRSRV_MODNAME.KM_SUFFIX]])
fi)

ifelse(SUPPORT_ANDROID_PLATFORM,1,,[[
	$DOIT touch LIBMODPROBE_CONF
	$DOIT touch MODULES_CONF
	$DOIT rm -f MODULES_TMP
]])

popdivert()]])

###############################################################################
##
## STANDARD_HEADERS
## Install headers
##
## No parameters.
##
###############################################################################
define([[STANDARD_HEADERS]], [[pushdivert(52)dnl
ifelse(SUPPORT_EWS,1,[[dnl
HEADER(ews.h, ews)dnl
HEADER(ews_types.h, ews)dnl
]])dnl
popdivert()]])

###############################################################################
##
## XORG_ALL -
##
## No parameters.
##
###############################################################################
define([[XORG_ALL]], [[pushdivert(54)dnl
pushdivert(54)dnl
install_X()
{
popdivert()dnl
pushdivert(55)dnl
ifelse(SUPPORT_XORG,1,[[dnl
	[ -d usr ] &&
		install_tree usr XORG_DEST/usr "X.Org X server and libraries"
	[ -f pvr_drv.so ] &&
		install_file pvr_drv.so PVR_DDX_DESTDIR/pvr_drv.so "X.Org PVR DDX module" 0755 0:0
 ifelse(SUPPORT_XORG_CONF,1,[[dnl
	[ -f xorg.conf ] &&
		install_file xorg.conf XORG_DIR/etc/xorg.conf "X.Org configuration file" 0644 0:0
 ]])dnl
 ifelse(SUPPORT_OPENGL,1,[[dnl
	[ -f pvr_dri.so ] &&
		install_file pvr_dri.so PVR_DRI_DESTDIR/pvr_dri.so "X.Org PVR DRI module" 0755 0:0
 ]])dnl
]])dnl
popdivert()dnl
pushdivert(56)dnl
}
popdivert()dnl
pushmain()dnl
[ -z "$NO_X" ] && install_X
popdivert()]])
