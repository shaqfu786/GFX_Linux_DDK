/*!****************************************************************************
@File           graphics_buffer_interface_base.h

@Title          Common header containing type definitions for the Common Buffer Interface

@Author         Texas Instruments

@date           01/015/2012

* Copyright © 2011 Texas Instruments Incorporated
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* Author(s):
*    Atanas (Tony) Zlatinski <x0146664@ti.com> <zlatinski@gmail.com>


@Platform       Generic

@Description    Interfaces and type definitions for the CBI.

@DoxygenVer

******************************************************************************/

#ifndef graphics_buffer_interface_base_h__
#define graphics_buffer_interface_base_h__

/********************* Subsystem definitions ******************************/
/* Windowing system */
#define GPU_MEMORY_SUBSYSTEM_WS                   ((IMG_UINT64)(0xFFF << 0))
/* Reserved space for WS specific OBJECTS
 * FB, general buffers, mapped, etc */

#define GPU_MEMORY_SUBSYSTEM_PBUFFER_SURFACE      ((IMG_UINT64)(0x01 << 1))
#define GPU_MEMORY_SUBSYSTEM_PIXMAP_SURFACE       ((IMG_UINT64)(0x01 << 2))
#define GPU_MEMORY_SUBSYSTEM_WINDOW_SURFACE       ((IMG_UINT64)(0x01 << 3))
#define GPU_MEMORY_SUBSYSTEM_MM_BUFFER            ((IMG_UINT64)(0x01 << 4))
#define GPU_MEMORY_SUBSYSTEM_TILER_BUFFER         ((IMG_UINT64)(0x01 << 5))
#define GPU_MEMORY_SUBSYSTEM_DEPTH_BUFFER         ((IMG_UINT64)(0x01 << 6))
#define GPU_MEMORY_SUBSYSTEM_STENCIL_BUFFER       ((IMG_UINT64)(0x01 << 7))

#define GPU_MEMORY_SUBSYSTEM_IMPORTED_PIXMAP      ((IMG_UINT64)(0x01 << 8))
#define GPU_MEMORY_SUBSYSTEM_WS_RESERVED_1        ((IMG_UINT64)(0x01 << 9))
#define GPU_MEMORY_SUBSYSTEM_WS_RESERVED_2        ((IMG_UINT64)(0x01 << 10))
#define GPU_MEMORY_SUBSYSTEM_WS_RESERVED_3        ((IMG_UINT64)(0x01 << 11))

#define GPU_MEMORY_SUBSYSTEM_EGL                  ((IMG_UINT64)(0x0F << 12))
/* Reserved space for EGL specific OBJECTS
 * EGL_Image, etc.  */
#define GPU_MEMORY_SUBSYSTEM_EGL_NOK_IMPORT       ((IMG_UINT64)(0x01 << 12))

#define GPU_MEMORY_SUBSYSTEM_EGL_IMAGE            ((IMG_UINT64)(0x0F << 12))

#define GPU_MEMORY_SUBSYSTEM_OPENGL1              ((IMG_UINT64)(0xFF << 16))
/* Reserved space for OPENGL1 specific OBJECTS */
#define GPU_MEMORY_SUBSYSTEM_OPENGL1_TEXTURE      ((IMG_UINT64)(0x01 << 16))
#define GPU_MEMORY_SUBSYSTEM_OPENGL1_PROGRAM      ((IMG_UINT64)(0x01 << 17))
#define GPU_MEMORY_SUBSYSTEM_OPENGL1_BUFOBJ       ((IMG_UINT64)(0x01 << 18))
#define GPU_MEMORY_SUBSYSTEM_OPENGL1_RENDERBUFFER ((IMG_UINT64)(0x01 << 19))
#define GPU_MEMORY_SUBSYSTEM_OPENGL1_FRAMEBUFFER  ((IMG_UINT64)(0x01 << 20))
#define GPU_MEMORY_SUBSYSTEM_OPENGL1_VAO          ((IMG_UINT64)(0x01 << 21))

#define GPU_MEMORY_SUBSYSTEM_OPENGL2              ((IMG_UINT64)(0xFF << 24))
/* Reserved space for OPENGL2 specific OBJECTS */
#define GPU_MEMORY_SUBSYSTEM_OPENGL2_TEXTURE      ((IMG_UINT64)(0x01 << 24))
#define GPU_MEMORY_SUBSYSTEM_OPENGL2_PROGRAM      ((IMG_UINT64)(0x01 << 25))
#define GPU_MEMORY_SUBSYSTEM_OPENGL2_BUFOBJ       ((IMG_UINT64)(0x01 << 26))
#define GPU_MEMORY_SUBSYSTEM_OPENGL2_RENDERBUFFER ((IMG_UINT64)(0x01 << 27))
#define GPU_MEMORY_SUBSYSTEM_OPENGL2_FRAMEBUFFER  ((IMG_UINT64)(0x01 << 28))
#define GPU_MEMORY_SUBSYSTEM_OPENGL2_VAO          ((IMG_UINT64)(0x01 << 29))

#define GPU_MEMORY_SUBSYSTEM_OPENVG               ((IMG_UINT64)(0xFF << 32))
/* Reserved space for OPENVG specific OBJECTS */

#define GPU_MEMORY_SUBSYSTEM_OPENCL               ((IMG_UINT64)(0xFF << 40))
/* Reserved space for OPENCL specific OBJECTS */

#define GPU_MEMORY_SUBSYSTEM_ALL                  (~((IMG_UINT64)0))


/* Collect memory use information */
#define GPU_MEMORY_COLLECT_MEMORY_USE_INFO      ((IMG_UINT32)1 << 0)

/* Dump the data to the console */
#define GPU_MEMORY_CTRL_DUMP                    ((IMG_UINT32)1 << 1)

/* Try to reclaim GPU resources */
#define GPU_MEMORY_CTRL_RECLAIM_GPU_RESOURCES   ((IMG_UINT32)1 << 2)

typedef enum PVRSRV_OBJECT_IF_ID_
{
	PVRSRV_BASE_IF_ID = 0,
	PVRSRV_GPU_RESOURCE_IF_ID,
	PVRSRV_CPU_RESOURCE_IF_ID,
	PVRSRV_LAST_IF_ID = (IMG_UINT32)-1,
}PVRSRV_OBJECT_IF_ID;

/* Hint flags */
#define PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_ALLOCATED     (1 << 0)
#define PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_DISPLAY_OWNED (1 << 1)
#define PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_WRAPPED       (1 << 2)
#define PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_MAPPED        (1 << 3)
#define PVRSRV_GPU_MEMORY_RESOURCE_OBJECT_TYPE_MASK     (0xF)

struct PVRSRV_RESOURCE_OBJECT_BASE_IF_;

typedef struct PVRSRV_RESOURCE_OBJECT_BASE_
{

	const struct PVRSRV_RESOURCE_OBJECT_BASE_IF_ *pIf;

	IMG_UINT32 ui32RefCount;

}PVRSRV_RESOURCE_OBJECT_BASE;


/**
 * General Object Interface
 */
typedef struct PVRSRV_RESOURCE_OBJECT_BASE_IF_
{

	/* Obtains an interface of the object of type interfaceID.
	 * Note, the function automatically increments the interface
	 * reference count.
	 * On success it returns a not NULL interface and PVRSRV_OK
	 * On error, it return PVRSRV_ERROR negative code */
	IMG_INT32 (*pfnGetInterface)(PVRSRV_RESOURCE_OBJECT_BASE *pObject,
			PVRSRV_OBJECT_IF_ID interfaceID, IMG_VOID ** ppObject);
	/**
	* Increases the reference count for this interface.
	* The associated instance will not be deleted unless
	* the reference count goes down to zero.
	*
	* return: If result < 0: PVRSRV_ERROR error. SUCCESS otherwise;
	*/
	IMG_INT32 (*pfnAddRef)(PVRSRV_RESOURCE_OBJECT_BASE *pObject);

	/**
	* Decreases the reference count for this interface.
	* Generally, if the reference count returns to zero,
	* the associated instance is deleted.
	*
	* return: If result < 0: PVRSRV_ERROR error. SUCCESS otherwise;
	*/
	IMG_INT32 (*pfnRelease)(PVRSRV_RESOURCE_OBJECT_BASE *pObject, IMG_BOOL bIsShutdown);

}PVRSRV_RESOURCE_OBJECT_BASE_IF;

struct PVRSRV_OBJECT_IF_GPU_MAPPING_;

typedef struct PVRSRV_GPU_RESOURCE_OBJECT_
{

	const struct PVRSRV_OBJECT_IF_GPU_MAPPING_ *pIf;

	IMG_UINT32 ui32RefCount;

}PVRSRV_GPU_RESOURCE_OBJECT;

/**
 * GPU mapping buffer object interface. Buffer objects which implement
 * this interface support mapping / unmapping of GPU memory and
 * GPU memory eviction
 */
typedef struct PVRSRV_OBJECT_IF_GPU_MAPPING_
{

	PVRSRV_RESOURCE_OBJECT_BASE_IF baseObjectIf;

	/** pfnEnsureGpuMapping() obtains the GPU virtual address of buffer.
	* Please note that this function does not increment the reference count.
	* If the caller has not incremented the interface count by explicitly
	* obtaining the interface (borrowing it from a different client) or explicitly
	* incrementing it with pfnAddRef() above, then it is very possible
	* the GPU mapping to go away while the driver is trying to use the buffer.
	* If  the caller of the function supplies a valid pointer to psDevVAddr, then
	* the function would try to obtain the GPU resources before returning,
	* supplying the GPU address within the sDevVAddr.
	* On failure, the function returns negative error value, otherwise
	* it returns 0 if the address is not available, otherwise the size of
	* the GPU mapping, if there is a valid GPU virtual address mapping.
	* */
	IMG_INT32 (*pfnEnsureGpuMapping)(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject,
			IMG_DEV_VIRTADDR* psDevVAddr);

	/* pfnEnsurePlanesGpuMapping() - Get MM Planes Addresses
	 * for use with multi-planar buffers such as YUV (NV12)
	 * The function arguments are:
	 * pasDevPlanesVAddr - an array of addresses  to be filled
	 * for the buffer planes
	 * pui32NumPlanes - the number of slots supplied within pasDevPlanesVAddr
	 * The function returns the plane addresses in the array and the number of
	 * address slots filled
	 * Base on the color format, client must ensure that it passes
	 * sufficient size array of addresses. */
	IMG_INT32 (*pfnEnsurePlanesGpuMapping)(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject,
			IMG_UINT32 *pasDevPlanesVAddr, IMG_UINT32 *pui32NumPlanes);

	/* pfnResourceInUse returns IMG_TRUE if the resource is in use by the GPU and
	 * IMG_FALSE otherwise */
	IMG_BOOL (*pfnResourceInUse)(PVRSRV_GPU_RESOURCE_OBJECT *pGpuObject);

}PVRSRV_OBJECT_IF_GPU_MAPPING;

static INLINE
PVRSRV_RESOURCE_OBJECT_BASE * PVRSRV_BaseInterfaceIsValid(IMG_VOID *pObject)
{
	PVRSRV_RESOURCE_OBJECT_BASE * pBaseInterfce = (PVRSRV_RESOURCE_OBJECT_BASE *)pObject;
	if(pBaseInterfce &&  pBaseInterfce->ui32RefCount && pBaseInterfce->pIf)
		return pBaseInterfce;

	return IMG_NULL;
}

static INLINE
IMG_INT32 PVRSRV_GetInterface(IMG_VOID *pObject,
		PVRSRV_OBJECT_IF_ID interfaceID, IMG_VOID ** ppNewObject)
{
	PVRSRV_RESOURCE_OBJECT_BASE * pBaseInterfce = PVRSRV_BaseInterfaceIsValid(pObject);

	if(pBaseInterfce && ppNewObject)
		return pBaseInterfce->pIf->pfnGetInterface(pBaseInterfce, interfaceID, ppNewObject);

	return -PVRSRV_ERROR_INVALID_PARAMS;
}

static INLINE
IMG_INT32 PVRSRV_AddRef(IMG_VOID *pObject)
{
	PVRSRV_RESOURCE_OBJECT_BASE * pBaseInterfce = PVRSRV_BaseInterfaceIsValid(pObject);

	if(pBaseInterfce)
		return pBaseInterfce->pIf->pfnAddRef(pBaseInterfce);

	return -PVRSRV_ERROR_INVALID_PARAMS;
}

static INLINE
IMG_INT32 PVRSRV_Release(IMG_VOID *pObject, IMG_BOOL bIsShutdown)
{
	PVRSRV_RESOURCE_OBJECT_BASE * pBaseInterfce = PVRSRV_BaseInterfaceIsValid(pObject);

	if(pBaseInterfce)
		return pBaseInterfce->pIf->pfnRelease(pBaseInterfce, bIsShutdown);

	return -PVRSRV_ERROR_INVALID_PARAMS;
}

static INLINE
IMG_INT32 PVRSRV_EnsureGpuMapping(PVRSRV_GPU_RESOURCE_OBJECT *pGpuResource, IMG_DEV_VIRTADDR* psDevVAddr)
{
	if(pGpuResource && pGpuResource->ui32RefCount && pGpuResource->pIf )
		return pGpuResource->pIf->pfnEnsureGpuMapping(pGpuResource, psDevVAddr);

	return -PVRSRV_ERROR_INVALID_PARAMS;
}

static INLINE
IMG_INT32 PVRSRV_EnsurePlanesGpuMapping(PVRSRV_GPU_RESOURCE_OBJECT *pGpuResource,
		IMG_UINT32 *pasDevPlanesVAddr, IMG_UINT32 *pui32NumPlanes)
{
	if(pGpuResource && pGpuResource->ui32RefCount && pGpuResource->pIf )
		return pGpuResource->pIf->pfnEnsurePlanesGpuMapping(pGpuResource,
				pasDevPlanesVAddr, pui32NumPlanes);

	return -PVRSRV_ERROR_INVALID_PARAMS;
}

static INLINE
IMG_BOOL PVRSRV_ResourceInUse(PVRSRV_GPU_RESOURCE_OBJECT *pGpuResource)
{
	if(pGpuResource && pGpuResource->ui32RefCount && pGpuResource->pIf )
		return pGpuResource->pIf->pfnResourceInUse(pGpuResource);

	return IMG_FALSE;
}

IMG_IMPORT
IMG_INT32 MemoryResourceDevMemAllocBuffer(PVRSRV_DEV_DATA *psDevData,
#if defined (SUPPORT_SID_INTERFACE)
	   IMG_SID	 hDevMemHeap,
#else
	   IMG_HANDLE hDevMemHeap,
#endif
	   IMG_UINT32 ui32Attribs,
	   IMG_SIZE_T uSize,
	   IMG_SIZE_T uAlignment,
	   IMG_PVOID pvPrivData,
	   IMG_UINT32 ui32PrivDataLength,
	   PVRSRV_CLIENT_MEM_INFO **ppsMemInfo,
	   IMG_INT	*iExportedFd,
	   IMG_UINT32	ui32Hints,
	   IMG_UINT64 	uiSubSystem,
	   PVRSRV_RESOURCE_OBJECT_BASE **ppsMemResourceObjectBase);

IMG_IMPORT
IMG_INT32 MemoryResourceDevMemImportBuffer(PVRSRV_DEV_DATA *psDevData,
       IMG_INT iFd,
#if defined (SUPPORT_SID_INTERFACE)
	   IMG_SID	 hDevMemHeap,
#else
	   IMG_HANDLE hDevMemHeap,
#endif
	   PVRSRV_CLIENT_MEM_INFO **ppsMemInfo,
	   IMG_UINT32	ui32Hints,
	   IMG_UINT64 	uiSubSystem,
	   PVRSRV_RESOURCE_OBJECT_BASE **ppsMemResourceObjectBase);

IMG_IMPORT
IMG_BOOL MemoryResourceCheckGuardAreaAndInvalidateObject(
		PVRSRV_CLIENT_MEM_INFO *ppsMemInfo);

IMG_IMPORT
IMG_INT32 MemoryResourceInitService(IMG_CONST PVRSRV_CONNECTION *psConnection);

IMG_IMPORT
IMG_INT32 MemoryResourceReclaimMemory(IMG_CONST PVRSRV_CONNECTION *psDevData, IMG_BOOL isShutdown);

IMG_IMPORT
IMG_INT32 MemoryResourceCloseService(IMG_CONST PVRSRV_CONNECTION *psConnection);

IMG_IMPORT
IMG_UINT64 MemoryResourceSetEvictionDebugMask(IMG_CONST PVRSRV_CONNECTION *psConnection,
		IMG_UINT64 ui64EvictionSubSystemDebugMask);

IMG_IMPORT
IMG_UINT64 MemoryResourceSetEvictionEnableMask(IMG_CONST PVRSRV_CONNECTION *psConnection,
		IMG_UINT64 ui64EvictionSubSystemEnableMask);

IMG_IMPORT
IMG_INT32 MemoryResourceGetInterfaceHwAddress(PVRSRV_RESOURCE_OBJECT_BASE *psMemResourceObjectBase,
		PVRSRV_GPU_RESOURCE_OBJECT ** ppsGpuMappingResourceObject,
		IMG_DEV_VIRTADDR *psDevVAddr);

static INLINE
IMG_VOID PVRSRV_SwapMemoryResourceInterface(PVRSRV_RESOURCE_OBJECT_BASE **ppOrgObject, PVRSRV_RESOURCE_OBJECT_BASE *pNewObject)
{
	PVRSRV_RESOURCE_OBJECT_BASE *pObject = *ppOrgObject;

	if(pObject)
	{
		*ppOrgObject = IMG_NULL;
		PVRSRV_Release(pObject, IMG_FALSE);
	}

	*ppOrgObject = pNewObject;

}

static INLINE
IMG_VOID PVRSRV_SwapGpuMemoryResourceInterface(PVRSRV_GPU_RESOURCE_OBJECT **ppOrgObject, PVRSRV_GPU_RESOURCE_OBJECT *pNewObject)
{
	PVRSRV_GPU_RESOURCE_OBJECT *pObject = *ppOrgObject;

	if(pObject)
	{
		*ppOrgObject = IMG_NULL;
		PVRSRV_Release(pObject, IMG_FALSE);
	}

	*ppOrgObject = pNewObject;

}

/******************************************************************************
 Process Memory Operations Requests and Reports
******************************************************************************/

typedef struct _PVRSRV_CB_MEMORY_OPERAION_REQUEST
{
	IMG_UINT64 ui32SubsystemsMask;
	IMG_UINT32 ctrlFlags;
	IMG_UINT32 currently_used;
	IMG_UINT32 evicted;
	IMG_UINT32 newlyEvicted;
}PVRSRV_CB_MEMORY_OPERAION_REQUEST;

typedef PVRSRV_ERROR (*pfGpuMemoryOpRequestCbType)(IMG_VOID* func_context, PVRSRV_CB_MEMORY_OPERAION_REQUEST* memOpRequest);

IMG_IMPORT IMG_CONST IMG_CHAR* IMG_CALLCONV PVRSRVProcMemorySubSystemToName(IMG_UINT64 ui64SubSystem);

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcMemoryOpRequest(IMG_VOID* clientContext,
		PVRSRV_CB_MEMORY_OPERAION_REQUEST* memOpRequest);
IMG_IMPORT IMG_BOOL IMG_CALLCONV PVRSRVUnregisterProcMemoryOpRequestUser(IMG_UINT64 ui64SubSystem,
		IMG_VOID* clientContext );
IMG_IMPORT IMG_BOOL IMG_CALLCONV PVRSRVRegisterProcMemoryOpRequestUser(IMG_UINT64 ui64SubSystem,
		pfGpuMemoryOpRequestCbType pFunc, IMG_VOID* clientContext );
IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVProcBufferReportMemoryUse(IMG_CONST PVRSRV_DEV_DATA *psDevData);
IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PVRSRVTriggerProcBufferMemoryReport(IMG_CONST PVRSRV_DEV_DATA *psDevData);

/**************************************************************************/
/********************** Developer helper MACROs ***************************/
/**************************************************************************/
/* Eviction related messages levels */
#define PVR_DBG_EVICT_VERBOSE  PVR_DBG_VERBOSE
#define PVR_DBG_EVICT_PRINT_ADDR PVR_DBG_VERBOSE

#define PVRSRV_DUMP_MEMINFO(PVR_DBG_TYPE, psClientMemInfo, info_string)\
    {\
		if(psClientMemInfo)\
		{\
			PVR_DPF((PVR_DBG_TYPE, info_string " Meminfo %p, subsystem %s,\n"\
			"Address 0x%08x to 0x%08x with size %d",\
			psClientMemInfo, PVRSRVProcMemorySubSystemToName(psClientMemInfo->uiSubSystem),\
			psClientMemInfo->sDevVAddr.uiAddr,\
			(psClientMemInfo->sDevVAddr.uiAddr != PVRSRV_BAD_DEVICE_ADDRESS)? \
			(psClientMemInfo->sDevVAddr.uiAddr + psClientMemInfo->uAllocSize)\
			: PVRSRV_BAD_DEVICE_ADDRESS,\
			psClientMemInfo->uAllocSize));\
		}\
		else\
		{\
			PVR_DPF((PVR_DBG_TYPE, info_string " Meminfo is NULL"));\
		}\
    }

#define PVRSRV_CHECK_MEMORY_INTERFACE(pObject, expectedRefCount) \
	{ \
		PVRSRV_RESOURCE_OBJECT_BASE * pBaseInterfce = (PVRSRV_RESOURCE_OBJECT_BASE *)pObject; \
		if(pBaseInterfce && ( !pBaseInterfce->pIf || \
				(pBaseInterfce->ui32RefCount < (expectedRefCount))) ) \
		{ \
			PVR_DPF((PVR_DBG_ERROR, "%s Object %p with unexpected refcount %d and " \
				"interface %p", __FUNCTION__, pBaseInterfce, \
				(pBaseInterfce ? pBaseInterfce->ui32RefCount : 0), \
				(pBaseInterfce ? pBaseInterfce->pIf : IMG_NULL) )); \
		}\
		else \
		{ \
			PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Release Object %p with refcount %d and " \
							"interface %p", pBaseInterfce, \
							(pBaseInterfce ? pBaseInterfce->ui32RefCount : 0), \
							(pBaseInterfce ? pBaseInterfce->pIf : IMG_NULL) )); \
		} \
	}

#define PVRSRV_CHECK_MEMORY_IF_MUST_BE_THE_LAST_CLIENT(pObject, psClientMemInfo) \
	{ \
		PVRSRV_RESOURCE_OBJECT_BASE * pBaseInterfce = (PVRSRV_RESOURCE_OBJECT_BASE *)pObject; \
		if(pBaseInterfce && ( !pBaseInterfce->pIf || \
				(pBaseInterfce->ui32RefCount != 1)) ) \
		{ \
			PVR_DPF((PVR_DBG_ERROR, "%s FATAL ERROR: Object %p with unexpected refcount %d and " \
				"interface %p", __FUNCTION__, pBaseInterfce, \
				(pBaseInterfce ? pBaseInterfce->ui32RefCount : 0), \
				(pBaseInterfce ? pBaseInterfce->pIf : IMG_NULL) )); \
			PVRSRV_DUMP_MEMINFO(PVR_DBG_ERROR, psClientMemInfo, "MORE INFO");\
		}\
		else \
		{ \
			PVR_DPF((PVR_DBG_EVICT_VERBOSE, "Release Object %p with refcount %d and " \
							"interface %p", pBaseInterfce, \
							(pBaseInterfce ? pBaseInterfce->ui32RefCount : 0), \
							(pBaseInterfce ? pBaseInterfce->pIf : IMG_NULL) )); \
		} \
	}


#define FREE_RESOURCE_OBJECT_OR_MEMINFO(psMemResourceObjectBase, psMemInfo, psDevData) \
	{ \
		if(psMemResourceObjectBase) \
		{ \
			PVR_DPF((PVR_DBG_EVICT_VERBOSE, \
					"%s RELEASE psMemResourceObjectBase %p with DevAddr 0x%08x", \
					__FUNCTION__, psMemResourceObjectBase, \
					psMemInfo ? \
							psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1)); \
\
			PVRSRV_CHECK_MEMORY_IF_MUST_BE_THE_LAST_CLIENT(psMemResourceObjectBase, psMemInfo); \
			PVRSRV_SwapMemoryResourceInterface(&(psMemResourceObjectBase), IMG_NULL); \
		} \
		else if (psMemInfo) \
		{ \
			PVR_DPF((PVR_DBG_WARNING, \
					"%s RELEASE Meminfo %p with DevAddr 0x%08x", \
					__FUNCTION__, psMemInfo, psMemInfo ? \
							psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1)); \
\
			if(psDevData) \
			{ \
				PVRSRV_ERROR retVal = PVRSRVFreeDeviceMem(psDevData, psMemInfo); \
				if(retVal != PVRSRV_OK) \
				{ \
					PVR_DPF((PVR_DBG_EVICT_VERBOSE, \
						"%s PVRSRVFreeDeviceMem for Meminfo %p with DevAddr 0x%08x has FAILED %d", \
						__FUNCTION__, \
						psMemInfo, psMemInfo ? \
							psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1, retVal)); \
				} \
			} \
			else \
			{ \
				PVR_DPF((PVR_DBG_ERROR, \
						"%s RELEASE Meminfo %p with DevAddr 0x%08x" \
						"psDevData is NULL !!!", \
						__FUNCTION__, psMemInfo, psMemInfo ? \
								psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1)); \
			} \
		} \
		psMemInfo = IMG_NULL; \
	}

#define UNMAP_RESOURCE_OBJECT_OR_MEMINFO(psMemResourceObjectBase, psMemInfo, psDevData) \
	{ \
		if(psMemResourceObjectBase) \
		{ \
			PVR_DPF((PVR_DBG_EVICT_VERBOSE, \
					"%s RELEASE psMemResourceObjectBase %p with DevAddr 0x%08x", \
					__FUNCTION__, psMemResourceObjectBase, \
					psMemInfo ? \
							psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1)); \
\
			PVRSRV_CHECK_MEMORY_IF_MUST_BE_THE_LAST_CLIENT(psMemResourceObjectBase, psMemInfo); \
			PVRSRV_SwapMemoryResourceInterface(&(psMemResourceObjectBase), IMG_NULL); \
		} \
		else if (psMemInfo) \
		{ \
			PVR_DPF((PVR_DBG_WARNING, \
					"%s RELEASE Meminfo %p with DevAddr 0x%08x", \
					__FUNCTION__, psMemInfo, psMemInfo ? \
							psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1)); \
\
			if(psDevData) \
			{ \
				PVRSRV_ERROR retVal = PVRSRVUnmapDeviceMemory(psDevData, psMemInfo); \
				if(retVal != PVRSRV_OK) \
				{ \
					PVR_DPF((PVR_DBG_EVICT_VERBOSE, \
						"%s PVRSRVUnmapDeviceMemory for Meminfo %p with DevAddr 0x%08x has FAILED %d", \
						__FUNCTION__, \
						psMemInfo, psMemInfo ? \
							psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1, retVal)); \
				} \
			} \
			else \
			{ \
				PVR_DPF((PVR_DBG_ERROR, \
						"%s RELEASE Meminfo %p with DevAddr 0x%08x" \
						"psDevData is NULL !!!", \
						__FUNCTION__, psMemInfo, psMemInfo ? \
								psMemInfo->sDevVAddr.uiAddr : (IMG_UINT32)-1)); \
			} \
		} \
		psMemInfo = IMG_NULL; \
	}

/* FIXME: Use function to obtain the valid General heap address range */
#define SGX_GENERAL_HEAP_BASE 0x00001000
#define SGX_GENERAL_HEAP_SIZE (0x0B800000-0x00001000-0x00001000)
#define SGX_GENERAL_HEAP_TOP (SGX_GENERAL_HEAP_BASE + SGX_GENERAL_HEAP_SIZE)

#define VERIFY_HW_ADDRESS(hwAddr, message) \
		{ \
			if(( (hwAddr) == PVRSRV_BAD_DEVICE_ADDRESS) ||\
			((hwAddr) < SGX_GENERAL_HEAP_BASE) ||\
			((hwAddr) > (SGX_GENERAL_HEAP_TOP)))\
			{ \
				PVR_DPF((PVR_DBG_ERROR, "\n%s (%d) " message " : INVALID HW Address 0x%08x\n"\
					"Valid device heap range is 0x%08x to 0x%08x",\
					__FUNCTION__, __LINE__, hwAddr,\
					SGX_GENERAL_HEAP_BASE, SGX_GENERAL_HEAP_TOP)); \
			} \
			else \
			{ \
				PVR_DPF((PVR_DBG_EVICT_PRINT_ADDR, "%s " message " HW Address is 0x%08x", \
						__FUNCTION__, (IMG_UINT32)(hwAddr))); \
			} \
        }

#define VERIFY_HW_ADDRESS_WITH_REF(hwAddr, refHwAddress, message) \
		{ \
			if(hwAddr != refHwAddress) \
			{ \
				PVR_DPF((PVR_DBG_ERROR, "%s (%d) " message " : HW Address does not match "\
						"0x%08x != 0x%08x",\
						__FUNCTION__, __LINE__, hwAddr, refHwAddress)); \
			} \
			VERIFY_HW_ADDRESS(hwAddr, message); \
        }

#endif //graphics_buffer_interface_base_h
