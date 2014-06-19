########################################################################### ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Strictly Confidential.
### ###########################################################################

cores_with_monolithic_ukernel := 543 544 545 554
monolithic_ukernel :=
ifneq ($(words $(cores_with_monolithic_ukernel)),$(words \
$(filter-out $(SGXCORE),$(cores_with_monolithic_ukernel))))
monolithic_ukernel := true
endif

# PDS/USE headers are built as module_name.h. These definitions exploit that
# to shorten the rest of the makefile
ukernel_pds := sgxinit_primary1_pds sgxinit_primary2_pds \
               sgxinit_secondary_pds eventhandler_pds loopback_pds \
               vdmcontextstore_pds tastaterestore_pds tasarestore_pds

ifeq ($(monolithic_ukernel),true)
ukernel_use := ukernel vdmcontextstore_use tastaterestore_use tasarestore_use cmplxstaterestore
else
ukernel_use := ukernel ukernelTA ukernelSPM ukernel3D ukernel2D
endif

ifeq ($(SGX_FEATURE_MP),1)
ukernel_use += slave_ukernel
ukernel_pds += slave_eventhandler_pds
endif

modules := libsrv_init $(ukernel_use) $(ukernel_pds) pvrsrvinit_SGX$(SGXCORE)_$(SGX_CORE_REV) pvrsrvctl pvrsrvinit

libsrv_init_type := shared_library
libsrv_init_src := $(TOP)/services4/srvinit/common/srvinit.c
libsrv_init_cflags := $(if $(filter 1,$(W)),,-Werror)

libsrv_init_includes := include4 hwdefs services4/include \
                        services4/srvinit/include services4/system/$(PVR_SYSTEM) \
                        $(TARGET_INTERMEDIATES)/libsrv_init \
                        $(foreach _h,$(ukernel_pds) $(ukernel_use),$(TARGET_INTERMEDIATES)/$(_h))
libsrv_init_genheaders := \
 $(foreach _h,$(ukernel_pds) $(ukernel_use),$(_h)/$(_h).h) \
 $(foreach _h,$(ukernel_pds),$(_h)/$(_h)_sizeof.h) \
 $(foreach _h,$(ukernel_use),$(_h)/$(_h)_labels.h)

libsrv_init_libs := srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

ifeq ($(SUPPORT_SGX),1)
libsrv_init_src += $(TOP)/services4/srvinit/devices/sgx/sgxinit.c
libsrv_init_includes += services4/devices/sgx
endif

all_ukernel_includes := hwdefs include4 services4/srvinit/devices/sgx \
                        services4/include services4/system/$(PVR_SYSTEM)


ukernel_type := use_header
ukernel_name := uKernelProgram
ukernel_label := UKERNEL_

ifeq ($(monolithic_ukernel),true)
ukernel_src := \
 $(TOP)/services4/srvinit/devices/sgx/eventhandler.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_init.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/supervisor.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_timer.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/ta.asm \
 $(TOP)/services4/srvinit/devices/sgx/spm.asm \
 $(TOP)/services4/srvinit/devices/sgx/3d.asm \
 $(TOP)/services4/srvinit/devices/sgx/2d.asm \
 $(TOP)/services4/srvinit/devices/sgx/3dcontextswitch.asm \
 $(TOP)/services4/srvinit/devices/sgx/tacontextswitch.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
else
ukernel_src := \
 $(TOP)/services4/srvinit/devices/sgx/eventhandler.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_init.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_timer.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
endif
ukernel_includes := $(all_ukernel_includes)

ukernelTA_type := use_header
ukernelTA_name := uKernelTAProgram
ukernelTA_label := UKERNEL_TA_
ukernelTA_src := \
 $(TOP)/services4/srvinit/devices/sgx/ta.asm \
 $(TOP)/services4/srvinit/devices/sgx/tacontextswitch.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
ukernelTA_includes := $(all_ukernel_includes)

ukernelSPM_type := use_header
ukernelSPM_name := uKernelSPMProgram
ukernelSPM_label := UKERNEL_SPM_
ukernelSPM_src := \
 $(TOP)/services4/srvinit/devices/sgx/spm.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
ukernelSPM_includes := $(all_ukernel_includes)

ukernel3D_type := use_header
ukernel3D_name := uKernel3DProgram
ukernel3D_label := UKERNEL_3D_
ukernel3D_src := \
 $(TOP)/services4/srvinit/devices/sgx/3d.asm \
 $(TOP)/services4/srvinit/devices/sgx/3dcontextswitch.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
ukernel3D_includes := $(all_ukernel_includes)

ukernel2D_type := use_header
ukernel2D_name := uKernel2DProgram
ukernel2D_label := UKERNEL_2D_
ukernel2D_src := \
 $(TOP)/services4/srvinit/devices/sgx/2d.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
ukernel2D_includes := $(all_ukernel_includes)

sgxinit_primary1_pds_type := pds_header
sgxinit_primary1_pds_src := $(TOP)/services4/srvinit/devices/sgx/sgxinit_primary.pds.asm
sgxinit_primary1_pds_label := UKERNEL_INIT_PRIM1
sgxinit_primary1_pds_includes := $(all_ukernel_includes)

sgxinit_primary2_pds_type := pds_header
sgxinit_primary2_pds_src := $(TOP)/services4/srvinit/devices/sgx/sgxinit_primary.pds.asm
sgxinit_primary2_pds_label := UKERNEL_INIT_PRIM2
sgxinit_primary2_pds_includes := $(all_ukernel_includes)

sgxinit_secondary_pds_type := pds_header
sgxinit_secondary_pds_src := $(TOP)/services4/srvinit/devices/sgx/sgxinit_secondary.pds.asm
sgxinit_secondary_pds_label := UKERNEL_INIT_SEC
sgxinit_secondary_pds_includes := $(all_ukernel_includes)

eventhandler_pds_type := pds_header
eventhandler_pds_src := $(TOP)/services4/srvinit/devices/sgx/eventhandler.pds.asm
eventhandler_pds_label := UKERNEL_EVENTS
eventhandler_pds_includes := $(all_ukernel_includes)

loopback_pds_type := pds_header
loopback_pds_src := $(TOP)/services4/srvinit/devices/sgx/loopback.pds.asm
loopback_pds_label := UKERNEL_LB
loopback_pds_includes := $(all_ukernel_includes)

vdmcontextstore_pds_type := pds_header
vdmcontextstore_pds_src := $(TOP)/services4/srvinit/devices/sgx/pds_vdmcontextstore.asm
vdmcontextstore_pds_label := VDMCONTEXT_STORE
vdmcontextstore_pds_includes := $(all_ukernel_includes)

tastaterestore_pds_type := pds_header
tastaterestore_pds_src := $(TOP)/services4/srvinit/devices/sgx/tastaterestorepds.asm
tastaterestore_pds_label := TASTATERESTORE
tastaterestore_pds_includes := $(all_ukernel_includes)

tasarestore_pds_type := pds_header
tasarestore_pds_src := $(TOP)/services4/srvinit/devices/sgx/tasarestore.pds.asm
tasarestore_pds_label := TASARESTORE
tasarestore_pds_includes := $(all_ukernel_includes)

tastaterestore_use_type := use_header
tastaterestore_use_src := $(TOP)/services4/srvinit/devices/sgx/tastaterestore.asm
tastaterestore_use_label := TASTATE_
tastaterestore_use_includes := $(all_ukernel_includes)
tastaterestore_use_name := USEProgramTAStateRestore

vdmcontextstore_use_type := use_header
vdmcontextstore_use_src := $(TOP)/services4/srvinit/devices/sgx/vdmcontextstore_use.asm
vdmcontextstore_use_label := TACS_TERM_
vdmcontextstore_use_includes := $(all_ukernel_includes)
vdmcontextstore_use_name := USEProgramVDMContextStore
vdmcontextstore_use_assembleonly := true

tasarestore_use_type := use_header
tasarestore_use_src := $(TOP)/services4/srvinit/devices/sgx/tasarestore.use.asm
tasarestore_use_label := TASAR_
tasarestore_use_includes := $(all_ukernel_includes)
tasarestore_use_name := USEProgramTASARestore

cmplxstaterestore_type := use_header
cmplxstaterestore_src := $(TOP)/services4/srvinit/devices/sgx/cmplxstaterestore.asm
cmplxstaterestore_label := TSTATE_
cmplxstaterestore_includes := $(all_ukernel_includes)
cmplxstaterestore_name := USEProgramCmplxStateRestore

slave_ukernel_type := use_header
slave_ukernel_src := \
 $(TOP)/services4/srvinit/devices/sgx/slave_eventhandler.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/slave_supervisor.use.asm \
 $(TOP)/services4/srvinit/devices/sgx/sgx_utils.asm
slave_ukernel_label := SLAVE_UKERNEL_
slave_ukernel_includes := $(all_ukernel_includes)
slave_ukernel_name := SlaveuKernelProgram

slave_eventhandler_pds_type := pds_header
slave_eventhandler_pds_src := $(TOP)/services4/srvinit/devices/sgx/slave_eventhandler.pds.asm
slave_eventhandler_pds_label := SLAVE_UKERNEL_EVENTS
slave_eventhandler_pds_includes := $(all_ukernel_includes)

pvrsrvctl_type := executable
pvrsrvctl_src := pvrsrvctl.c
pvrsrvctl_includes := include4 services4/srvinit/include
pvrsrvctl_libs := srv_init_SGX$(SGXCORE)_$(SGX_CORE_REV) srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)
pvrsrvctl_cflags := -DBUILD_PVRSRVCTL

pvrsrvinit_SGX$(SGXCORE)_$(SGX_CORE_REV)_type := executable
pvrsrvinit_SGX$(SGXCORE)_$(SGX_CORE_REV)_src := pvrsrvinit.c
pvrsrvinit_SGX$(SGXCORE)_$(SGX_CORE_REV)_includes := include4 services4/srvinit/include
pvrsrvinit_SGX$(SGXCORE)_$(SGX_CORE_REV)_libs := srv_init_SGX$(SGXCORE)_$(SGX_CORE_REV) srv_um_SGX$(SGXCORE)_$(SGX_CORE_REV)

pvrsrvinit_type := executable
pvrsrvinit_src := pvrsrvinit_wrapper.c pvrsrvctl.c
pvrsrvinit_includes := include4 services4/srvinit/include
