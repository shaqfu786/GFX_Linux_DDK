# Copyright	2010 Imagination Technologies Limited. All rights reserved.
#
# No part of this software, either material or conceptual may be
# copied or distributed, transmitted, transcribed, stored in a
# retrieval system or translated into any human or computer
# language in any form by any means, electronic, mechanical,
# manual or other-wise, or disclosed to third parties without
# the express written permission of: Imagination Technologies
# Limited, HomePark Industrial Estate, Kings Langley,
# Hertfordshire, WD4 8LZ, UK
#
# $Log: Linux.mk $
#

modules := PVRScopeServices

PVRScopeServices_type := shared_library
PVRScopeServices_target := libPVRScopeServices_SGX$(SGXCORE)_$(SGX_CORE_REV).so
PVRScopeServices_src := PVRScopeServices.cpp
PVRScopeServices_includes := include4 hwdefs
PVRScopeServices_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)
PVRScopeServices_extlibs := stdc++
