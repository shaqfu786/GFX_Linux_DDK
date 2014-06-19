/*!****************************************************************************
@File           blitops.use

@Title          Hardware Composer (hwcomposer) HAL component (blit USE)

@Author         Imagination Technologies

@Date           2011/06/03

@Copyright      Copyright (C) Imagination Technologies Limited.
                All rights reserved. No part of this software, either material
                or conceptual may be copied or distributed, transmitted,
                transcribed, stored in a retrieval system or translated into
                any human or computer language in any form by any means,
                electronic, mechanical, manual or otherwise, or disclosed
                to third parties without the express written permission of
                Imagination Technologies Limited, Home Park Estate,
                Kings Langley, Hertfordshire, WD4 8LZ, U.K.

@Platform       linux

******************************************************************************/

/* Registers for custom usse blt (8 bits per channel):
 * Inputs:
 *  pa0 - source pixel
 *  pa1 - dest pixel
 *  pa2 - source2 pixel (optional)
 *  sa0 - UseParams[0] (per-blt constant)
 *  sa1 - UseParams[1]
 * Outputs :
 *  o0 - dest pixel
 */

.export AlphaToFF;
.export AlphaToFFEnd;

AlphaToFF:
{
	or.skipinv.end  o0, pa0, #0xFF000000;
}
AlphaToFFEnd:
