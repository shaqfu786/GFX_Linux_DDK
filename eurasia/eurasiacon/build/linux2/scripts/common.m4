divert(-1)
# File		$Id: common.m4 1.10 2011/06/30 10:45:31 rufus.hamade Exp $
# Title		Target common m4 macros.
# Author	Aidan Dixon
#
# Copyright	2004,2005 by Imagination Technologies Limited.
#			All rights reserved.  No part of this software, either
#			material or conceptual may be copied or distributed,
#			transmitted, transcribed, stored in a retrieval system
#			or translated into any human or computer language in any
#			form by any means, electronic, mechanical, manual or
#			other-wise, or disclosed to third parties without the
#			express written permission of Imagination Technologies
#			Limited, Unit 8, HomePark Industrial Estate,
#			King's Langley, Hertfordshire, WD4 8LZ, U.K.
#
# $Log: common.m4 $

# Define how we quote things.
#
changequote([[, ]])

# Define how we comment things.  We don't define a comment end delimiter here so
# the end-of-line serves that function.  Why change the comment starter?  We do
# this so we that we can have macro replacement in text intended as comments in
# the output stream.  The default starter is '#'; "dnl" is also usuable.  Note
# that the way we've set up the diversion discipline means that only comments
# inside pushdivert...popdivert pairs will be copied anyway.
#
changecom([[##]])

#! Some macros to handle diversions conviently.
#!
define([[_current_divert]],	[[-1]])
define([[pushdivert]],		[[pushdef([[_current_divert]],$1)divert($1)]])
define([[popdivert]],		[[popdef([[_current_divert]])divert(_current_divert)]])
define([[pushmain]],		[[pushdivert(100)]])

## Diversion discipline
##
## 0		standard-output stream - DO NOT USE
## 1		shell script interpreter directive
## 2		copyright
## 3		version trace
## 4-49	for use only by common.m4
## 50-99	for use by other macro bodies (rc.pvr.m4, install.sh.m4 etc.)
## 100	main output body of script.
## 200	macro bodies's trailers
## 201	common.m4's trailers
##

###############################################################################
##
##	Diversion #1 - Copy in the interpreter line
##
###############################################################################
define([[INTERPRETER]], [[pushdivert(1)dnl
#!/bin/sh

popdivert]])


###############################################################################
##
##	Diversion #2 - Copy in the copyright text.
##
###############################################################################
pushdivert(2)dnl
# PowerVR SGX DDK for Embedded Linux - installation script
#
# Copyright	Imagination Technologies Limited.
#		All rights reserved.  No part of this software, either
#		material or conceptual may be copied or distributed,
#		transmitted, transcribed, stored in a retrieval system
#		or translated into any human or computer language in any
#		form by any means, electronic, mechanical, manual or
#		other-wise, or disclosed to third parties without the
#		express written permission of Imagination Technologies
#		Limited, Unit 8, HomePark Industrial Estate,
#		e eKing's Langley, Hertfordshire, WD4 8LZ, U.K.

popdivert


###############################################################################
##
##	Some defines for useful constants
##
###############################################################################
define([[TRUE]],					[[1]])
define([[FALSE]],					[[0]])


###############################################################################
##
##	Some defines for where things go.
##
###############################################################################
define([[RC_DESTDIR]],				[[/etc/init.d]])
define([[MOD_ROOTDIR]],				[[/lib/modules/KERNEL_ID]])
define([[MOD_DESTDIR]],				[[MOD_ROOTDIR/extra]])
define([[MODULES_DEP]],				[[${DISCIMAGE}MOD_ROOTDIR/modules.dep]])
define([[MODULES_TMP]],				[[/tmp/modules.$$.tmp]])
define([[MODULES_CONF]],			[[${DISCIMAGE}/etc/modules.conf]])
define([[MODPROBE_CONF]],			[[${DISCIMAGE}/etc/modprobe.conf]])
define([[LIBMODPROBE_CONF]],		[[${DISCIMAGE}/lib/modules/modprobe.conf]])
define([[PATH_DEPMOD]],				[[/sbin/depmod]])
define([[PATH_MODPROBE]],			[[/sbin/modprobe]])
define([[APK_DESTDIR]],				[[/data/app]])
define([[HAL_DESTDIR]],				[[SHLIB_DESTDIR/hw]])

###############################################################################
##
## Diversion #3 - start of the versioning trace information.
##
###############################################################################
pushdivert(3)dnl
[[#]] Auto-generated for PVR_BUILD_DIR from ifelse(BUILD,release, build: PVRVERSION)
popdivert


###############################################################################
##
## Diversion #4 - end of versioning trace information.
## Leave a comment line and a blank line for output tidiness.
##
###############################################################################
pushdivert(4)dnl
#

popdivert


###############################################################################
##
## VERSION - 
## Leave a trace comment in the final output showing what was used to get here. 
## We actually invoke the defined macro here to give trace output for this
## file itself.
##
###############################################################################
define([[VERSION]], [[dnl
ifelse(BUILD,release,, [[dnl
pushdivert(3)dnl
[[#]]	$1
popdivert
]])]])dnl

VERSION($RCSfile: common.m4 $ $Revision: 1.10 $)


## The following two commented line are templates for creating 'variables' which
## operate thus:
##
## A(5) -> A=5
## A -> outputs A
## CAT_A(N) -> A=A,N
##
## define([[A]], [[ifelse($#, 0, _A, [[define([[_A]], $1)]])]])dnl
## define([[CAT_A]], [[define([[_A]], ifelse(defn([[_A]]),,$1,[[defn([[_A]])[[,$1]]]]))]])


###############################################################################
##
## TARGET_HAS_DEPMOD -
## Indicates that the target has a depmod binary available.  This means we
## don't have to update modules.dep manually - unpleasant.
##
###############################################################################
define([[TARGET_HAS_DEPMOD]], [[ifelse($#, 0, _$0, [[define([[_$0]], $1)]])]])


###############################################################################
##
## TARGET_RUNS_DEPMOD_AT_BOOT
## Indicates that the target runs depmod every boot time.  This means we don't
## have to run it ourselves.
##
###############################################################################
define([[TARGET_RUNS_DEPMOD_AT_BOOT]], [[ifelse($#, 0, _$0, [[define([[_$0]], $1)]])]])

define([[PLATFORM_VERSION]],		[[VERSION($*)]])

define([[DISPLAY_CONTROLLER]],		[[define([[_DISPLAY_CONTROLLER_]], $1),
									  define([[_DISPLAY_CONTROLLER_FAILED_]], $1_failed)]])

define([[BUFFER_CLASS_DEVICE]],		[[define([[_BUFFER_CLASS_DEVICE_]], $1),
									  define([[_BUFFER_CLASS_DEVICE_FAILED_]], $1_failed)]])
