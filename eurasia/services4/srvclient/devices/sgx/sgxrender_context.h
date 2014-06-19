#ifndef __SGXRENDER_CONTEXT_H__
#define __SGXRENDER_CONTEXT_H__

IMG_IMPORT PVRSRV_ERROR
SGXDestroyRenderContextUM(PVRSRV_DEV_DATA *psDevData,
                          PSGX_RENDERCONTEXT psRenderContext);

IMG_IMPORT PVRSRV_ERROR
SGXCreateRenderContextUM(PVRSRV_DEV_DATA *psDevData,
                         PSGX_CREATERENDERCONTEXT psCreateRenderContext,
                         IMG_HANDLE *phRenderContext,
                         PVRSRV_CLIENT_MEM_INFO **ppsVisTestResultMemInfo);

#endif /* __SGXRENDER_CONTEXT_H__ */

