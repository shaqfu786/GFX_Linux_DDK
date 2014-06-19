/*!****************************************************************************
@File           fdsocket.c

@Title          Graphics HAL (fd passing)

@Author         Imagination Technologies

@Date           2009/03/19

@Copyright      Copyright 2009 by Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

@Description    Graphics HAL (fd passing)

******************************************************************************/
/******************************************************************************
Modifications :-
$Log: fdsocket.c $
******************************************************************************/

#include "fdsocket.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>

#define UNIX_SOCKET_PATH "/blah"

static inline int handleIntsInBytes(native_handle_t *psNativeHandle)
{
	return psNativeHandle->numInts * sizeof(int);
}

static inline int handleFdsInBytes(native_handle_t *psNativeHandle)
{
	return psNativeHandle->numFds * sizeof(int);
}

static IMG_BOOL handleRecv(int fd, native_handle_t *psNativeHandle)
{
	char *pcFdData, *pcIntData, *aCmsgBuf;
	struct cmsghdr *psCmsg;
	struct iovec sIovec;
	struct msghdr sMsg;
	int iCmsgBufLen;

	iCmsgBufLen = CMSG_SPACE(handleFdsInBytes(psNativeHandle));
	aCmsgBuf = malloc(iCmsgBufLen);

	pcFdData = ((char *)psNativeHandle) + sizeof(native_handle_t);
	pcIntData = pcFdData + handleFdsInBytes(psNativeHandle);

	memset(&sMsg, 0, sizeof(struct msghdr));

	sIovec.iov_len = handleIntsInBytes(psNativeHandle);
	sIovec.iov_base = pcIntData;
	sMsg.msg_iov = &sIovec;
	sMsg.msg_iovlen = 1;

	sMsg.msg_controllen = iCmsgBufLen;
	sMsg.msg_control = aCmsgBuf;

	if(recvmsg(fd, &sMsg, MSG_NOSIGNAL) < 0)
	{
		perror("recvmesg");
		return IMG_FALSE;
	}

	psCmsg = CMSG_FIRSTHDR(&sMsg);
	memcpy(pcFdData, CMSG_DATA(psCmsg), handleFdsInBytes(psNativeHandle));

	free(aCmsgBuf);
	return IMG_TRUE;
}

static IMG_BOOL handleSend(int fd, native_handle_t *psNativeHandle)
{
	char *pcFdData, *pcIntData, *aCmsgBuf;
	struct cmsghdr *psCmsg;
	struct iovec sIovec;
	struct msghdr sMsg;
	int iCmsgBufLen;

	iCmsgBufLen = CMSG_SPACE(handleFdsInBytes(psNativeHandle));
	aCmsgBuf = malloc(iCmsgBufLen);

	pcFdData = ((char *)psNativeHandle) + sizeof(native_handle_t);
	pcIntData = pcFdData + handleFdsInBytes(psNativeHandle);

	memset(&sMsg, 0, sizeof(struct msghdr));

	sIovec.iov_len = handleIntsInBytes(psNativeHandle);
	sIovec.iov_base = pcIntData;
	sMsg.msg_iov = &sIovec;
	sMsg.msg_iovlen = 1;

	sMsg.msg_controllen = iCmsgBufLen;
	sMsg.msg_control = aCmsgBuf;

	psCmsg = CMSG_FIRSTHDR(&sMsg);
	psCmsg->cmsg_level = SOL_SOCKET;
	psCmsg->cmsg_type = SCM_RIGHTS;
	psCmsg->cmsg_len = CMSG_LEN(handleFdsInBytes(psNativeHandle));

	memcpy(CMSG_DATA(psCmsg), pcFdData, handleFdsInBytes(psNativeHandle));
	sMsg.msg_controllen = psCmsg->cmsg_len;

	if(sendmsg(fd, &sMsg, MSG_NOSIGNAL) < 0)
	{
		perror("sendmsg");
		return IMG_FALSE;
	}

	return IMG_TRUE;
}

IMG_EXPORT
IMG_BOOL waitForClientSendHandle(native_handle_t *psNativeHandle)
{
	struct sockaddr_un local, remote;
	IMG_BOOL ret = IMG_FALSE;
	fd_set readfds;
	socklen_t len;
	int s, s2;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0)
	{
		perror("socket");
		goto err_out;
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, UNIX_SOCKET_PATH);
	unlink(local.sun_path); /* may fail */

	len = strlen(local.sun_path) + sizeof(sa_family_t) + 1;
	if(bind(s, (struct sockaddr *)&local, len) < 0)
	{
		perror("bind");
		goto err_close;
	}

	if(listen(s, 0) < 0)
	{
		perror("listen");
		goto err_close;
	}

	len = (socklen_t)sizeof(struct sockaddr_un);
	s2 = accept(s, (struct sockaddr *)&remote, &len);

	if(send(s2, &psNativeHandle->numFds, sizeof(int), MSG_NOSIGNAL) < 0)
	{
		perror("send1");
		goto err_close_2;
	}

	if(send(s2, &psNativeHandle->numInts, sizeof(int), MSG_NOSIGNAL) < 0)
	{
		perror("send2");
		goto err_close_2;
	}

	if(!handleSend(s2, psNativeHandle))
		goto err_close_2;

	FD_ZERO(&readfds);
	FD_SET(s2, &readfds);
	select(s2+1, &readfds, NULL, NULL, NULL);

	ret = IMG_TRUE;
err_close_2:
	close(s2);
err_close:
	close(s);
err_out:
	return ret;
}

IMG_EXPORT
IMG_BOOL connectServerRecvHandle(native_handle_t **ppsNativeHandle)
{
	native_handle_t *psNativeHandle;
	IMG_BOOL ret = IMG_FALSE;
	struct sockaddr_un local;
	int s, numFds, numInts;
	size_t len;

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if(s < 0) {
		perror("socket");
		goto err_out;
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, UNIX_SOCKET_PATH);

	len = strlen(local.sun_path) + sizeof(sa_family_t) + 1;
	if(connect(s, (struct sockaddr *)&local, len)) {
		perror("connect");
		goto err_close;
	}

	if(recv(s, &numFds, sizeof(int), MSG_NOSIGNAL) < 0)
	{
		perror("send1");
		goto err_close;
	}

	if(recv(s, &numInts, sizeof(int), MSG_NOSIGNAL) < 0)
	{
		perror("send2");
		goto err_close;
	}

	psNativeHandle = native_handle_create(numFds, numInts);

	if(!handleRecv(s, psNativeHandle))
		goto err_close;

	*ppsNativeHandle = psNativeHandle;
	ret = IMG_TRUE;
err_close:
	close(s);
err_out:
	return ret;
}
