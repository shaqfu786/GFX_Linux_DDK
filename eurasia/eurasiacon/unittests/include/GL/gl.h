#ifndef __gl_h_
#define __gl_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.0 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

#include <GL/glplatform.h>

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef unsigned short GLhalf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef int GLintptrARB;
typedef int GLsizeiptrARB;
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
/* Internal convenience typedefs */
typedef void (*_GLfuncptr)(void);

/*************************************************************/

/* Extensions */
#define GL_VERSION_1_1                    1
#define GL_VERSION_1_2                    1
#define GL_VERSION_1_3                    1
#define GL_VERSION_1_4                    1
#define GL_VERSION_1_5                    1
#define GL_VERSION_2_0                    1
#define GL_ARB_imaging                    1
#define GL_IMG_binary_shader              1
#define GL_EXT_abgr                       1
#define GL_EXT_blend_color                1
#define GL_EXT_blend_logic_op             1
#define GL_EXT_blend_minmax               1
#define GL_EXT_blend_subtract             1
#define GL_EXT_cmyka                      1
#define GL_EXT_convolution                1
#define GL_EXT_copy_texture               1
#define GL_EXT_histogram                  1
#define GL_EXT_packed_pixels              1
#define GL_EXT_point_parameters           1
#define GL_EXT_polygon_offset             1
#define GL_EXT_rescale_normal             1
#define GL_EXT_shared_texture_palette     1
#define GL_EXT_subtexture                 1
#define GL_EXT_texture                    1
#define GL_EXT_texture3D                  1
#define GL_EXT_texture_object             1
#define GL_EXT_vertex_array               1
#define GL_SGIS_detail_texture            0
#define GL_SGIS_fog_function              0
#define GL_SGIS_generate_mipmap           0
#define GL_SGIS_multisample               0
#define GL_SGIS_pixel_texture             0
#define GL_SGIS_point_line_texgen         0
#define GL_SGIS_point_parameters          0
#define GL_SGIS_sharpen_texture           0
#define GL_SGIS_texture4D                 0
#define GL_SGIS_texture_border_clamp      0
#define GL_SGIS_texture_edge_clamp        0
#define GL_SGIS_texture_filter4           0
#define GL_SGIS_texture_lod               0
#define GL_SGIS_texture_select            0
#define GL_SGIX_async                     0
#define GL_SGIX_async_histogram           0
#define GL_SGIX_async_pixel               0
#define GL_SGIX_blend_alpha_minmax        0
#define GL_SGIX_calligraphic_fragment     0
#define GL_SGIX_clipmap                   0
#define GL_SGIX_convolution_accuracy      0
#define GL_SGIX_depth_texture             0
#define GL_SGIX_flush_raster              0
#define GL_SGIX_fog_offset                0
#define GL_SGIX_fragment_lighting         0
#define GL_SGIX_framezoom                 0
#define GL_SGIX_icc_texture               0
#define GL_SGIX_impact_pixel_texture      0
#define GL_SGIX_instruments               0
#define GL_SGIX_interlace                 0
#define GL_SGIX_ir_instrument1            0
#define GL_SGIX_list_priority             0
#define GL_SGIX_pixel_texture             0
#define GL_SGIX_pixel_tiles               0
#define GL_SGIX_polynomial_ffd            0
#define GL_SGIX_reference_plane           0
#define GL_SGIX_resample                  0
#define GL_SGIX_scalebias_hint            0
#define GL_SGIX_shadow                    0
#define GL_SGIX_shadow_ambient            0
#define GL_SGIX_sprite                    0
#define GL_SGIX_subsample                 0
#define GL_SGIX_tag_sample_buffer         0
#define GL_SGIX_texture_add_env           0
#define GL_SGIX_texture_coordinate_clamp  0
#define GL_SGIX_texture_lod_bias          0
#define GL_SGIX_texture_multi_buffer      0
#define GL_SGIX_texture_scale_bias        0
#define GL_SGIX_vertex_preclip            0
#define GL_SGIX_ycrcb                     0
#define GL_SGI_color_matrix               0
#define GL_SGI_color_table                0
#define GL_SGI_texture_color_table        0

/* AttribMask */
#define GL_CURRENT_BIT                    0x00000001
#define GL_POINT_BIT                      0x00000002
#define GL_LINE_BIT                       0x00000004
#define GL_POLYGON_BIT                    0x00000008
#define GL_POLYGON_STIPPLE_BIT            0x00000010
#define GL_PIXEL_MODE_BIT                 0x00000020
#define GL_LIGHTING_BIT                   0x00000040
#define GL_FOG_BIT                        0x00000080
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_ACCUM_BUFFER_BIT               0x00000200
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_VIEWPORT_BIT                   0x00000800
#define GL_TRANSFORM_BIT                  0x00001000
#define GL_ENABLE_BIT                     0x00002000
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_HINT_BIT                       0x00008000
#define GL_EVAL_BIT                       0x00010000
#define GL_LIST_BIT                       0x00020000
#define GL_TEXTURE_BIT                    0x00040000
#define GL_SCISSOR_BIT                    0x00080000
#define GL_DELTA_BUFFER_BIT_IMG           0x00100000
#define GL_ALL_ATTRIB_BITS                0xFFFFFFFF

/* ARB_multisample */
#define GL_MULTISAMPLE_BIT                0x20000000
#define GL_MULTISAMPLE_BIT_ARB            0x20000000

/* ClearBufferMask */
/*      GL_COLOR_BUFFER_BIT */
/*      GL_ACCUM_BUFFER_BIT */
/*      GL_STENCIL_BUFFER_BIT */
/*      GL_DEPTH_BUFFER_BIT */
/*      GL_DELTA_BUFFER_BIT_IMG */

/* ClientAttribMask */
#define GL_CLIENT_PIXEL_STORE_BIT         0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT        0x00000002
#define GL_CLIENT_ALL_ATTRIB_BITS         0xFFFFFFFF

/* Boolean */
#define GL_FALSE                          0
#define GL_TRUE                           1

/* BeginMode */
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                          0x0007
#define GL_QUAD_STRIP                     0x0008
#define GL_POLYGON                        0x0009

/* AccumOp */
#define GL_ACCUM                          0x0100
#define GL_LOAD                           0x0101
#define GL_RETURN                         0x0102
#define GL_MULT                           0x0103
#define GL_ADD                            0x0104

/* AlphaFunction */
#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207

/* BlendingFactorDest */
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
/*      GL_CONSTANT_COLOR_EXT */
/*      GL_ONE_MINUS_CONSTANT_COLOR_EXT */
/*      GL_CONSTANT_ALPHA_EXT */
/*      GL_ONE_MINUS_CONSTANT_ALPHA_EXT */

/* BlendingFactorSrc */
/*      GL_ZERO */
/*      GL_ONE */
#define GL_DST_COLOR                      0x0306
#define GL_ONE_MINUS_DST_COLOR            0x0307
#define GL_SRC_ALPHA_SATURATE             0x0308
/*      GL_SRC_ALPHA */
/*      GL_ONE_MINUS_SRC_ALPHA */
/*      GL_DST_ALPHA */
/*      GL_ONE_MINUS_DST_ALPHA */
/*      GL_CONSTANT_COLOR_EXT */
/*      GL_ONE_MINUS_CONSTANT_COLOR_EXT */
/*      GL_CONSTANT_ALPHA_EXT */
/*      GL_ONE_MINUS_CONSTANT_ALPHA_EXT */

/* BlendEquationModeEXT */
/*      GL_LOGIC_OP */
/*      GL_FUNC_ADD_EXT */
/*      GL_MIN_EXT */
/*      GL_MAX_EXT */
/*      GL_FUNC_SUBTRACT_EXT */
/*      GL_FUNC_REVERSE_SUBTRACT_EXT */
/*      GL_ALPHA_MIN_SGIX */
/*      GL_ALPHA_MAX_SGIX */

/* ColorMaterialFace */
/*      GL_FRONT */
/*      GL_BACK */
/*      GL_FRONT_AND_BACK */

/* ColorMaterialParameter */
/*      GL_AMBIENT */
/*      GL_DIFFUSE */
/*      GL_SPECULAR */
/*      GL_EMISSION */
/*      GL_AMBIENT_AND_DIFFUSE */

/* ColorPointerType */
/*      GL_BYTE */
/*      GL_UNSIGNED_BYTE */
/*      GL_SHORT */
/*      GL_UNSIGNED_SHORT */
/*      GL_INT */
/*      GL_UNSIGNED_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* ColorTableParameterPNameSGI */
/*      GL_COLOR_TABLE_SCALE_SGI */
/*      GL_COLOR_TABLE_BIAS_SGI */

/* ColorTableTargetSGI */
/*      GL_COLOR_TABLE_SGI */
/*      GL_POST_CONVOLUTION_COLOR_TABLE_SGI */
/*      GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI */
/*      GL_PROXY_COLOR_TABLE_SGI */
/*      GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI */
/*      GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI */
/*      GL_TEXTURE_COLOR_TABLE_SGI */
/*      GL_PROXY_TEXTURE_COLOR_TABLE_SGI */

/* ConvolutionBorderModeEXT */
/*      GL_REDUCE_EXT */

/* ConvolutionParameterEXT */
/*      GL_CONVOLUTION_BORDER_MODE_EXT */
/*      GL_CONVOLUTION_FILTER_SCALE_EXT */
/*      GL_CONVOLUTION_FILTER_BIAS_EXT */

/* ConvolutionTargetEXT */
/*      GL_CONVOLUTION_1D_EXT */
/*      GL_CONVOLUTION_2D_EXT */

/* CullFaceMode */
/*      GL_FRONT */
/*      GL_BACK */
/*      GL_FRONT_AND_BACK */

/* DepthFunction */
/*      GL_NEVER */
/*      GL_LESS */
/*      GL_EQUAL */
/*      GL_LEQUAL */
/*      GL_GREATER */
/*      GL_NOTEQUAL */
/*      GL_GEQUAL */
/*      GL_ALWAYS */

/* DrawBufferMode */
#define GL_NONE                           0
#define GL_FRONT_LEFT                     0x0400
#define GL_FRONT_RIGHT                    0x0401
#define GL_BACK_LEFT                      0x0402
#define GL_BACK_RIGHT                     0x0403
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_LEFT                           0x0406
#define GL_RIGHT                          0x0407
#define GL_FRONT_AND_BACK                 0x0408
#define GL_AUX0                           0x0409
#define GL_AUX1                           0x040A
#define GL_AUX2                           0x040B
#define GL_AUX3                           0x040C

/* EnableCap */
/*      GL_FOG */
/*      GL_LIGHTING */
/*      GL_TEXTURE_1D */
/*      GL_TEXTURE_2D */
/*      GL_LINE_STIPPLE */
/*      GL_POLYGON_STIPPLE */
/*      GL_CULL_FACE */
/*      GL_ALPHA_TEST */
/*      GL_BLEND */
/*      GL_INDEX_LOGIC_OP */
/*      GL_COLOR_LOGIC_OP */
/*      GL_DITHER */
/*      GL_STENCIL_TEST */
/*      GL_DEPTH_TEST */
/*      GL_CLIP_PLANE0 */
/*      GL_CLIP_PLANE1 */
/*      GL_CLIP_PLANE2 */
/*      GL_CLIP_PLANE3 */
/*      GL_CLIP_PLANE4 */
/*      GL_CLIP_PLANE5 */
/*      GL_LIGHT0 */
/*      GL_LIGHT1 */
/*      GL_LIGHT2 */
/*      GL_LIGHT3 */
/*      GL_LIGHT4 */
/*      GL_LIGHT5 */
/*      GL_LIGHT6 */
/*      GL_LIGHT7 */
/*      GL_TEXTURE_GEN_S */
/*      GL_TEXTURE_GEN_T */
/*      GL_TEXTURE_GEN_R */
/*      GL_TEXTURE_GEN_Q */
/*      GL_MAP1_VERTEX_3 */
/*      GL_MAP1_VERTEX_4 */
/*      GL_MAP1_COLOR_4 */
/*      GL_MAP1_INDEX */
/*      GL_MAP1_NORMAL */
/*      GL_MAP1_TEXTURE_COORD_1 */
/*      GL_MAP1_TEXTURE_COORD_2 */
/*      GL_MAP1_TEXTURE_COORD_3 */
/*      GL_MAP1_TEXTURE_COORD_4 */
/*      GL_MAP2_VERTEX_3 */
/*      GL_MAP2_VERTEX_4 */
/*      GL_MAP2_COLOR_4 */
/*      GL_MAP2_INDEX */
/*      GL_MAP2_NORMAL */
/*      GL_MAP2_TEXTURE_COORD_1 */
/*      GL_MAP2_TEXTURE_COORD_2 */
/*      GL_MAP2_TEXTURE_COORD_3 */
/*      GL_MAP2_TEXTURE_COORD_4 */
/*      GL_POINT_SMOOTH */
/*      GL_LINE_SMOOTH */
/*      GL_POLYGON_SMOOTH */
/*      GL_SCISSOR_TEST */
/*      GL_COLOR_MATERIAL */
/*      GL_NORMALIZE */
/*      GL_AUTO_NORMAL */
/*      GL_POLYGON_OFFSET_POINT */
/*      GL_POLYGON_OFFSET_LINE */
/*      GL_POLYGON_OFFSET_FILL */
/*      GL_VERTEX_ARRAY */
/*      GL_NORMAL_ARRAY */
/*      GL_COLOR_ARRAY */
/*      GL_INDEX_ARRAY */
/*      GL_TEXTURE_COORD_ARRAY */
/*      GL_EDGE_FLAG_ARRAY */
/*      GL_CONVOLUTION_1D_EXT */
/*      GL_CONVOLUTION_2D_EXT */
/*      GL_SEPARABLE_2D_EXT */
/*      GL_HISTOGRAM_EXT */
/*      GL_MINMAX_EXT */
/*      GL_RESCALE_NORMAL_EXT */
/*      GL_SHARED_TEXTURE_PALETTE_EXT */
/*      GL_TEXTURE_3D_EXT */
/*      GL_MULTISAMPLE_SGIS */
/*      GL_SAMPLE_ALPHA_TO_MASK_SGIS */
/*      GL_SAMPLE_ALPHA_TO_ONE_SGIS */
/*      GL_SAMPLE_MASK_SGIS */
/*      GL_TEXTURE_4D_SGIS */
/*      GL_ASYNC_HISTOGRAM_SGIX */
/*      GL_ASYNC_TEX_IMAGE_SGIX */
/*      GL_ASYNC_DRAW_PIXELS_SGIX */
/*      GL_ASYNC_READ_PIXELS_SGIX */
/*      GL_CALLIGRAPHIC_FRAGMENT_SGIX */
/*      GL_FOG_OFFSET_SGIX */
/*      GL_FRAGMENT_LIGHTING_SGIX */
/*      GL_FRAGMENT_COLOR_MATERIAL_SGIX */
/*      GL_FRAGMENT_LIGHT0_SGIX */
/*      GL_FRAGMENT_LIGHT1_SGIX */
/*      GL_FRAGMENT_LIGHT2_SGIX */
/*      GL_FRAGMENT_LIGHT3_SGIX */
/*      GL_FRAGMENT_LIGHT4_SGIX */
/*      GL_FRAGMENT_LIGHT5_SGIX */
/*      GL_FRAGMENT_LIGHT6_SGIX */
/*      GL_FRAGMENT_LIGHT7_SGIX */
/*      GL_FRAMEZOOM_SGIX */
/*      GL_INTERLACE_SGIX */
/*      GL_IR_INSTRUMENT1_SGIX */
/*      GL_PIXEL_TEX_GEN_SGIX */
/*      GL_PIXEL_TEXTURE_SGIS */
/*      GL_REFERENCE_PLANE_SGIX */
/*      GL_SPRITE_SGIX */
/*      GL_COLOR_TABLE_SGI */
/*      GL_POST_CONVOLUTION_COLOR_TABLE_SGI */
/*      GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI */
/*      GL_TEXTURE_COLOR_TABLE_SGI */

/* ErrorCode */
#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505
/*      GL_TABLE_TOO_LARGE_EXT */
/*      GL_TEXTURE_TOO_LARGE_EXT */

/* EXT_framebuffer_object */
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506

/* FeedbackType */
#define GL_2D                             0x0600
#define GL_3D                             0x0601
#define GL_3D_COLOR                       0x0602
#define GL_3D_COLOR_TEXTURE               0x0603
#define GL_4D_COLOR_TEXTURE               0x0604

/* FeedBackToken */
#define GL_PASS_THROUGH_TOKEN             0x0700
#define GL_POINT_TOKEN                    0x0701
#define GL_LINE_TOKEN                     0x0702
#define GL_POLYGON_TOKEN                  0x0703
#define GL_BITMAP_TOKEN                   0x0704
#define GL_DRAW_PIXEL_TOKEN               0x0705
#define GL_COPY_PIXEL_TOKEN               0x0706
#define GL_LINE_RESET_TOKEN               0x0707

/* FfdMaskSGIX */
#define GL_TEXTURE_DEFORMATION_BIT_SGIX   0x00000001
#define GL_GEOMETRY_DEFORMATION_BIT_SGIX  0x00000002

/* FfdTargetSGIX */
/*      GL_GEOMETRY_DEFORMATION_SGIX */
/*      GL_TEXTURE_DEFORMATION_SGIX */

/* FogMode */
/*      GL_LINEAR */
#define GL_EXP                            0x0800
#define GL_EXP2                           0x0801
/*      GL_FOG_FUNC_SGIS */

/* FogParameter */
/*      GL_FOG_COLOR */
/*      GL_FOG_DENSITY */
/*      GL_FOG_END */
/*      GL_FOG_INDEX */
/*      GL_FOG_MODE */
/*      GL_FOG_START */
/*      GL_FOG_OFFSET_VALUE_SGIX */

/* FragmentLightModelParameterSGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_LOCAL_VIEWER_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_TWO_SIDE_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_NORMAL_INTERPOLATION_SGIX */

/* FrontFaceDirection */
#define GL_CW                             0x0900
#define GL_CCW                            0x0901

/* GetColorTableParameterPNameSGI */
/*      GL_COLOR_TABLE_SCALE_SGI */
/*      GL_COLOR_TABLE_BIAS_SGI */
/*      GL_COLOR_TABLE_FORMAT_SGI */
/*      GL_COLOR_TABLE_WIDTH_SGI */
/*      GL_COLOR_TABLE_RED_SIZE_SGI */
/*      GL_COLOR_TABLE_GREEN_SIZE_SGI */
/*      GL_COLOR_TABLE_BLUE_SIZE_SGI */
/*      GL_COLOR_TABLE_ALPHA_SIZE_SGI */
/*      GL_COLOR_TABLE_LUMINANCE_SIZE_SGI */
/*      GL_COLOR_TABLE_INTENSITY_SIZE_SGI */

/* GetConvolutionParameter */
/*      GL_CONVOLUTION_BORDER_MODE_EXT */
/*      GL_CONVOLUTION_FILTER_SCALE_EXT */
/*      GL_CONVOLUTION_FILTER_BIAS_EXT */
/*      GL_CONVOLUTION_FORMAT_EXT */
/*      GL_CONVOLUTION_WIDTH_EXT */
/*      GL_CONVOLUTION_HEIGHT_EXT */
/*      GL_MAX_CONVOLUTION_WIDTH_EXT */
/*      GL_MAX_CONVOLUTION_HEIGHT_EXT */

/* GetHistogramParameterPNameEXT */
/*      GL_HISTOGRAM_WIDTH_EXT */
/*      GL_HISTOGRAM_FORMAT_EXT */
/*      GL_HISTOGRAM_RED_SIZE_EXT */
/*      GL_HISTOGRAM_GREEN_SIZE_EXT */
/*      GL_HISTOGRAM_BLUE_SIZE_EXT */
/*      GL_HISTOGRAM_ALPHA_SIZE_EXT */
/*      GL_HISTOGRAM_LUMINANCE_SIZE_EXT */
/*      GL_HISTOGRAM_SINK_EXT */

/* GetMapQuery */
#define GL_COEFF                          0x0A00
#define GL_ORDER                          0x0A01
#define GL_DOMAIN                         0x0A02

/* GetMinmaxParameterPNameEXT */
/*      GL_MINMAX_FORMAT_EXT */
/*      GL_MINMAX_SINK_EXT */

/* GetPixelMap */
#define GL_PIXEL_MAP_I_TO_I               0x0C70
#define GL_PIXEL_MAP_S_TO_S               0x0C71
#define GL_PIXEL_MAP_I_TO_R               0x0C72
#define GL_PIXEL_MAP_I_TO_G               0x0C73
#define GL_PIXEL_MAP_I_TO_B               0x0C74
#define GL_PIXEL_MAP_I_TO_A               0x0C75
#define GL_PIXEL_MAP_R_TO_R               0x0C76
#define GL_PIXEL_MAP_G_TO_G               0x0C77
#define GL_PIXEL_MAP_B_TO_B               0x0C78
#define GL_PIXEL_MAP_A_TO_A               0x0C79

/* GetPointervPName */
#define GL_VERTEX_ARRAY_POINTER           0x808E
#define GL_NORMAL_ARRAY_POINTER           0x808F
#define GL_COLOR_ARRAY_POINTER            0x8090
#define GL_INDEX_ARRAY_POINTER            0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER    0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER        0x8093
#define GL_FEEDBACK_BUFFER_POINTER        0x0DF0
#define GL_SELECTION_BUFFER_POINTER       0x0DF3
/*      GL_INSTRUMENT_BUFFER_POINTER_SGIX */

/* GetPName */
#define GL_CURRENT_COLOR                  0x0B00
#define GL_CURRENT_INDEX                  0x0B01
#define GL_CURRENT_NORMAL                 0x0B02
#define GL_CURRENT_TEXTURE_COORDS         0x0B03
#define GL_CURRENT_RASTER_COLOR           0x0B04
#define GL_CURRENT_RASTER_INDEX           0x0B05
#define GL_CURRENT_RASTER_TEXTURE_COORDS  0x0B06
#define GL_CURRENT_RASTER_POSITION        0x0B07
#define GL_CURRENT_RASTER_POSITION_VALID  0x0B08
#define GL_CURRENT_RASTER_DISTANCE        0x0B09
#define GL_POINT_SMOOTH                   0x0B10
#define GL_POINT_SIZE                     0x0B11
#define GL_POINT_SIZE_RANGE               0x0B12
#define GL_POINT_SIZE_GRANULARITY         0x0B13
#define GL_LINE_SMOOTH                    0x0B20
#define GL_LINE_WIDTH                     0x0B21
#define GL_LINE_WIDTH_RANGE               0x0B22
#define GL_LINE_WIDTH_GRANULARITY         0x0B23
#define GL_LINE_STIPPLE                   0x0B24
#define GL_LINE_STIPPLE_PATTERN           0x0B25
#define GL_LINE_STIPPLE_REPEAT            0x0B26
/*      GL_SMOOTH_POINT_SIZE_RANGE */
/*      GL_SMOOTH_POINT_SIZE_GRANULARITY */
/*      GL_SMOOTH_LINE_WIDTH_RANGE */
/*      GL_SMOOTH_LINE_WIDTH_GRANULARITY */
/*      GL_ALIASED_POINT_SIZE_RANGE */
/*      GL_ALIASED_LINE_WIDTH_RANGE */
#define GL_LIST_MODE                      0x0B30
#define GL_MAX_LIST_NESTING               0x0B31
#define GL_LIST_BASE                      0x0B32
#define GL_LIST_INDEX                     0x0B33
#define GL_POLYGON_MODE                   0x0B40
#define GL_POLYGON_SMOOTH                 0x0B41
#define GL_POLYGON_STIPPLE                0x0B42
#define GL_EDGE_FLAG                      0x0B43
#define GL_CULL_FACE                      0x0B44
#define GL_CULL_FACE_MODE                 0x0B45
#define GL_FRONT_FACE                     0x0B46
#define GL_LIGHTING                       0x0B50
#define GL_LIGHT_MODEL_LOCAL_VIEWER       0x0B51
#define GL_LIGHT_MODEL_TWO_SIDE           0x0B52
#define GL_LIGHT_MODEL_AMBIENT            0x0B53
#define GL_SHADE_MODEL                    0x0B54
#define GL_COLOR_MATERIAL_FACE            0x0B55
#define GL_COLOR_MATERIAL_PARAMETER       0x0B56
#define GL_COLOR_MATERIAL                 0x0B57
#define GL_FOG                            0x0B60
#define GL_FOG_INDEX                      0x0B61
#define GL_FOG_DENSITY                    0x0B62
#define GL_FOG_START                      0x0B63
#define GL_FOG_END                        0x0B64
#define GL_FOG_MODE                       0x0B65
#define GL_FOG_COLOR                      0x0B66
#define GL_DEPTH_RANGE                    0x0B70
#define GL_DEPTH_TEST                     0x0B71
#define GL_DEPTH_WRITEMASK                0x0B72
#define GL_DEPTH_CLEAR_VALUE              0x0B73
#define GL_DEPTH_FUNC                     0x0B74
#define GL_ACCUM_CLEAR_VALUE              0x0B80
#define GL_STENCIL_TEST                   0x0B90
#define GL_STENCIL_CLEAR_VALUE            0x0B91
#define GL_STENCIL_FUNC                   0x0B92
#define GL_STENCIL_VALUE_MASK             0x0B93
#define GL_STENCIL_FAIL                   0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL        0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS        0x0B96
#define GL_STENCIL_REF                    0x0B97
#define GL_STENCIL_WRITEMASK              0x0B98
#define GL_MATRIX_MODE                    0x0BA0
#define GL_NORMALIZE                      0x0BA1
#define GL_VIEWPORT                       0x0BA2
#define GL_MODELVIEW_STACK_DEPTH          0x0BA3
#define GL_PROJECTION_STACK_DEPTH         0x0BA4
#define GL_TEXTURE_STACK_DEPTH            0x0BA5
#define GL_MODELVIEW_MATRIX               0x0BA6
#define GL_PROJECTION_MATRIX              0x0BA7
#define GL_TEXTURE_MATRIX                 0x0BA8
#define GL_ATTRIB_STACK_DEPTH             0x0BB0
#define GL_CLIENT_ATTRIB_STACK_DEPTH      0x0BB1
#define GL_ALPHA_TEST                     0x0BC0
#define GL_ALPHA_TEST_FUNC                0x0BC1
#define GL_ALPHA_TEST_REF                 0x0BC2
#define GL_DITHER                         0x0BD0
#define GL_BLEND_DST                      0x0BE0
#define GL_BLEND_SRC                      0x0BE1
#define GL_BLEND                          0x0BE2
#define GL_LOGIC_OP_MODE                  0x0BF0
#define GL_INDEX_LOGIC_OP                 0x0BF1
#define GL_LOGIC_OP                       0x0BF1
#define GL_COLOR_LOGIC_OP                 0x0BF2
#define GL_AUX_BUFFERS                    0x0C00
#define GL_DRAW_BUFFER                    0x0C01
#define GL_READ_BUFFER                    0x0C02
#define GL_SCISSOR_BOX                    0x0C10
#define GL_SCISSOR_TEST                   0x0C11
#define GL_INDEX_CLEAR_VALUE              0x0C20
#define GL_INDEX_WRITEMASK                0x0C21
#define GL_COLOR_CLEAR_VALUE              0x0C22
#define GL_COLOR_WRITEMASK                0x0C23
#define GL_INDEX_MODE                     0x0C30
#define GL_RGBA_MODE                      0x0C31
#define GL_DOUBLEBUFFER                   0x0C32
#define GL_STEREO                         0x0C33
#define GL_RENDER_MODE                    0x0C40
#define GL_PERSPECTIVE_CORRECTION_HINT    0x0C50
#define GL_POINT_SMOOTH_HINT              0x0C51
#define GL_LINE_SMOOTH_HINT               0x0C52
#define GL_POLYGON_SMOOTH_HINT            0x0C53
#define GL_FOG_HINT                       0x0C54
#define GL_TEXTURE_GEN_S                  0x0C60
#define GL_TEXTURE_GEN_T                  0x0C61
#define GL_TEXTURE_GEN_R                  0x0C62
#define GL_TEXTURE_GEN_Q                  0x0C63
#define GL_PIXEL_MAP_I_TO_I_SIZE          0x0CB0
#define GL_PIXEL_MAP_S_TO_S_SIZE          0x0CB1
#define GL_PIXEL_MAP_I_TO_R_SIZE          0x0CB2
#define GL_PIXEL_MAP_I_TO_G_SIZE          0x0CB3
#define GL_PIXEL_MAP_I_TO_B_SIZE          0x0CB4
#define GL_PIXEL_MAP_I_TO_A_SIZE          0x0CB5
#define GL_PIXEL_MAP_R_TO_R_SIZE          0x0CB6
#define GL_PIXEL_MAP_G_TO_G_SIZE          0x0CB7
#define GL_PIXEL_MAP_B_TO_B_SIZE          0x0CB8
#define GL_PIXEL_MAP_A_TO_A_SIZE          0x0CB9
#define GL_UNPACK_SWAP_BYTES              0x0CF0
#define GL_UNPACK_LSB_FIRST               0x0CF1
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_UNPACK_SKIP_ROWS               0x0CF3
#define GL_UNPACK_SKIP_PIXELS             0x0CF4
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_PACK_SWAP_BYTES                0x0D00
#define GL_PACK_LSB_FIRST                 0x0D01
#define GL_PACK_ROW_LENGTH                0x0D02
#define GL_PACK_SKIP_ROWS                 0x0D03
#define GL_PACK_SKIP_PIXELS               0x0D04
#define GL_PACK_ALIGNMENT                 0x0D05
#define GL_MAP_COLOR                      0x0D10
#define GL_MAP_STENCIL                    0x0D11
#define GL_INDEX_SHIFT                    0x0D12
#define GL_INDEX_OFFSET                   0x0D13
#define GL_RED_SCALE                      0x0D14
#define GL_RED_BIAS                       0x0D15
#define GL_ZOOM_X                         0x0D16
#define GL_ZOOM_Y                         0x0D17
#define GL_GREEN_SCALE                    0x0D18
#define GL_GREEN_BIAS                     0x0D19
#define GL_BLUE_SCALE                     0x0D1A
#define GL_BLUE_BIAS                      0x0D1B
#define GL_ALPHA_SCALE                    0x0D1C
#define GL_ALPHA_BIAS                     0x0D1D
#define GL_DEPTH_SCALE                    0x0D1E
#define GL_DEPTH_BIAS                     0x0D1F
#define GL_MAX_EVAL_ORDER                 0x0D30
#define GL_MAX_LIGHTS                     0x0D31
#define GL_MAX_CLIP_PLANES                0x0D32
#define GL_MAX_TEXTURE_SIZE               0x0D33
#define GL_MAX_PIXEL_MAP_TABLE            0x0D34
#define GL_MAX_ATTRIB_STACK_DEPTH         0x0D35
#define GL_MAX_MODELVIEW_STACK_DEPTH      0x0D36
#define GL_MAX_NAME_STACK_DEPTH           0x0D37
#define GL_MAX_PROJECTION_STACK_DEPTH     0x0D38
#define GL_MAX_TEXTURE_STACK_DEPTH        0x0D39
#define GL_MAX_VIEWPORT_DIMS              0x0D3A
#define GL_MAX_CLIENT_ATTRIB_STACK_DEPTH  0x0D3B
#define GL_SUBPIXEL_BITS                  0x0D50
#define GL_INDEX_BITS                     0x0D51
#define GL_RED_BITS                       0x0D52
#define GL_GREEN_BITS                     0x0D53
#define GL_BLUE_BITS                      0x0D54
#define GL_ALPHA_BITS                     0x0D55
#define GL_DEPTH_BITS                     0x0D56
#define GL_STENCIL_BITS                   0x0D57
#define GL_ACCUM_RED_BITS                 0x0D58
#define GL_ACCUM_GREEN_BITS               0x0D59
#define GL_ACCUM_BLUE_BITS                0x0D5A
#define GL_ACCUM_ALPHA_BITS               0x0D5B
#define GL_NAME_STACK_DEPTH               0x0D70
#define GL_AUTO_NORMAL                    0x0D80
#define GL_MAP1_COLOR_4                   0x0D90
#define GL_MAP1_INDEX                     0x0D91
#define GL_MAP1_NORMAL                    0x0D92
#define GL_MAP1_TEXTURE_COORD_1           0x0D93
#define GL_MAP1_TEXTURE_COORD_2           0x0D94
#define GL_MAP1_TEXTURE_COORD_3           0x0D95
#define GL_MAP1_TEXTURE_COORD_4           0x0D96
#define GL_MAP1_VERTEX_3                  0x0D97
#define GL_MAP1_VERTEX_4                  0x0D98
#define GL_MAP2_COLOR_4                   0x0DB0
#define GL_MAP2_INDEX                     0x0DB1
#define GL_MAP2_NORMAL                    0x0DB2
#define GL_MAP2_TEXTURE_COORD_1           0x0DB3
#define GL_MAP2_TEXTURE_COORD_2           0x0DB4
#define GL_MAP2_TEXTURE_COORD_3           0x0DB5
#define GL_MAP2_TEXTURE_COORD_4           0x0DB6
#define GL_MAP2_VERTEX_3                  0x0DB7
#define GL_MAP2_VERTEX_4                  0x0DB8
#define GL_MAP1_GRID_DOMAIN               0x0DD0
#define GL_MAP1_GRID_SEGMENTS             0x0DD1
#define GL_MAP2_GRID_DOMAIN               0x0DD2
#define GL_MAP2_GRID_SEGMENTS             0x0DD3
#define GL_TEXTURE_1D                     0x0DE0
#define GL_TEXTURE_2D                     0x0DE1
#define GL_FEEDBACK_BUFFER_SIZE           0x0DF1
#define GL_FEEDBACK_BUFFER_TYPE           0x0DF2
#define GL_SELECTION_BUFFER_SIZE          0x0DF4
#define GL_POLYGON_OFFSET_UNITS           0x2A00
#define GL_POLYGON_OFFSET_POINT           0x2A01
#define GL_POLYGON_OFFSET_LINE            0x2A02
#define GL_POLYGON_OFFSET_FILL            0x8037
#define GL_POLYGON_OFFSET_FACTOR          0x8038
#define GL_TEXTURE_BINDING_1D             0x8068
#define GL_TEXTURE_BINDING_2D             0x8069
#define GL_TEXTURE_BINDING_3D             0x806A
#define GL_VERTEX_ARRAY                   0x8074
#define GL_NORMAL_ARRAY                   0x8075
#define GL_COLOR_ARRAY                    0x8076
#define GL_INDEX_ARRAY                    0x8077
#define GL_TEXTURE_COORD_ARRAY            0x8078
#define GL_EDGE_FLAG_ARRAY                0x8079
#define GL_VERTEX_ARRAY_SIZE              0x807A
#define GL_VERTEX_ARRAY_TYPE              0x807B
#define GL_VERTEX_ARRAY_STRIDE            0x807C
#define GL_NORMAL_ARRAY_TYPE              0x807E
#define GL_NORMAL_ARRAY_STRIDE            0x807F
#define GL_COLOR_ARRAY_SIZE               0x8081
#define GL_COLOR_ARRAY_TYPE               0x8082
#define GL_COLOR_ARRAY_STRIDE             0x8083
#define GL_INDEX_ARRAY_TYPE               0x8085
#define GL_INDEX_ARRAY_STRIDE             0x8086
#define GL_TEXTURE_COORD_ARRAY_SIZE       0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE       0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE     0x808A
#define GL_EDGE_FLAG_ARRAY_STRIDE         0x808C
/*      GL_CLIP_PLANE0 */
/*      GL_CLIP_PLANE1 */
/*      GL_CLIP_PLANE2 */
/*      GL_CLIP_PLANE3 */
/*      GL_CLIP_PLANE4 */
/*      GL_CLIP_PLANE5 */
/*      GL_LIGHT0 */
/*      GL_LIGHT1 */
/*      GL_LIGHT2 */
/*      GL_LIGHT3 */
/*      GL_LIGHT4 */
/*      GL_LIGHT5 */
/*      GL_LIGHT6 */
/*      GL_LIGHT7 */
/*      GL_LIGHT_MODEL_COLOR_CONTROL */
/*      GL_BLEND_COLOR_EXT */
/*      GL_BLEND_EQUATION_EXT */
/*      GL_PACK_CMYK_HINT_EXT */
/*      GL_UNPACK_CMYK_HINT_EXT */
/*      GL_CONVOLUTION_1D_EXT */
/*      GL_CONVOLUTION_2D_EXT */
/*      GL_SEPARABLE_2D_EXT */
/*      GL_POST_CONVOLUTION_RED_SCALE_EXT */
/*      GL_POST_CONVOLUTION_GREEN_SCALE_EXT */
/*      GL_POST_CONVOLUTION_BLUE_SCALE_EXT */
/*      GL_POST_CONVOLUTION_ALPHA_SCALE_EXT */
/*      GL_POST_CONVOLUTION_RED_BIAS_EXT */
/*      GL_POST_CONVOLUTION_GREEN_BIAS_EXT */
/*      GL_POST_CONVOLUTION_BLUE_BIAS_EXT */
/*      GL_POST_CONVOLUTION_ALPHA_BIAS_EXT */
/*      GL_HISTOGRAM_EXT */
/*      GL_MINMAX_EXT */
/*      GL_POLYGON_OFFSET_BIAS_EXT */
/*      GL_RESCALE_NORMAL_EXT */
/*      GL_SHARED_TEXTURE_PALETTE_EXT */
/*      GL_TEXTURE_3D_BINDING_EXT */
/*      GL_PACK_SKIP_IMAGES_EXT */
/*      GL_PACK_IMAGE_HEIGHT_EXT */
/*      GL_UNPACK_SKIP_IMAGES_EXT */
/*      GL_UNPACK_IMAGE_HEIGHT_EXT */
/*      GL_TEXTURE_3D_EXT */
/*      GL_MAX_3D_TEXTURE_SIZE_EXT */
/*      GL_VERTEX_ARRAY_COUNT_EXT */
/*      GL_NORMAL_ARRAY_COUNT_EXT */
/*      GL_COLOR_ARRAY_COUNT_EXT */
/*      GL_INDEX_ARRAY_COUNT_EXT */
/*      GL_TEXTURE_COORD_ARRAY_COUNT_EXT */
/*      GL_EDGE_FLAG_ARRAY_COUNT_EXT */
/*      GL_DETAIL_TEXTURE_2D_BINDING_SGIS */
/*      GL_FOG_FUNC_POINTS_SGIS */
/*      GL_MAX_FOG_FUNC_POINTS_SGIS */
/*      GL_GENERATE_MIPMAP_HINT_SGIS */
/*      GL_MULTISAMPLE_SGIS */
/*      GL_SAMPLE_ALPHA_TO_MASK_SGIS */
/*      GL_SAMPLE_ALPHA_TO_ONE_SGIS */
/*      GL_SAMPLE_MASK_SGIS */
/*      GL_SAMPLE_BUFFERS_SGIS */
/*      GL_SAMPLES_SGIS */
/*      GL_SAMPLE_MASK_VALUE_SGIS */
/*      GL_SAMPLE_MASK_INVERT_SGIS */
/*      GL_SAMPLE_PATTERN_SGIS */
/*      GL_PIXEL_TEXTURE_SGIS */
/*      GL_POINT_SIZE_MIN_SGIS */
/*      GL_POINT_SIZE_MAX_SGIS */
/*      GL_POINT_FADE_THRESHOLD_SIZE_SGIS */
/*      GL_DISTANCE_ATTENUATION_SGIS */
/*      GL_PACK_SKIP_VOLUMES_SGIS */
/*      GL_PACK_IMAGE_DEPTH_SGIS */
/*      GL_UNPACK_SKIP_VOLUMES_SGIS */
/*      GL_UNPACK_IMAGE_DEPTH_SGIS */
/*      GL_TEXTURE_4D_SGIS */
/*      GL_MAX_4D_TEXTURE_SIZE_SGIS */
/*      GL_TEXTURE_4D_BINDING_SGIS */
/*      GL_ASYNC_MARKER_SGIX */
/*      GL_ASYNC_HISTOGRAM_SGIX */
/*      GL_MAX_ASYNC_HISTOGRAM_SGIX */
/*      GL_ASYNC_TEX_IMAGE_SGIX */
/*      GL_ASYNC_DRAW_PIXELS_SGIX */
/*      GL_ASYNC_READ_PIXELS_SGIX */
/*      GL_MAX_ASYNC_TEX_IMAGE_SGIX */
/*      GL_MAX_ASYNC_DRAW_PIXELS_SGIX */
/*      GL_MAX_ASYNC_READ_PIXELS_SGIX */
/*      GL_CALLIGRAPHIC_FRAGMENT_SGIX */
/*      GL_MAX_CLIPMAP_VIRTUAL_DEPTH_SGIX */
/*      GL_MAX_CLIPMAP_DEPTH_SGIX */
/*      GL_CONVOLUTION_HINT_SGIX */
/*      GL_FOG_OFFSET_SGIX */
/*      GL_FOG_OFFSET_VALUE_SGIX */
/*      GL_FRAGMENT_LIGHTING_SGIX */
/*      GL_FRAGMENT_COLOR_MATERIAL_SGIX */
/*      GL_FRAGMENT_COLOR_MATERIAL_FACE_SGIX */
/*      GL_FRAGMENT_COLOR_MATERIAL_PARAMETER_SGIX */
/*      GL_MAX_FRAGMENT_LIGHTS_SGIX */
/*      GL_MAX_ACTIVE_LIGHTS_SGIX */
/*      GL_LIGHT_ENV_MODE_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_LOCAL_VIEWER_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_TWO_SIDE_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX */
/*      GL_FRAGMENT_LIGHT_MODEL_NORMAL_INTERPOLATION_SGIX */
/*      GL_FRAGMENT_LIGHT0_SGIX */
/*      GL_FRAMEZOOM_SGIX */
/*      GL_FRAMEZOOM_FACTOR_SGIX */
/*      GL_MAX_FRAMEZOOM_FACTOR_SGIX */
/*      GL_INSTRUMENT_MEASUREMENTS_SGIX */
/*      GL_INTERLACE_SGIX */
/*      GL_IR_INSTRUMENT1_SGIX */
/*      GL_PIXEL_TEX_GEN_SGIX */
/*      GL_PIXEL_TEX_GEN_MODE_SGIX */
/*      GL_PIXEL_TILE_BEST_ALIGNMENT_SGIX */
/*      GL_PIXEL_TILE_CACHE_INCREMENT_SGIX */
/*      GL_PIXEL_TILE_WIDTH_SGIX */
/*      GL_PIXEL_TILE_HEIGHT_SGIX */
/*      GL_PIXEL_TILE_GRID_WIDTH_SGIX */
/*      GL_PIXEL_TILE_GRID_HEIGHT_SGIX */
/*      GL_PIXEL_TILE_GRID_DEPTH_SGIX */
/*      GL_PIXEL_TILE_CACHE_SIZE_SGIX */
/*      GL_DEFORMATIONS_MASK_SGIX */
/*      GL_REFERENCE_PLANE_EQUATION_SGIX */
/*      GL_REFERENCE_PLANE_SGIX */
/*      GL_SPRITE_SGIX */
/*      GL_SPRITE_MODE_SGIX */
/*      GL_SPRITE_AXIS_SGIX */
/*      GL_SPRITE_TRANSLATION_SGIX */
/*      GL_PACK_SUBSAMPLE_RATE_SGIX */
/*      GL_UNPACK_SUBSAMPLE_RATE_SGIX */
/*      GL_PACK_RESAMPLE_SGIX */
/*      GL_UNPACK_RESAMPLE_SGIX */
/*      GL_POST_TEXTURE_FILTER_BIAS_RANGE_SGIX */
/*      GL_POST_TEXTURE_FILTER_SCALE_RANGE_SGIX */
/*      GL_VERTEX_PRECLIP_SGIX */
/*      GL_VERTEX_PRECLIP_HINT_SGIX */
/*      GL_COLOR_MATRIX_SGI */
/*      GL_COLOR_MATRIX_STACK_DEPTH_SGI */
/*      GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI */
/*      GL_POST_COLOR_MATRIX_RED_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_RED_BIAS_SGI */
/*      GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI */
/*      GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI */
/*      GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI */
/*      GL_COLOR_TABLE_SGI */
/*      GL_POST_CONVOLUTION_COLOR_TABLE_SGI */
/*      GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI */
/*      GL_TEXTURE_COLOR_TABLE_SGI */

/* GetTextureParameter */
/*      GL_TEXTURE_MAG_FILTER */
/*      GL_TEXTURE_MIN_FILTER */
/*      GL_TEXTURE_WRAP_S */
/*      GL_TEXTURE_WRAP_T */
#define GL_TEXTURE_WIDTH                  0x1000
#define GL_TEXTURE_HEIGHT                 0x1001
#define GL_TEXTURE_INTERNAL_FORMAT        0x1003
#define GL_TEXTURE_COMPONENTS             0x1003
#define GL_TEXTURE_BORDER_COLOR           0x1004
#define GL_TEXTURE_BORDER                 0x1005
#define GL_TEXTURE_RED_SIZE               0x805C
#define GL_TEXTURE_GREEN_SIZE             0x805D
#define GL_TEXTURE_BLUE_SIZE              0x805E
#define GL_TEXTURE_ALPHA_SIZE             0x805F
#define GL_TEXTURE_LUMINANCE_SIZE         0x8060
#define GL_TEXTURE_INTENSITY_SIZE         0x8061
#define GL_TEXTURE_PRIORITY               0x8066
#define GL_TEXTURE_RESIDENT               0x8067
/*      GL_TEXTURE_DEPTH_EXT */
/*      GL_TEXTURE_WRAP_R_EXT */
/*      GL_DETAIL_TEXTURE_LEVEL_SGIS */
/*      GL_DETAIL_TEXTURE_MODE_SGIS */
/*      GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS */
/*      GL_GENERATE_MIPMAP_SGIS */
/*      GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS */
/*      GL_TEXTURE_FILTER4_SIZE_SGIS */
/*      GL_TEXTURE_MIN_LOD_SGIS */
/*      GL_TEXTURE_MAX_LOD_SGIS */
/*      GL_TEXTURE_BASE_LEVEL_SGIS */
/*      GL_TEXTURE_MAX_LEVEL_SGIS */
/*      GL_DUAL_TEXTURE_SELECT_SGIS */
/*      GL_QUAD_TEXTURE_SELECT_SGIS */
/*      GL_TEXTURE_4DSIZE_SGIS */
/*      GL_TEXTURE_WRAP_Q_SGIS */
/*      GL_TEXTURE_CLIPMAP_CENTER_SGIX */
/*      GL_TEXTURE_CLIPMAP_FRAME_SGIX */
/*      GL_TEXTURE_CLIPMAP_OFFSET_SGIX */
/*      GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX */
/*      GL_TEXTURE_CLIPMAP_LOD_OFFSET_SGIX */
/*      GL_TEXTURE_CLIPMAP_DEPTH_SGIX */
/*      GL_TEXTURE_COMPARE_SGIX */
/*      GL_TEXTURE_COMPARE_OPERATOR_SGIX */
/*      GL_TEXTURE_LEQUAL_R_SGIX */
/*      GL_TEXTURE_GEQUAL_R_SGIX */
/*      GL_SHADOW_AMBIENT_SGIX */
/*      GL_TEXTURE_MAX_CLAMP_S_SGIX */
/*      GL_TEXTURE_MAX_CLAMP_T_SGIX */
/*      GL_TEXTURE_MAX_CLAMP_R_SGIX */
/*      GL_TEXTURE_LOD_BIAS_S_SGIX */
/*      GL_TEXTURE_LOD_BIAS_T_SGIX */
/*      GL_TEXTURE_LOD_BIAS_R_SGIX */
/*      GL_POST_TEXTURE_FILTER_BIAS_SGIX */
/*      GL_POST_TEXTURE_FILTER_SCALE_SGIX */

/* HintMode */
#define GL_DONT_CARE                      0x1100
#define GL_FASTEST                        0x1101
#define GL_NICEST                         0x1102

/* HintTarget */
/*      GL_PERSPECTIVE_CORRECTION_HINT */
/*      GL_POINT_SMOOTH_HINT */
/*      GL_LINE_SMOOTH_HINT */
/*      GL_POLYGON_SMOOTH_HINT */
/*      GL_FOG_HINT */
/*      GL_PACK_CMYK_HINT_EXT */
/*      GL_UNPACK_CMYK_HINT_EXT */
/*      GL_GENERATE_MIPMAP_HINT_SGIS */
/*      GL_CONVOLUTION_HINT_SGIX */
/*      GL_TEXTURE_MULTI_BUFFER_HINT_SGIX */
/*      GL_VERTEX_PRECLIP_HINT_SGIX */

/* HistogramTargetEXT */
/*      GL_HISTOGRAM_EXT */
/*      GL_PROXY_HISTOGRAM_EXT */

/* IndexPointerType */
/*      GL_SHORT */
/*      GL_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* LightEnvModeSGIX */
/*      GL_REPLACE */
/*      GL_MODULATE */
/*      GL_ADD */

/* LightEnvParameterSGIX */
/*      GL_LIGHT_ENV_MODE_SGIX */

/* LightModelColorControl */
/*      GL_SINGLE_COLOR */
/*      GL_SEPARATE_SPECULAR_COLOR */

/* LightModelParameter */
/*      GL_LIGHT_MODEL_AMBIENT */
/*      GL_LIGHT_MODEL_LOCAL_VIEWER */
/*      GL_LIGHT_MODEL_TWO_SIDE */
/*      GL_LIGHT_MODEL_COLOR_CONTROL */

/* LightParameter */
#define GL_AMBIENT                        0x1200
#define GL_DIFFUSE                        0x1201
#define GL_SPECULAR                       0x1202
#define GL_POSITION                       0x1203
#define GL_SPOT_DIRECTION                 0x1204
#define GL_SPOT_EXPONENT                  0x1205
#define GL_SPOT_CUTOFF                    0x1206
#define GL_CONSTANT_ATTENUATION           0x1207
#define GL_LINEAR_ATTENUATION             0x1208
#define GL_QUADRATIC_ATTENUATION          0x1209

/* ListMode */
#define GL_COMPILE                        0x1300
#define GL_COMPILE_AND_EXECUTE            0x1301

/* DataType */
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_2_BYTES                        0x1407
#define GL_3_BYTES                        0x1408
#define GL_4_BYTES                        0x1409
#define GL_DOUBLE                         0x140A
#define GL_DOUBLE_EXT                     0x140A

/* ARB_half_float_pixel */
#define GL_HALF_FLOAT_ARB                 0x140B
#define GL_HALF_FLOAT_NV                  0x140B

/* ListNameType */
/*      GL_BYTE */
/*      GL_UNSIGNED_BYTE */
/*      GL_SHORT */
/*      GL_UNSIGNED_SHORT */
/*      GL_INT */
/*      GL_UNSIGNED_INT */
/*      GL_FLOAT */
/*      GL_2_BYTES */
/*      GL_3_BYTES */
/*      GL_4_BYTES */

/* ListParameterName */
/*      GL_LIST_PRIORITY_SGIX */

/* LogicOp */
#define GL_CLEAR                          0x1500
#define GL_AND                            0x1501
#define GL_AND_REVERSE                    0x1502
#define GL_COPY                           0x1503
#define GL_AND_INVERTED                   0x1504
#define GL_NOOP                           0x1505
#define GL_XOR                            0x1506
#define GL_OR                             0x1507
#define GL_NOR                            0x1508
#define GL_EQUIV                          0x1509
#define GL_INVERT                         0x150A
#define GL_OR_REVERSE                     0x150B
#define GL_COPY_INVERTED                  0x150C
#define GL_OR_INVERTED                    0x150D
#define GL_NAND                           0x150E
#define GL_SET                            0x150F

/* MapTarget */
/*      GL_MAP1_COLOR_4 */
/*      GL_MAP1_INDEX */
/*      GL_MAP1_NORMAL */
/*      GL_MAP1_TEXTURE_COORD_1 */
/*      GL_MAP1_TEXTURE_COORD_2 */
/*      GL_MAP1_TEXTURE_COORD_3 */
/*      GL_MAP1_TEXTURE_COORD_4 */
/*      GL_MAP1_VERTEX_3 */
/*      GL_MAP1_VERTEX_4 */
/*      GL_MAP2_COLOR_4 */
/*      GL_MAP2_INDEX */
/*      GL_MAP2_NORMAL */
/*      GL_MAP2_TEXTURE_COORD_1 */
/*      GL_MAP2_TEXTURE_COORD_2 */
/*      GL_MAP2_TEXTURE_COORD_3 */
/*      GL_MAP2_TEXTURE_COORD_4 */
/*      GL_MAP2_VERTEX_3 */
/*      GL_MAP2_VERTEX_4 */
/*      GL_GEOMETRY_DEFORMATION_SGIX */
/*      GL_TEXTURE_DEFORMATION_SGIX */

/* MaterialFace */
/*      GL_FRONT */
/*      GL_BACK */
/*      GL_FRONT_AND_BACK */

/* MaterialParameter */
#define GL_EMISSION                       0x1600
#define GL_SHININESS                      0x1601
#define GL_AMBIENT_AND_DIFFUSE            0x1602
#define GL_COLOR_INDEXES                  0x1603
/*      GL_AMBIENT */
/*      GL_DIFFUSE */
/*      GL_SPECULAR */

/* MatrixMode */
#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701
#define GL_TEXTURE                        0x1702

/* MeshMode1 */
/*      GL_POINT */
/*      GL_LINE */

/* MeshMode2 */
/*      GL_POINT */
/*      GL_LINE */
/*      GL_FILL */

/* MinmaxTargetEXT */
/*      GL_MINMAX_EXT */

/* NormalPointerType */
/*      GL_BYTE */
/*      GL_SHORT */
/*      GL_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* PixelCopyType */
#define GL_COLOR                          0x1800
#define GL_DEPTH                          0x1801
#define GL_STENCIL                        0x1802

/* PixelFormat */
#define GL_COLOR_INDEX                    0x1900
#define GL_STENCIL_INDEX                  0x1901
#define GL_DEPTH_COMPONENT                0x1902
#define GL_RED                            0x1903
#define GL_GREEN                          0x1904
#define GL_BLUE                           0x1905
#define GL_ALPHA                          0x1906
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_LUMINANCE                      0x1909
#define GL_LUMINANCE_ALPHA                0x190A
/*      GL_ABGR_EXT */
/*      GL_CMYK_EXT */
/*      GL_CMYKA_EXT */
/*      GL_R5_G6_B5_ICC_SGIX */
/*      GL_R5_G6_B5_A8_ICC_SGIX */
/*      GL_ALPHA16_ICC_SGIX */
/*      GL_LUMINANCE16_ICC_SGIX */
/*      GL_LUMINANCE16_ALPHA8_ICC_SGIX */
/*      GL_YCRCB_422_SGIX */
/*      GL_YCRCB_444_SGIX */

/* PixelMap */
/*      GL_PIXEL_MAP_I_TO_I */
/*      GL_PIXEL_MAP_S_TO_S */
/*      GL_PIXEL_MAP_I_TO_R */
/*      GL_PIXEL_MAP_I_TO_G */
/*      GL_PIXEL_MAP_I_TO_B */
/*      GL_PIXEL_MAP_I_TO_A */
/*      GL_PIXEL_MAP_R_TO_R */
/*      GL_PIXEL_MAP_G_TO_G */
/*      GL_PIXEL_MAP_B_TO_B */
/*      GL_PIXEL_MAP_A_TO_A */

/* PixelStoreParameter */
/*      GL_UNPACK_SWAP_BYTES */
/*      GL_UNPACK_LSB_FIRST */
/*      GL_UNPACK_ROW_LENGTH */
/*      GL_UNPACK_SKIP_ROWS */
/*      GL_UNPACK_SKIP_PIXELS */
/*      GL_UNPACK_ALIGNMENT */
/*      GL_PACK_SWAP_BYTES */
/*      GL_PACK_LSB_FIRST */
/*      GL_PACK_ROW_LENGTH */
/*      GL_PACK_SKIP_ROWS */
/*      GL_PACK_SKIP_PIXELS */
/*      GL_PACK_ALIGNMENT */
/*      GL_PACK_SKIP_IMAGES_EXT */
/*      GL_PACK_IMAGE_HEIGHT_EXT */
/*      GL_UNPACK_SKIP_IMAGES_EXT */
/*      GL_UNPACK_IMAGE_HEIGHT_EXT */
/*      GL_PACK_SKIP_VOLUMES_SGIS */
/*      GL_PACK_IMAGE_DEPTH_SGIS */
/*      GL_UNPACK_SKIP_VOLUMES_SGIS */
/*      GL_UNPACK_IMAGE_DEPTH_SGIS */
/*      GL_PIXEL_TILE_WIDTH_SGIX */
/*      GL_PIXEL_TILE_HEIGHT_SGIX */
/*      GL_PIXEL_TILE_GRID_WIDTH_SGIX */
/*      GL_PIXEL_TILE_GRID_HEIGHT_SGIX */
/*      GL_PIXEL_TILE_GRID_DEPTH_SGIX */
/*      GL_PIXEL_TILE_CACHE_SIZE_SGIX */
/*      GL_PACK_SUBSAMPLE_RATE_SGIX */
/*      GL_UNPACK_SUBSAMPLE_RATE_SGIX */
/*      GL_PACK_RESAMPLE_SGIX */
/*      GL_UNPACK_RESAMPLE_SGIX */

/* PixelStoreResampleMode */
/*      GL_RESAMPLE_REPLICATE_SGIX */
/*      GL_RESAMPLE_ZERO_FILL_SGIX */
/*      GL_RESAMPLE_DECIMATE_SGIX */

/* PixelStoreSubsampleRate */
/*      GL_PIXEL_SUBSAMPLE_4444_SGIX */
/*      GL_PIXEL_SUBSAMPLE_2424_SGIX */
/*      GL_PIXEL_SUBSAMPLE_4242_SGIX */

/* PixelTexGenMode */
/*      GL_NONE */
/*      GL_RGB */
/*      GL_RGBA */
/*      GL_LUMINANCE */
/*      GL_LUMINANCE_ALPHA */
/*      GL_PIXEL_TEX_GEN_ALPHA_REPLACE_SGIX */
/*      GL_PIXEL_TEX_GEN_ALPHA_NO_REPLACE_SGIX */
/*      GL_PIXEL_TEX_GEN_ALPHA_MS_SGIX */
/*      GL_PIXEL_TEX_GEN_ALPHA_LS_SGIX */

/* PixelTexGenParameterNameSGIS */
/*      GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS */
/*      GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS */

/* PixelTransferParameter */
/*      GL_MAP_COLOR */
/*      GL_MAP_STENCIL */
/*      GL_INDEX_SHIFT */
/*      GL_INDEX_OFFSET */
/*      GL_RED_SCALE */
/*      GL_RED_BIAS */
/*      GL_GREEN_SCALE */
/*      GL_GREEN_BIAS */
/*      GL_BLUE_SCALE */
/*      GL_BLUE_BIAS */
/*      GL_ALPHA_SCALE */
/*      GL_ALPHA_BIAS */
/*      GL_DEPTH_SCALE */
/*      GL_DEPTH_BIAS */
/*      GL_POST_CONVOLUTION_RED_SCALE_EXT */
/*      GL_POST_CONVOLUTION_GREEN_SCALE_EXT */
/*      GL_POST_CONVOLUTION_BLUE_SCALE_EXT */
/*      GL_POST_CONVOLUTION_ALPHA_SCALE_EXT */
/*      GL_POST_CONVOLUTION_RED_BIAS_EXT */
/*      GL_POST_CONVOLUTION_GREEN_BIAS_EXT */
/*      GL_POST_CONVOLUTION_BLUE_BIAS_EXT */
/*      GL_POST_CONVOLUTION_ALPHA_BIAS_EXT */
/*      GL_POST_COLOR_MATRIX_RED_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI */
/*      GL_POST_COLOR_MATRIX_RED_BIAS_SGI */
/*      GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI */
/*      GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI */
/*      GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI */

/* PixelType */
#define GL_BITMAP                         0x1A00
/*      GL_BYTE */
/*      GL_UNSIGNED_BYTE */
/*      GL_SHORT */
/*      GL_UNSIGNED_SHORT */
/*      GL_INT */
/*      GL_UNSIGNED_INT */
/*      GL_FLOAT */
/*      GL_UNSIGNED_BYTE_3_3_2_EXT */
/*      GL_UNSIGNED_SHORT_4_4_4_4_EXT */
/*      GL_UNSIGNED_SHORT_5_5_5_1_EXT */
/*      GL_UNSIGNED_INT_8_8_8_8_EXT */
/*      GL_UNSIGNED_INT_10_10_10_2_EXT */

/* PointParameterNameSGIS */
/*      GL_POINT_SIZE_MIN_SGIS */
/*      GL_POINT_SIZE_MAX_SGIS */
/*      GL_POINT_FADE_THRESHOLD_SIZE_SGIS */
/*      GL_DISTANCE_ATTENUATION_SGIS */

/* PolygonMode */
#define GL_POINT                          0x1B00
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02

/* ReadBufferMode */
/*      GL_FRONT_LEFT */
/*      GL_FRONT_RIGHT */
/*      GL_BACK_LEFT */
/*      GL_BACK_RIGHT */
/*      GL_FRONT */
/*      GL_BACK */
/*      GL_LEFT */
/*      GL_RIGHT */
/*      GL_AUX0 */
/*      GL_AUX1 */
/*      GL_AUX2 */
/*      GL_AUX3 */

/* RenderingMode */
#define GL_RENDER                         0x1C00
#define GL_FEEDBACK                       0x1C01
#define GL_SELECT                         0x1C02

/* SamplePatternSGIS */
/*      GL_1PASS_SGIS */
/*      GL_2PASS_0_SGIS */
/*      GL_2PASS_1_SGIS */
/*      GL_4PASS_0_SGIS */
/*      GL_4PASS_1_SGIS */
/*      GL_4PASS_2_SGIS */
/*      GL_4PASS_3_SGIS */

/* SeparableTargetEXT */
/*      GL_SEPARABLE_2D_EXT */

/* ShadingModel */
#define GL_FLAT                           0x1D00
#define GL_SMOOTH                         0x1D01

/* StencilFunction */
/*      GL_NEVER */
/*      GL_LESS */
/*      GL_EQUAL */
/*      GL_LEQUAL */
/*      GL_GREATER */
/*      GL_NOTEQUAL */
/*      GL_GEQUAL */
/*      GL_ALWAYS */

/* StencilOp */
/*      GL_ZERO */
#define GL_KEEP                           0x1E00
#define GL_REPLACE                        0x1E01
#define GL_INCR                           0x1E02
#define GL_DECR                           0x1E03
/*      GL_INVERT */

/* StringName */
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02
#define GL_EXTENSIONS                     0x1F03

/* TexCoordPointerType */
/*      GL_SHORT */
/*      GL_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* TextureCoordName */
#define GL_S                              0x2000
#define GL_T                              0x2001
#define GL_R                              0x2002
#define GL_Q                              0x2003

/* TextureEnvMode */
#define GL_MODULATE                       0x2100
#define GL_DECAL                          0x2101
/*      GL_BLEND */
/*      GL_REPLACE_EXT */
/*      GL_ADD */
/*      GL_TEXTURE_ENV_BIAS_SGIX */

/* TextureEnvParameter */
#define GL_TEXTURE_ENV_MODE               0x2200
#define GL_TEXTURE_ENV_COLOR              0x2201

/* TextureEnvTarget */
#define GL_TEXTURE_ENV                    0x2300

/* TextureFilterFuncSGIS */
/*      GL_FILTER4_SGIS */

/* TextureGenMode */
#define GL_EYE_LINEAR                     0x2400
#define GL_OBJECT_LINEAR                  0x2401
#define GL_SPHERE_MAP                     0x2402
/*      GL_EYE_DISTANCE_TO_POINT_SGIS */
/*      GL_OBJECT_DISTANCE_TO_POINT_SGIS */
/*      GL_EYE_DISTANCE_TO_LINE_SGIS */
/*      GL_OBJECT_DISTANCE_TO_LINE_SGIS */

/* TextureGenParameter */
#define GL_TEXTURE_GEN_MODE               0x2500
#define GL_OBJECT_PLANE                   0x2501
#define GL_EYE_PLANE                      0x2502
/*      GL_EYE_POINT_SGIS */
/*      GL_OBJECT_POINT_SGIS */
/*      GL_EYE_LINE_SGIS */
/*      GL_OBJECT_LINE_SGIS */

/* TextureMagFilter */
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
/*      GL_LINEAR_DETAIL_SGIS */
/*      GL_LINEAR_DETAIL_ALPHA_SGIS */
/*      GL_LINEAR_DETAIL_COLOR_SGIS */
/*      GL_LINEAR_SHARPEN_SGIS */
/*      GL_LINEAR_SHARPEN_ALPHA_SGIS */
/*      GL_LINEAR_SHARPEN_COLOR_SGIS */
/*      GL_FILTER4_SGIS */
/*      GL_PIXEL_TEX_GEN_Q_CEILING_SGIX */
/*      GL_PIXEL_TEX_GEN_Q_ROUND_SGIX */
/*      GL_PIXEL_TEX_GEN_Q_FLOOR_SGIX */

/* TextureMinFilter */
/*      GL_NEAREST */
/*      GL_LINEAR */
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
/*      GL_FILTER4_SGIS */
/*      GL_LINEAR_CLIPMAP_LINEAR_SGIX */
/*      GL_NEAREST_CLIPMAP_NEAREST_SGIX */
/*      GL_NEAREST_CLIPMAP_LINEAR_SGIX */
/*      GL_LINEAR_CLIPMAP_NEAREST_SGIX */
/*      GL_PIXEL_TEX_GEN_Q_CEILING_SGIX */
/*      GL_PIXEL_TEX_GEN_Q_ROUND_SGIX */
/*      GL_PIXEL_TEX_GEN_Q_FLOOR_SGIX */

/* TextureParameterName */
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
/*      GL_TEXTURE_BORDER_COLOR */
/*      GL_TEXTURE_PRIORITY */
/*      GL_TEXTURE_WRAP_R_EXT */
/*      GL_DETAIL_TEXTURE_LEVEL_SGIS */
/*      GL_DETAIL_TEXTURE_MODE_SGIS */
/*      GL_GENERATE_MIPMAP_SGIS */
/*      GL_DUAL_TEXTURE_SELECT_SGIS */
/*      GL_QUAD_TEXTURE_SELECT_SGIS */
/*      GL_TEXTURE_WRAP_Q_SGIS */
/*      GL_TEXTURE_CLIPMAP_CENTER_SGIX */
/*      GL_TEXTURE_CLIPMAP_FRAME_SGIX */
/*      GL_TEXTURE_CLIPMAP_OFFSET_SGIX */
/*      GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX */
/*      GL_TEXTURE_CLIPMAP_LOD_OFFSET_SGIX */
/*      GL_TEXTURE_CLIPMAP_DEPTH_SGIX */
/*      GL_TEXTURE_COMPARE_SGIX */
/*      GL_TEXTURE_COMPARE_OPERATOR_SGIX */
/*      GL_SHADOW_AMBIENT_SGIX */
/*      GL_TEXTURE_MAX_CLAMP_S_SGIX */
/*      GL_TEXTURE_MAX_CLAMP_T_SGIX */
/*      GL_TEXTURE_MAX_CLAMP_R_SGIX */
/*      GL_TEXTURE_LOD_BIAS_S_SGIX */
/*      GL_TEXTURE_LOD_BIAS_T_SGIX */
/*      GL_TEXTURE_LOD_BIAS_R_SGIX */
/*      GL_POST_TEXTURE_FILTER_BIAS_SGIX */
/*      GL_POST_TEXTURE_FILTER_SCALE_SGIX */

/* TextureTarget */
/*      GL_TEXTURE_1D */
/*      GL_TEXTURE_2D */
#define GL_PROXY_TEXTURE_1D               0x8063
#define GL_PROXY_TEXTURE_2D               0x8064
/*      GL_TEXTURE_3D_EXT */
/*      GL_PROXY_TEXTURE_3D_EXT */
/*      GL_DETAIL_TEXTURE_2D_SGIS */
/*      GL_TEXTURE_4D_SGIS */
/*      GL_PROXY_TEXTURE_4D_SGIS */
/*      GL_TEXTURE_MIN_LOD_SGIS */
/*      GL_TEXTURE_MAX_LOD_SGIS */
/*      GL_TEXTURE_BASE_LEVEL_SGIS */
/*      GL_TEXTURE_MAX_LEVEL_SGIS */

/* TextureWrapMode */
#define GL_CLAMP                          0x2900
#define GL_REPEAT                         0x2901
/*      GL_CLAMP_TO_BORDER_SGIS */
/*      GL_CLAMP_TO_EDGE_SGIS */

/* PixelInternalFormat */
#define GL_R3_G3_B2                       0x2A10
#define GL_ALPHA4                         0x803B
#define GL_ALPHA8                         0x803C
#define GL_ALPHA12                        0x803D
#define GL_ALPHA16                        0x803E
#define GL_LUMINANCE4                     0x803F
#define GL_LUMINANCE8                     0x8040
#define GL_LUMINANCE12                    0x8041
#define GL_LUMINANCE16                    0x8042
#define GL_LUMINANCE4_ALPHA4              0x8043
#define GL_LUMINANCE6_ALPHA2              0x8044
#define GL_LUMINANCE8_ALPHA8              0x8045
#define GL_LUMINANCE12_ALPHA4             0x8046
#define GL_LUMINANCE12_ALPHA12            0x8047
#define GL_LUMINANCE16_ALPHA16            0x8048
#define GL_INTENSITY                      0x8049
#define GL_INTENSITY4                     0x804A
#define GL_INTENSITY8                     0x804B
#define GL_INTENSITY12                    0x804C
#define GL_INTENSITY16                    0x804D
#define GL_RGB4                           0x804F
#define GL_RGB5                           0x8050
#define GL_RGB8                           0x8051
#define GL_RGB10                          0x8052
#define GL_RGB12                          0x8053
#define GL_RGB16                          0x8054
#define GL_RGBA2                          0x8055
#define GL_RGBA4                          0x8056
#define GL_RGB5_A1                        0x8057
#define GL_RGBA8                          0x8058
#define GL_RGB10_A2                       0x8059
#define GL_RGBA12                         0x805A
#define GL_RGBA16                         0x805B
/*      GL_RGB2_EXT */
/*      GL_DUAL_ALPHA4_SGIS */
/*      GL_DUAL_ALPHA8_SGIS */
/*      GL_DUAL_ALPHA12_SGIS */
/*      GL_DUAL_ALPHA16_SGIS */
/*      GL_DUAL_LUMINANCE4_SGIS */
/*      GL_DUAL_LUMINANCE8_SGIS */
/*      GL_DUAL_LUMINANCE12_SGIS */
/*      GL_DUAL_LUMINANCE16_SGIS */
/*      GL_DUAL_INTENSITY4_SGIS */
/*      GL_DUAL_INTENSITY8_SGIS */
/*      GL_DUAL_INTENSITY12_SGIS */
/*      GL_DUAL_INTENSITY16_SGIS */
/*      GL_DUAL_LUMINANCE_ALPHA4_SGIS */
/*      GL_DUAL_LUMINANCE_ALPHA8_SGIS */
/*      GL_QUAD_ALPHA4_SGIS */
/*      GL_QUAD_ALPHA8_SGIS */
/*      GL_QUAD_LUMINANCE4_SGIS */
/*      GL_QUAD_LUMINANCE8_SGIS */
/*      GL_QUAD_INTENSITY4_SGIS */
/*      GL_QUAD_INTENSITY8_SGIS */
/*      GL_DEPTH_COMPONENT16_SGIX */
/*      GL_DEPTH_COMPONENT24_SGIX */
/*      GL_DEPTH_COMPONENT32_SGIX */
/*      GL_RGB_ICC_SGIX */
/*      GL_RGBA_ICC_SGIX */
/*      GL_ALPHA_ICC_SGIX */
/*      GL_LUMINANCE_ICC_SGIX */
/*      GL_INTENSITY_ICC_SGIX */
/*      GL_LUMINANCE_ALPHA_ICC_SGIX */
/*      GL_R5_G6_B5_ICC_SGIX */
/*      GL_R5_G6_B5_A8_ICC_SGIX */
/*      GL_ALPHA16_ICC_SGIX */
/*      GL_LUMINANCE16_ICC_SGIX */
/*      GL_INTENSITY16_ICC_SGIX */
/*      GL_LUMINANCE16_ALPHA8_ICC_SGIX */

/* InterleavedArrayFormat */
#define GL_V2F                            0x2A20
#define GL_V3F                            0x2A21
#define GL_C4UB_V2F                       0x2A22
#define GL_C4UB_V3F                       0x2A23
#define GL_C3F_V3F                        0x2A24
#define GL_N3F_V3F                        0x2A25
#define GL_C4F_N3F_V3F                    0x2A26
#define GL_T2F_V3F                        0x2A27
#define GL_T4F_V4F                        0x2A28
#define GL_T2F_C4UB_V3F                   0x2A29
#define GL_T2F_C3F_V3F                    0x2A2A
#define GL_T2F_N3F_V3F                    0x2A2B
#define GL_T2F_C4F_N3F_V3F                0x2A2C
#define GL_T4F_C4F_N3F_V4F                0x2A2D

/* VertexPointerType */
/*      GL_SHORT */
/*      GL_INT */
/*      GL_FLOAT */
/*      GL_DOUBLE */

/* ClipPlaneName */
#define GL_CLIP_PLANE0                    0x3000
#define GL_CLIP_PLANE1                    0x3001
#define GL_CLIP_PLANE2                    0x3002
#define GL_CLIP_PLANE3                    0x3003
#define GL_CLIP_PLANE4                    0x3004
#define GL_CLIP_PLANE5                    0x3005

/* LightName */
#define GL_LIGHT0                         0x4000
#define GL_LIGHT1                         0x4001
#define GL_LIGHT2                         0x4002
#define GL_LIGHT3                         0x4003
#define GL_LIGHT4                         0x4004
#define GL_LIGHT5                         0x4005
#define GL_LIGHT6                         0x4006
#define GL_LIGHT7                         0x4007
/*      GL_FRAGMENT_LIGHT0_SGIX */
/*      GL_FRAGMENT_LIGHT1_SGIX */
/*      GL_FRAGMENT_LIGHT2_SGIX */
/*      GL_FRAGMENT_LIGHT3_SGIX */
/*      GL_FRAGMENT_LIGHT4_SGIX */
/*      GL_FRAGMENT_LIGHT5_SGIX */
/*      GL_FRAGMENT_LIGHT6_SGIX */
/*      GL_FRAGMENT_LIGHT7_SGIX */

/* EXT_abgr */
#define GL_ABGR_EXT                       0x8000

/* EXT_blend_color */
#define GL_CONSTANT_COLOR                 0x8001
#define GL_CONSTANT_COLOR_EXT             0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT   0x8002
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_CONSTANT_ALPHA_EXT             0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT   0x8004
#define GL_BLEND_COLOR                    0x8005
#define GL_BLEND_COLOR_EXT                0x8005

/* EXT_blend_minmax */
#define GL_FUNC_ADD                       0x8006
#define GL_FUNC_ADD_EXT                   0x8006
#define GL_MIN                            0x8007
#define GL_MIN_EXT                        0x8007
#define GL_MAX                            0x8008
#define GL_MAX_EXT                        0x8008
#define GL_BLEND_EQUATION                 0x8009
#define GL_BLEND_EQUATION_EXT             0x8009

/* EXT_blend_equation_separate */
#define GL_BLEND_EQUATION_RGB             GL_BLEND_EQUATION
#define GL_BLEND_EQUATION_RGB_EXT         GL_BLEND_EQUATION

/* EXT_blend_subtract */
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_FUNC_SUBTRACT_EXT              0x800A
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_FUNC_REVERSE_SUBTRACT_EXT      0x800B

/* EXT_cmyka */
#define GL_CMYK_EXT                       0x800C
#define GL_CMYKA_EXT                      0x800D
#define GL_PACK_CMYK_HINT_EXT             0x800E
#define GL_UNPACK_CMYK_HINT_EXT           0x800F

/* EXT_convolution */
#define GL_CONVOLUTION_1D                 0x8010
#define GL_CONVOLUTION_1D_EXT             0x8010
#define GL_CONVOLUTION_2D                 0x8011
#define GL_CONVOLUTION_2D_EXT             0x8011
#define GL_SEPARABLE_2D                   0x8012
#define GL_SEPARABLE_2D_EXT               0x8012
#define GL_CONVOLUTION_BORDER_MODE        0x8013
#define GL_CONVOLUTION_BORDER_MODE_EXT    0x8013
#define GL_CONVOLUTION_FILTER_SCALE       0x8014
#define GL_CONVOLUTION_FILTER_SCALE_EXT   0x8014
#define GL_CONVOLUTION_FILTER_BIAS        0x8015
#define GL_CONVOLUTION_FILTER_BIAS_EXT    0x8015
#define GL_REDUCE                         0x8016
#define GL_REDUCE_EXT                     0x8016
#define GL_CONVOLUTION_FORMAT             0x8017
#define GL_CONVOLUTION_FORMAT_EXT         0x8017
#define GL_CONVOLUTION_WIDTH              0x8018
#define GL_CONVOLUTION_WIDTH_EXT          0x8018
#define GL_CONVOLUTION_HEIGHT             0x8019
#define GL_CONVOLUTION_HEIGHT_EXT         0x8019
#define GL_MAX_CONVOLUTION_WIDTH          0x801A
#define GL_MAX_CONVOLUTION_WIDTH_EXT      0x801A
#define GL_MAX_CONVOLUTION_HEIGHT         0x801B
#define GL_MAX_CONVOLUTION_HEIGHT_EXT     0x801B
#define GL_POST_CONVOLUTION_RED_SCALE     0x801C
#define GL_POST_CONVOLUTION_RED_SCALE_EXT 0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE   0x801D
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT 0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE    0x801E
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT 0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE   0x801F
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT 0x801F
#define GL_POST_CONVOLUTION_RED_BIAS      0x8020
#define GL_POST_CONVOLUTION_RED_BIAS_EXT  0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS    0x8021
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT 0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS     0x8022
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT 0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS    0x8023
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT 0x8023

/* EXT_histogram */
#define GL_HISTOGRAM                      0x8024
#define GL_HISTOGRAM_EXT                  0x8024
#define GL_PROXY_HISTOGRAM                0x8025
#define GL_PROXY_HISTOGRAM_EXT            0x8025
#define GL_HISTOGRAM_WIDTH                0x8026
#define GL_HISTOGRAM_WIDTH_EXT            0x8026
#define GL_HISTOGRAM_FORMAT               0x8027
#define GL_HISTOGRAM_FORMAT_EXT           0x8027
#define GL_HISTOGRAM_RED_SIZE             0x8028
#define GL_HISTOGRAM_RED_SIZE_EXT         0x8028
#define GL_HISTOGRAM_GREEN_SIZE           0x8029
#define GL_HISTOGRAM_GREEN_SIZE_EXT       0x8029
#define GL_HISTOGRAM_BLUE_SIZE            0x802A
#define GL_HISTOGRAM_BLUE_SIZE_EXT        0x802A
#define GL_HISTOGRAM_ALPHA_SIZE           0x802B
#define GL_HISTOGRAM_ALPHA_SIZE_EXT       0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE       0x802C
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT   0x802C
#define GL_HISTOGRAM_SINK                 0x802D
#define GL_HISTOGRAM_SINK_EXT             0x802D
#define GL_MINMAX                         0x802E
#define GL_MINMAX_EXT                     0x802E
#define GL_MINMAX_FORMAT                  0x802F
#define GL_MINMAX_FORMAT_EXT              0x802F
#define GL_MINMAX_SINK                    0x8030
#define GL_MINMAX_SINK_EXT                0x8030
#define GL_TABLE_TOO_LARGE                0x8031
#define GL_TABLE_TOO_LARGE_EXT            0x8031

/* EXT_packed_pixels */
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_BYTE_3_3_2_EXT        0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT     0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT     0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_8_8_8_8_EXT       0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_UNSIGNED_INT_10_10_10_2_EXT    0x8036
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_BYTE_2_3_3_REV_EXT    0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_EXT       0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_5_6_5_REV_EXT   0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT 0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_UNSIGNED_INT_8_8_8_8_REV_EXT   0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#define GL_UNSIGNED_INT_2_10_10_10_REV_EXT 0x8368

/* EXT_polygon_offset */
#define GL_POLYGON_OFFSET_EXT             0x8037
#define GL_POLYGON_OFFSET_FACTOR_EXT      0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT        0x8039

/* EXT_rescale_normal */
#define GL_RESCALE_NORMAL                 0x803A
#define GL_RESCALE_NORMAL_EXT             0x803A

/* EXT_texture */
#define GL_ALPHA4_EXT                     0x803B
#define GL_ALPHA8_EXT                     0x803C
#define GL_ALPHA12_EXT                    0x803D
#define GL_ALPHA16_EXT                    0x803E
#define GL_LUMINANCE4_EXT                 0x803F
#define GL_LUMINANCE8_EXT                 0x8040
#define GL_LUMINANCE12_EXT                0x8041
#define GL_LUMINANCE16_EXT                0x8042
#define GL_LUMINANCE4_ALPHA4_EXT          0x8043
#define GL_LUMINANCE6_ALPHA2_EXT          0x8044
#define GL_LUMINANCE8_ALPHA8_EXT          0x8045
#define GL_LUMINANCE12_ALPHA4_EXT         0x8046
#define GL_LUMINANCE12_ALPHA12_EXT        0x8047
#define GL_LUMINANCE16_ALPHA16_EXT        0x8048
#define GL_INTENSITY_EXT                  0x8049
#define GL_INTENSITY4_EXT                 0x804A
#define GL_INTENSITY8_EXT                 0x804B
#define GL_INTENSITY12_EXT                0x804C
#define GL_INTENSITY16_EXT                0x804D
#define GL_RGB2_EXT                       0x804E
#define GL_RGB4_EXT                       0x804F
#define GL_RGB5_EXT                       0x8050
#define GL_RGB8_EXT                       0x8051
#define GL_RGB10_EXT                      0x8052
#define GL_RGB12_EXT                      0x8053
#define GL_RGB16_EXT                      0x8054
#define GL_RGBA2_EXT                      0x8055
#define GL_RGBA4_EXT                      0x8056
#define GL_RGB5_A1_EXT                    0x8057
#define GL_RGBA8_EXT                      0x8058
#define GL_RGB10_A2_EXT                   0x8059
#define GL_RGBA12_EXT                     0x805A
#define GL_RGBA16_EXT                     0x805B
#define GL_TEXTURE_RED_SIZE_EXT           0x805C
#define GL_TEXTURE_GREEN_SIZE_EXT         0x805D
#define GL_TEXTURE_BLUE_SIZE_EXT          0x805E
#define GL_TEXTURE_ALPHA_SIZE_EXT         0x805F
#define GL_TEXTURE_LUMINANCE_SIZE_EXT     0x8060
#define GL_TEXTURE_INTENSITY_SIZE_EXT     0x8061
#define GL_REPLACE_EXT                    0x8062
#define GL_PROXY_TEXTURE_1D_EXT           0x8063
#define GL_PROXY_TEXTURE_2D_EXT           0x8064
#define GL_TEXTURE_TOO_LARGE_EXT          0x8065

/* EXT_texture_object */
#define GL_TEXTURE_PRIORITY_EXT           0x8066
#define GL_TEXTURE_RESIDENT_EXT           0x8067
#define GL_TEXTURE_1D_BINDING_EXT         0x8068
#define GL_TEXTURE_2D_BINDING_EXT         0x8069
#define GL_TEXTURE_3D_BINDING_EXT         0x806A

/* EXT_texture3D */
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_SKIP_IMAGES_EXT           0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_PACK_IMAGE_HEIGHT_EXT          0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_SKIP_IMAGES_EXT         0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_UNPACK_IMAGE_HEIGHT_EXT        0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_3D_EXT                 0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_PROXY_TEXTURE_3D_EXT           0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_DEPTH_EXT              0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_TEXTURE_WRAP_R_EXT             0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073

/* EXT_vertex_array */
#define GL_VERTEX_ARRAY_EXT               0x8074
#define GL_NORMAL_ARRAY_EXT               0x8075
#define GL_COLOR_ARRAY_EXT                0x8076
#define GL_INDEX_ARRAY_EXT                0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT        0x8078
#define GL_EDGE_FLAG_ARRAY_EXT            0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT          0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT          0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT        0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT         0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT          0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT        0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT         0x8080
#define GL_COLOR_ARRAY_SIZE_EXT           0x8081
#define GL_COLOR_ARRAY_TYPE_EXT           0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT         0x8083
#define GL_COLOR_ARRAY_COUNT_EXT          0x8084
#define GL_INDEX_ARRAY_TYPE_EXT           0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT         0x8086
#define GL_INDEX_ARRAY_COUNT_EXT          0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT   0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT   0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT 0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT  0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT     0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT      0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT       0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT       0x808F
#define GL_COLOR_ARRAY_POINTER_EXT        0x8090
#define GL_INDEX_ARRAY_POINTER_EXT        0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT 0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT    0x8093

/* ARB_multisample */
#define GL_MULTISAMPLE                    0x809D
#define GL_MULTISAMPLE_ARB                0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB   0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_ALPHA_TO_ONE_ARB        0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_COVERAGE_ARB            0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLE_BUFFERS_ARB             0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLES_ARB                    0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_VALUE_ARB      0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_SAMPLE_COVERAGE_INVERT_ARB     0x80AB

/* SGI_color_matrix */
#define GL_COLOR_MATRIX                   0x80B1
#define GL_COLOR_MATRIX_SGI               0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH       0x80B2
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI   0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH   0x80B3
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI 0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE    0x80B4
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI 0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE  0x80B5
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI 0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE   0x80B6
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI 0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE  0x80B7
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI 0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS     0x80B8
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI 0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS   0x80B9
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI 0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS    0x80BA
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI 0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS   0x80BB
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI 0x80BB

/* SGIX_shadow_ambient */

/* ARB_shadow_ambient */
#define GL_SHADOW_AMBIENT_SGIX            0x80BF
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB 0x80BF

/* EXT_blend_func_separate */
#define GL_BLEND_DST_RGB                  0x80C8
#define GL_BLEND_DST_RGB_EXT              0x80C8
#define GL_BLEND_SRC_RGB                  0x80C9
#define GL_BLEND_SRC_RGB_EXT              0x80C9
#define GL_BLEND_DST_ALPHA                0x80CA
#define GL_BLEND_DST_ALPHA_EXT            0x80CA
#define GL_BLEND_SRC_ALPHA                0x80CB
#define GL_BLEND_SRC_ALPHA_EXT            0x80CB

/* SGI_color_table */
#define GL_COLOR_TABLE                    0x80D0
#define GL_COLOR_TABLE_SGI                0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE   0x80D1
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE  0x80D2
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D2
#define GL_PROXY_COLOR_TABLE              0x80D3
#define GL_PROXY_COLOR_TABLE_SGI          0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE 0x80D4
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE 0x80D5
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D5
#define GL_COLOR_TABLE_SCALE              0x80D6
#define GL_COLOR_TABLE_SCALE_SGI          0x80D6
#define GL_COLOR_TABLE_BIAS               0x80D7
#define GL_COLOR_TABLE_BIAS_SGI           0x80D7
#define GL_COLOR_TABLE_FORMAT             0x80D8
#define GL_COLOR_TABLE_FORMAT_SGI         0x80D8
#define GL_COLOR_TABLE_WIDTH              0x80D9
#define GL_COLOR_TABLE_WIDTH_SGI          0x80D9
#define GL_COLOR_TABLE_RED_SIZE           0x80DA
#define GL_COLOR_TABLE_RED_SIZE_SGI       0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE         0x80DB
#define GL_COLOR_TABLE_GREEN_SIZE_SGI     0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE          0x80DC
#define GL_COLOR_TABLE_BLUE_SIZE_SGI      0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE         0x80DD
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI     0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE     0x80DE
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI 0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE     0x80DF
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI 0x80DF

/* EXT_bgra */
#define GL_BGR                            0x80E0
#define GL_BGR_EXT                        0x80E0
#define GL_BGRA                           0x80E1
#define GL_BGRA_EXT                       0x80E1
#define GL_MAX_ELEMENTS_VERTICES          0x80E8
#define GL_MAX_ELEMENTS_INDICES           0x80E9

/* ARB_point_parameters */

/* EXT_point_parameters */

/* SGIS_point_parameters */
#define GL_POINT_SIZE_MIN                 0x8126
#define GL_POINT_SIZE_MIN_ARB             0x8126
#define GL_POINT_SIZE_MIN_EXT             0x8126
#define GL_POINT_SIZE_MIN_SGIS            0x8126
#define GL_POINT_SIZE_MAX                 0x8127
#define GL_POINT_SIZE_MAX_ARB             0x8127
#define GL_POINT_SIZE_MAX_EXT             0x8127
#define GL_POINT_SIZE_MAX_SGIS            0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE      0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_ARB  0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT  0x8128
#define GL_POINT_FADE_THRESHOLD_SIZE_SGIS 0x8128
#define GL_POINT_DISTANCE_ATTENUATION     0x8129
#define GL_POINT_DISTANCE_ATTENUATION_ARB 0x8129
#define GL_DISTANCE_ATTENUATION_EXT       0x8129
#define GL_DISTANCE_ATTENUATION_SGIS      0x8129

/* ARB_texture_border_clamp */

/* SGIS_texture_border_clamp */
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_CLAMP_TO_BORDER_ARB            0x812D
#define GL_CLAMP_TO_BORDER_SGIS           0x812D

/* SGIS_texture_edge_clamp */
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_CLAMP_TO_EDGE_SGIS             0x812F

/* SGIS_texture_lod */
#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MIN_LOD_SGIS           0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_MAX_LOD_SGIS           0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_BASE_LEVEL_SGIS        0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_TEXTURE_MAX_LEVEL_SGIS         0x813D
#define GL_CONSTANT_BORDER                0x8151
#define GL_CONSTANT_BORDER_HP             0x8151
#define GL_REPLICATE_BORDER               0x8153
#define GL_REPLICATE_BORDER_HP            0x8153
#define GL_CONVOLUTION_BORDER_COLOR       0x8154
#define GL_CONVOLUTION_BORDER_COLOR_HP    0x8154

/* SGIS_generate_mipmap */
#define GL_GENERATE_MIPMAP                0x8191
#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT           0x8192
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192

/* ARB_depth_texture */

/* SGIX_depth_texture */
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT16_SGIX         0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT24_SGIX         0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32_SGIX         0x81A7

/* EXT_compiled_vertex_array */
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT   0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT   0x81A9

/* SGIX_ycrcb */
#define GL_YCRCB_422_SGIX                 0x81BB
#define GL_YCRCB_444_SGIX                 0x81BC

/* EXT_separate_specular_color */
#define GL_LIGHT_MODEL_COLOR_CONTROL      0x81F8
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT  0x81F8
#define GL_SINGLE_COLOR                   0x81F9
#define GL_SINGLE_COLOR_EXT               0x81F9
#define GL_SEPARATE_SPECULAR_COLOR        0x81FA
#define GL_SEPARATE_SPECULAR_COLOR_EXT    0x81FA

/* SGIX_convolution_accuracy */
#define GL_CONVOLUTION_HINT_SGIX          0x8316
#define GL_ASYNC_MARKER_SGIX              0x8329
#define GL_FOG_FACTOR_TO_ALPHA_SGIX       0x836F

/* ARB_texture_mirrored_repeat */

/* IBM_texture_mirrored_repeat */
#define GL_MIRRORED_REPEAT                0x8370
#define GL_MIRRORED_REPEAT_ARB            0x8370
#define GL_MIRRORED_REPEAT_IBM            0x8370

/* EXT_fog_coord */
#define GL_FOG_COORD_SRC                  GL_FOG_COORDINATE_SOURCE
#define GL_FOG_COORDINATE_SOURCE          0x8450
#define GL_FOG_COORDINATE_SOURCE_EXT      0x8450
#define GL_FOG_COORD                      GL_FOG_COORDINATE
#define GL_FOG_COORDINATE                 0x8451
#define GL_FOG_COORDINATE_EXT             0x8451
#define GL_FRAGMENT_DEPTH                 0x8452
#define GL_FRAGMENT_DEPTH_EXT             0x8452
#define GL_CURRENT_FOG_COORD              GL_CURRENT_FOG_COORDINATE
#define GL_CURRENT_FOG_COORDINATE         0x8453
#define GL_CURRENT_FOG_COORDINATE_EXT     0x8453
#define GL_FOG_COORD_ARRAY_TYPE           GL_FOG_COORDINATE_ARRAY_TYPE
#define GL_FOG_COORDINATE_ARRAY_TYPE      0x8454
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT  0x8454
#define GL_FOG_COORD_ARRAY_STRIDE         GL_FOG_COORDINATE_ARRAY_STRIDE
#define GL_FOG_COORDINATE_ARRAY_STRIDE    0x8455
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT 0x8455
#define GL_FOG_COORD_ARRAY_POINTER        GL_FOG_COORDINATE_ARRAY_POINTER
#define GL_FOG_COORDINATE_ARRAY_POINTER   0x8456
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT 0x8456
#define GL_FOG_COORD_ARRAY                GL_FOG_COORDINATE_ARRAY
#define GL_FOG_COORDINATE_ARRAY           0x8457
#define GL_FOG_COORDINATE_ARRAY_EXT       0x8457

/* EXT_secondary_color */

/* ARB_vertex_program */
#define GL_COLOR_SUM                      0x8458
#define GL_COLOR_SUM_EXT                  0x8458
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_CURRENT_SECONDARY_COLOR        0x8459
#define GL_CURRENT_SECONDARY_COLOR_EXT    0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE     0x845A
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT 0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE     0x845B
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT 0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE   0x845C
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT 0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER  0x845D
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT 0x845D
#define GL_SECONDARY_COLOR_ARRAY          0x845E
#define GL_SECONDARY_COLOR_ARRAY_EXT      0x845E
#define GL_CURRENT_RASTER_SECONDARY_COLOR 0x845F
#define GL_SMOOTH_POINT_SIZE_RANGE        0x0B12
#define GL_SMOOTH_POINT_SIZE_GRANULARITY  0x0B13
#define GL_SMOOTH_LINE_WIDTH_RANGE        0x0B22
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY  0x0B23
#define GL_ALIASED_POINT_SIZE_RANGE       0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE       0x846E

/* ARB_multitexture */
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_ACTIVE_TEXTURE                 0x84E0
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE          0x84E1
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS              0x84E2
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2

/* ARB_transpose_matrix */
#define GL_TRANSPOSE_MODELVIEW_MATRIX     0x84E3
#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB 0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX    0x84E4
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB 0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX       0x84E5
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB   0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX         0x84E6
#define GL_TRANSPOSE_COLOR_MATRIX_ARB     0x84E6

/* ARB_texture_env_combine */
#define GL_SUBTRACT                       0x84E7
#define GL_SUBTRACT_ARB                   0x84E7

/* EXT_framebuffer_object */
#define GL_MAX_RENDERBUFFER_SIZE_EXT      0x84E8

/* ARB_texture_compression */
#define GL_COMPRESSED_ALPHA               0x84E9
#define GL_COMPRESSED_ALPHA_ARB           0x84E9
#define GL_COMPRESSED_LUMINANCE           0x84EA
#define GL_COMPRESSED_LUMINANCE_ARB       0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA     0x84EB
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY           0x84EC
#define GL_COMPRESSED_INTENSITY_ARB       0x84EC
#define GL_COMPRESSED_RGB                 0x84ED
#define GL_COMPRESSED_RGB_ARB             0x84ED
#define GL_COMPRESSED_RGBA                0x84EE
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#define GL_TEXTURE_COMPRESSION_HINT       0x84EF
#define GL_TEXTURE_COMPRESSION_HINT_ARB   0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE  0x86A0
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB 0x86A0
#define GL_TEXTURE_COMPRESSED             0x86A1
#define GL_TEXTURE_COMPRESSED_ARB         0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS     0x86A3
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3

/* EXT_texture_lod_bias */
#define GL_MAX_TEXTURE_LOD_BIAS           0x84FD
#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD

/* EXT_texture_filter_anisotropic */
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

/* EXT_texture_lod_bias */
#define GL_TEXTURE_FILTER_CONTROL         0x8500
#define GL_TEXTURE_FILTER_CONTROL_EXT     0x8500
#define GL_TEXTURE_LOD_BIAS               0x8501
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501

/* EXT_vertex_weighting */
#define GL_MODELVIEW1_STACK_DEPTH_EXT     0x8502

/* EXT_vertex_weighting */
#define GL_MODELVIEW_MATRIX1_EXT          0x8506

/* EXT_stencil_wrap */
#define GL_INCR_WRAP                      0x8507
#define GL_INCR_WRAP_EXT                  0x8507
#define GL_DECR_WRAP                      0x8508
#define GL_DECR_WRAP_EXT                  0x8508

/* EXT_vertex_weighting */
#define GL_VERTEX_WEIGHTING_EXT           0x8509
#define GL_MODELVIEW1_EXT                 0x850A
#define GL_CURRENT_VERTEX_WEIGHT_EXT      0x850B
#define GL_VERTEX_WEIGHT_ARRAY_EXT        0x850C
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT   0x850D
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT   0x850E
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT 0x850F
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT 0x8510

/* EXT_texture_cube_map */

/* ARB_texture_cube_map */
#define GL_NORMAL_MAP                     0x8511
#define GL_NORMAL_MAP_ARB                 0x8511
#define GL_REFLECTION_MAP                 0x8512
#define GL_REFLECTION_MAP_ARB             0x8512
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_TEXTURE_CUBE_MAP_ARB           0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP       0x8514
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB   0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X    0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y    0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y    0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z    0x8519
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z    0x851A
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP         0x851B
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB     0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE      0x851C
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB  0x851C

/* EXT_texture_env_combine */
#define GL_COMBINE                        0x8570
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB                    0x8571
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA                  0x8572
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_RGB_SCALE                      0x8573
#define GL_RGB_SCALE_EXT                  0x8573
#define GL_ADD_SIGNED                     0x8574
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE                    0x8575
#define GL_INTERPOLATE_EXT                0x8575
#define GL_CONSTANT                       0x8576
#define GL_CONSTANT_EXT                   0x8576
#define GL_PRIMARY_COLOR                  0x8577
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS                       0x8578
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SRC0_RGB                       GL_SOURCE0_RGB
#define GL_SOURCE0_RGB                    0x8580
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SRC1_RGB                       GL_SOURCE1_RGB
#define GL_SOURCE1_RGB                    0x8581
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SRC2_RGB                       GL_SOURCE2_RGB
#define GL_SOURCE2_RGB                    0x8582
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SRC0_ALPHA                     GL_SOURCE0_ALPHA
#define GL_SOURCE0_ALPHA                  0x8588
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SRC1_ALPHA                     GL_SOURCE1_ALPHA
#define GL_SOURCE1_ALPHA                  0x8589
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SRC2_ALPHA                     GL_SOURCE2_ALPHA
#define GL_SOURCE2_ALPHA                  0x858A
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_OPERAND0_RGB                   0x8590
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB                   0x8591
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB                   0x8592
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND0_ALPHA                 0x8598
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA                 0x8599
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA                 0x859A
#define GL_OPERAND2_ALPHA_EXT             0x859A

/* SGIX_subsample */
#define GL_PACK_SUBSAMPLE_RATE_SGIX       0x85A0
#define GL_UNPACK_SUBSAMPLE_RATE_SGIX     0x85A1
#define GL_PIXEL_SUBSAMPLE_4444_SGIX      0x85A2
#define GL_PIXEL_SUBSAMPLE_2424_SGIX      0x85A3
#define GL_PIXEL_SUBSAMPLE_4242_SGIX      0x85A4

/* ARB_vertex_program */

/* ARB_fragment_program */
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED    0x8622
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE       0x8623
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE     0x8624
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE       0x8625
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_CURRENT_VERTEX_ATTRIB          0x8626
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE        0x8643
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER    0x8645
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_PROGRAM_BINDING_ARB            0x8677

/* ARB_vertex_blend */
#define GL_MAX_VERTEX_UNITS_ARB           0x86A4
#define GL_ACTIVE_VERTEX_UNITS_ARB        0x86A5
#define GL_WEIGHT_SUM_UNITY_ARB           0x86A6
#define GL_VERTEX_BLEND_ARB               0x86A7
#define GL_CURRENT_WEIGHT_ARB             0x86A8
#define GL_WEIGHT_ARRAY_TYPE_ARB          0x86A9
#define GL_WEIGHT_ARRAY_STRIDE_ARB        0x86AA
#define GL_WEIGHT_ARRAY_SIZE_ARB          0x86AB
#define GL_WEIGHT_ARRAY_POINTER_ARB       0x86AC
#define GL_WEIGHT_ARRAY_ARB               0x86AD
#define GL_MODELVIEW0_ARB                 0x1700
#define GL_MODELVIEW1_ARB                 0x850A
#define GL_MODELVIEW2_ARB                 0x8722
#define GL_MODELVIEW3_ARB                 0x8723
#define GL_MODELVIEW4_ARB                 0x8724
#define GL_MODELVIEW5_ARB                 0x8725
#define GL_MODELVIEW6_ARB                 0x8726
#define GL_MODELVIEW7_ARB                 0x8727
#define GL_MODELVIEW8_ARB                 0x8728
#define GL_MODELVIEW9_ARB                 0x8729
#define GL_MODELVIEW10_ARB                0x872A
#define GL_MODELVIEW11_ARB                0x872B
#define GL_MODELVIEW12_ARB                0x872C
#define GL_MODELVIEW13_ARB                0x872D
#define GL_MODELVIEW14_ARB                0x872E
#define GL_MODELVIEW15_ARB                0x872F
#define GL_MODELVIEW16_ARB                0x8730
#define GL_MODELVIEW17_ARB                0x8731
#define GL_MODELVIEW18_ARB                0x8732
#define GL_MODELVIEW19_ARB                0x8733
#define GL_MODELVIEW20_ARB                0x8734
#define GL_MODELVIEW21_ARB                0x8735
#define GL_MODELVIEW22_ARB                0x8736
#define GL_MODELVIEW23_ARB                0x8737
#define GL_MODELVIEW24_ARB                0x8738
#define GL_MODELVIEW25_ARB                0x8739
#define GL_MODELVIEW26_ARB                0x873A
#define GL_MODELVIEW27_ARB                0x873B
#define GL_MODELVIEW28_ARB                0x873C
#define GL_MODELVIEW29_ARB                0x873D
#define GL_MODELVIEW30_ARB                0x873E
#define GL_MODELVIEW31_ARB                0x873F

/* ARB_texture_env_dot3 */
#define GL_DOT3_RGB                       0x86AE
#define GL_DOT3_RGB_ARB                   0x86AE
#define GL_DOT3_RGBA                      0x86AF
#define GL_DOT3_RGBA_ARB                  0x86AF

/* EXT_texture_env_dot3 */
#define GL_DOT3_RGB_EXT                   0x8740
#define GL_DOT3_RGBA_EXT                  0x8741

/* ATI_vertex_array_object */
#define GL_STATIC_ATI                     0x8760
#define GL_DYNAMIC_ATI                    0x8761
#define GL_PRESERVE_ATI                   0x8762
#define GL_DISCARD_ATI                    0x8763
#define GL_OBJECT_BUFFER_SIZE_ATI         0x8764
#define GL_OBJECT_BUFFER_USAGE_ATI        0x8765
#define GL_ARRAY_OBJECT_BUFFER_ATI        0x8766
#define GL_ARRAY_OBJECT_OFFSET_ATI        0x8767

/* ARB_vertex_buffer_object */
#define GL_BUFFER_SIZE                    0x8764
#define GL_BUFFER_SIZE_ARB                0x8764
#define GL_BUFFER_USAGE                   0x8765
#define GL_BUFFER_USAGE_ARB               0x8765

/* ATI_element_array */
#define GL_ELEMENT_ARRAY_ATI              0x8768
#define GL_ELEMENT_ARRAY_TYPE_ATI         0x8769
#define GL_ELEMENT_ARRAY_POINTER_ATI      0x876A
#define GL_STENCIL_BACK_FUNC              0x8800
#define GL_STENCIL_BACK_FAIL              0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL   0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS   0x8803

/* ARB_fragment_program */
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810

/* ARB_texture_float */
#define GL_RGBA32F_ARB                    0x8814
#define GL_RGBA_FLOAT32_ATI               0x8814
#define GL_RGB32F_ARB                     0x8815
#define GL_RGB_FLOAT32_ATI                0x8815
#define GL_ALPHA32F_ARB                   0x8816
#define GL_ALPHA_FLOAT32_ATI              0x8816
#define GL_INTENSITY32F_ARB               0x8817
#define GL_INTENSITY_FLOAT32_ATI          0x8817
#define GL_LUMINANCE32F_ARB               0x8818
#define GL_LUMINANCE_FLOAT32_ATI          0x8818
#define GL_LUMINANCE_ALPHA32F_ARB         0x8819
#define GL_LUMINANCE_ALPHA_FLOAT32_ATI    0x8819
#define GL_RGBA16F_ARB                    0x881A
#define GL_RGBA_FLOAT16_ATI               0x881A
#define GL_RGB16F_ARB                     0x881B
#define GL_RGB_FLOAT16_ATI                0x881B
#define GL_ALPHA16F_ARB                   0x881C
#define GL_ALPHA_FLOAT16_ATI              0x881C
#define GL_INTENSITY16F_ARB               0x881D
#define GL_INTENSITY_FLOAT16_ATI          0x881D
#define GL_LUMINANCE16F_ARB               0x881E
#define GL_LUMINANCE_FLOAT16_ATI          0x881E
#define GL_LUMINANCE_ALPHA16F_ARB         0x881F
#define GL_LUMINANCE_ALPHA_FLOAT16_ATI    0x881F

/* ARB_color_buffer_float */
#define GL_RGBA_FLOAT_MODE_ARB            0x8820

/* ARB_draw_buffers */
#define GL_MAX_DRAW_BUFFERS               0x8824
#define GL_MAX_DRAW_BUFFERS_ARB           0x8824
#define GL_DRAW_BUFFER0                   0x8825
#define GL_DRAW_BUFFER0_ARB               0x8825
#define GL_DRAW_BUFFER1                   0x8826
#define GL_DRAW_BUFFER1_ARB               0x8826
#define GL_DRAW_BUFFER2                   0x8827
#define GL_DRAW_BUFFER2_ARB               0x8827
#define GL_DRAW_BUFFER3                   0x8828
#define GL_DRAW_BUFFER3_ARB               0x8828
#define GL_DRAW_BUFFER4                   0x8829
#define GL_DRAW_BUFFER4_ARB               0x8829
#define GL_DRAW_BUFFER5                   0x882A
#define GL_DRAW_BUFFER5_ARB               0x882A
#define GL_DRAW_BUFFER6                   0x882B
#define GL_DRAW_BUFFER6_ARB               0x882B
#define GL_DRAW_BUFFER7                   0x882C
#define GL_DRAW_BUFFER7_ARB               0x882C
#define GL_DRAW_BUFFER8                   0x882D
#define GL_DRAW_BUFFER8_ARB               0x882D
#define GL_DRAW_BUFFER9                   0x882E
#define GL_DRAW_BUFFER9_ARB               0x882E
#define GL_DRAW_BUFFER10                  0x882F
#define GL_DRAW_BUFFER10_ARB              0x882F
#define GL_DRAW_BUFFER11                  0x8830
#define GL_DRAW_BUFFER11_ARB              0x8830
#define GL_DRAW_BUFFER12                  0x8831
#define GL_DRAW_BUFFER12_ARB              0x8831
#define GL_DRAW_BUFFER13                  0x8832
#define GL_DRAW_BUFFER13_ARB              0x8832
#define GL_DRAW_BUFFER14                  0x8833
#define GL_DRAW_BUFFER14_ARB              0x8833
#define GL_DRAW_BUFFER15                  0x8834
#define GL_DRAW_BUFFER15_ARB              0x8834

/* EXT_blend_equation_separate */
#define GL_BLEND_EQUATION_ALPHA           0x883D
#define GL_BLEND_EQUATION_ALPHA_EXT       0x883D

/* ARB_matrix_palette */
#define GL_MATRIX_PALETTE_ARB             0x8840
#define GL_MAX_MATRIX_PALETTE_STACK_DEPTH_ARB 0x8841
#define GL_MAX_PALETTE_MATRICES_ARB       0x8842
#define GL_CURRENT_PALETTE_MATRIX_ARB     0x8843
#define GL_MATRIX_INDEX_ARRAY_ARB         0x8844
#define GL_CURRENT_MATRIX_INDEX_ARB       0x8845
#define GL_MATRIX_INDEX_ARRAY_SIZE_ARB    0x8846
#define GL_MATRIX_INDEX_ARRAY_TYPE_ARB    0x8847
#define GL_MATRIX_INDEX_ARRAY_STRIDE_ARB  0x8848
#define GL_MATRIX_INDEX_ARRAY_POINTER_ARB 0x8849

/* ARB_depth_texture */
#define GL_TEXTURE_DEPTH_SIZE             0x884A
#define GL_TEXTURE_DEPTH_SIZE_ARB         0x884A
#define GL_DEPTH_TEXTURE_MODE             0x884B
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B

/* ARB_shadow */
#define GL_TEXTURE_COMPARE_MODE           0x884C
#define GL_TEXTURE_COMPARE_MODE_ARB       0x884C
#define GL_TEXTURE_COMPARE_FUNC           0x884D
#define GL_TEXTURE_COMPARE_FUNC_ARB       0x884D
#define GL_COMPARE_R_TO_TEXTURE           0x884E
#define GL_COMPARE_R_TO_TEXTURE_ARB       0x884E

/* ARB_point_sprite */
#define GL_POINT_SPRITE                   0x8861
#define GL_POINT_SPRITE_ARB               0x8861
#define GL_POINT_SPRITE_NV                0x8861
#define GL_COORD_REPLACE                  0x8862
#define GL_COORD_REPLACE_ARB              0x8862
#define GL_COORD_REPLACE_NV               0x8862

/* ARB_occlusion_query */
#define GL_QUERY_COUNTER_BITS             0x8864
#define GL_QUERY_COUNTER_BITS_ARB         0x8864
#define GL_PIXEL_COUNTER_BITS_NV          0x8864
#define GL_CURRENT_QUERY                  0x8865
#define GL_CURRENT_QUERY_ARB              0x8865
#define GL_CURRENT_OCCLUSION_QUERY_ID_NV  0x8865
#define GL_QUERY_RESULT                   0x8866
#define GL_QUERY_RESULT_ARB               0x8866
#define GL_PIXEL_COUNT_NV                 0x8866
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#define GL_QUERY_RESULT_AVAILABLE_ARB     0x8867
#define GL_PIXEL_COUNT_AVAILABLE_NV       0x8867

/* ARB_vertex_program */
#define GL_MAX_VERTEX_ATTRIBS             0x8869
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A

/* ARB_vertex_program */

/* ARB_fragment_program */
#define GL_MAX_TEXTURE_COORDS             0x8871
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS        0x8872
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_PROGRAM_FORMAT_ARB             0x8876

/* ARB_vertex_buffer_object */
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ARRAY_BUFFER_ARB               0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ELEMENT_ARRAY_BUFFER_ARB       0x8893
#define GL_ARRAY_BUFFER_BINDING           0x8894
#define GL_ARRAY_BUFFER_BINDING_ARB       0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING   0x8895
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING    0x8896
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING    0x8897
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING     0x8898
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING     0x8899
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING 0x889A
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING 0x889B
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING 0x889C
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORD_ARRAY_BUFFER_BINDING GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING 0x889D
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING    0x889E
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F

/* ARB_vertex_program */

/* ARB_fragment_program */
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7

/* ARB_vertex_buffer_object */
#define GL_READ_ONLY                      0x88B8
#define GL_READ_ONLY_ARB                  0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_WRITE_ONLY_ARB                 0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_READ_WRITE_ARB                 0x88BA
#define GL_BUFFER_ACCESS                  0x88BB
#define GL_BUFFER_ACCESS_ARB              0x88BB
#define GL_BUFFER_MAPPED                  0x88BC
#define GL_BUFFER_MAPPED_ARB              0x88BC
#define GL_BUFFER_MAP_POINTER             0x88BD
#define GL_BUFFER_MAP_POINTER_ARB         0x88BD

/* ARB_vertex_program */

/* ARB_fragment_program */
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF

/* ARB_vertex_buffer_object */
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_DRAW_ARB                0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_READ_ARB                0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STREAM_COPY_ARB                0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_DRAW_ARB                0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_READ_ARB                0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_STATIC_COPY_ARB                0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_DRAW_ARB               0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_READ_ARB               0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_DYNAMIC_COPY_ARB               0x88EA

/* EXT_stencil_two_side */
#define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT        0x8911

/* ARB_occlusion_query */
#define GL_SAMPLES_PASSED                 0x8914
#define GL_SAMPLES_PASSED_ARB             0x8914

/* ARB_color_buffer_float */
#define GL_CLAMP_VERTEX_COLOR_ARB         0x891A
#define GL_CLAMP_FRAGMENT_COLOR_ARB       0x891B
#define GL_CLAMP_READ_COLOR_ARB           0x891C
#define GL_FIXED_ONLY_ARB                 0x891D

/* ARB_shader_objects */

/* ARB_vertex_shader */

/* ARB_fragment_shader */
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_FRAGMENT_SHADER_ARB            0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_VERTEX_SHADER_ARB              0x8B31
#define GL_PROGRAM_OBJECT_ARB             0x8B40
#define GL_SHADER_OBJECT_ARB              0x8B48
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB 0x8B4A
#define GL_MAX_VARYING_FLOATS             0x8B4B
#define GL_MAX_VARYING_FLOATS_ARB         0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB 0x8B4D
#define GL_OBJECT_TYPE_ARB                0x8B4E
#define GL_SHADER_TYPE                    0x8B4F
#define GL_OBJECT_SUBTYPE_ARB             0x8B4F
#define GL_FLOAT_VEC2                     0x8B50
#define GL_FLOAT_VEC2_ARB                 0x8B50
#define GL_FLOAT_VEC3                     0x8B51
#define GL_FLOAT_VEC3_ARB                 0x8B51
#define GL_FLOAT_VEC4                     0x8B52
#define GL_FLOAT_VEC4_ARB                 0x8B52
#define GL_INT_VEC2                       0x8B53
#define GL_INT_VEC2_ARB                   0x8B53
#define GL_INT_VEC3                       0x8B54
#define GL_INT_VEC3_ARB                   0x8B54
#define GL_INT_VEC4                       0x8B55
#define GL_INT_VEC4_ARB                   0x8B55
#define GL_BOOL                           0x8B56
#define GL_BOOL_ARB                       0x8B56
#define GL_BOOL_VEC2                      0x8B57
#define GL_BOOL_VEC2_ARB                  0x8B57
#define GL_BOOL_VEC3                      0x8B58
#define GL_BOOL_VEC3_ARB                  0x8B58
#define GL_BOOL_VEC4                      0x8B59
#define GL_BOOL_VEC4_ARB                  0x8B59
#define GL_FLOAT_MAT2                     0x8B5A
#define GL_FLOAT_MAT2_ARB                 0x8B5A
#define GL_FLOAT_MAT3                     0x8B5B
#define GL_FLOAT_MAT3_ARB                 0x8B5B
#define GL_FLOAT_MAT4                     0x8B5C
#define GL_FLOAT_MAT4_ARB                 0x8B5C
#define GL_SAMPLER_1D                     0x8B5D
#define GL_SAMPLER_1D_ARB                 0x8B5D
#define GL_SAMPLER_2D                     0x8B5E
#define GL_SAMPLER_2D_ARB                 0x8B5E
#define GL_SAMPLER_3D                     0x8B5F
#define GL_SAMPLER_3D_ARB                 0x8B5F
#define GL_SAMPLER_CUBE                   0x8B60
#define GL_SAMPLER_CUBE_ARB               0x8B60
#define GL_SAMPLER_1D_SHADOW              0x8B61
#define GL_SAMPLER_1D_SHADOW_ARB          0x8B61
#define GL_SAMPLER_2D_SHADOW              0x8B62
#define GL_SAMPLER_2D_SHADOW_ARB          0x8B62
#define GL_SAMPLER_2D_RECT_ARB            0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB     0x8B64
#define GL_FLOAT_MAT2x3                   0x8B65
#define GL_FLOAT_MAT2x4                   0x8B66
#define GL_FLOAT_MAT3x2                   0x8B67
#define GL_FLOAT_MAT3x4                   0x8B68
#define GL_FLOAT_MAT4x2                   0x8B69
#define GL_FLOAT_MAT4x3                   0x8B6A
#define GL_DELETE_STATUS                  0x8B80
#define GL_OBJECT_DELETE_STATUS_ARB       0x8B80
#define GL_COMPILE_STATUS                 0x8B81
#define GL_OBJECT_COMPILE_STATUS_ARB      0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_OBJECT_LINK_STATUS_ARB         0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_OBJECT_VALIDATE_STATUS_ARB     0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_OBJECT_INFO_LOG_LENGTH_ARB     0x8B84
#define GL_ATTACHED_SHADERS               0x8B85
#define GL_OBJECT_ATTACHED_OBJECTS_ARB    0x8B85
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB     0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH      0x8B87
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_SHADER_SOURCE_LENGTH           0x8B88
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB 0x8B88
#define GL_ACTIVE_ATTRIBUTES              0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB   0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH    0x8B8A
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB 0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB 0x8B8B
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_SHADING_LANGUAGE_VERSION_ARB   0x8B8C
#define GL_CURRENT_PROGRAM                0x8B8D

/* IMG_texture_compression_pvrtc */
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03

/* ARB_texture_float */
#define GL_TEXTURE_RED_TYPE_ARB           0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB         0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB          0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB         0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB     0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB     0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB         0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB        0x8C17

#define GL_SRGB							  0x8C40
#define GL_SRGB8                          0x8C41
#define GL_SRGB_ALPHA					  0x8C42
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_SLUMINANCE_ALPHA				  0x8C44
#define GL_SLUMINANCE8_ALPHA8             0x8C45
#define GL_SLUMINANCE                     0x8C46
#define GL_SLUMINANCE8					  0x8C47
#define GL_COMPRESSED_SRGB                0x8C48
#define GL_COMPRESSED_SRGB_ALPHA          0x8c49
#define GL_COMPRESSED_SLUMINANCE          0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA    0x8C4B

#define GL_POINT_SPRITE_COORD_ORIGIN      0x8CA0
#define GL_LOWER_LEFT                     0x8CA1
#define GL_UPPER_LEFT                     0x8CA2
#define GL_STENCIL_BACK_REF               0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK        0x8CA4
#define GL_STENCIL_BACK_WRITEMASK         0x8CA5

/* EXT_framebuffer_object */
#define GL_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT       0x8CA7

/* EXT_framebuffer_object */
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT       0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT    0x8CDD
#define GL_FRAMEBUFFER_STATUS_ERROR_EXT   0x8CDE
#define GL_MAX_COLOR_ATTACHMENTS_EXT      0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT          0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT          0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT          0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT          0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT          0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT          0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT          0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT          0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT          0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT          0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT         0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT         0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT         0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT         0x8CED
#define GL_COLOR_ATTACHMENT14_EXT         0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT         0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT           0x8D00
#define GL_STENCIL_ATTACHMENT_EXT         0x8D20
#define GL_FRAMEBUFFER_EXT                0x8D40
#define GL_RENDERBUFFER_EXT               0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT         0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT        0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_STENCIL_INDEX_EXT              0x8D45
#define GL_STENCIL_INDEX1_EXT             0x8D46
#define GL_STENCIL_INDEX4_EXT             0x8D47
#define GL_STENCIL_INDEX8_EXT             0x8D48
#define GL_STENCIL_INDEX16_EXT            0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT      0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT    0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT     0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT    0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT    0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT  0x8D55

/* ShaderBinaryFormatImg */
#define GL_SGX_BINARY_IMG                 0x8C0A

/* IMG_line_tessellate */
#define GL_LINE_TESSELLATE_IMG            0x8EA3

/* IMG_texgen_line */
#define GL_LINE_PIXEL_IMG                 0x8EA4
#define GL_LINE_PARAMETRIC_IMG            0x8EA5
#define GL_LINE_REPEAT_IMG                0x8EA6
#define GL_LINE_OFFSET_IMG                0x8EA7
#define GL_LINE_ACCUMULATE_IMG            0x8EA8

/* IMG_delta_buffer */
#define GL_DELTA_BUFFER_TEST_IMG          0x8EA9
#define GL_RANGE_COMPARE_IMG              0x8EAA
#define GL_TEXTURE_PROPAGATE_COORD_IMG    0x8EAB

/* IMG_line_parameters */
#define GL_LINE_WIDTH_MIN_IMG             0x8EAC
#define GL_LINE_WIDTH_MAX_IMG             0x8EAD
#define GL_LINE_DISTANCE_ATTENUATION_IMG  0x8EAE

/* IMG_line_join_style */
#define GL_LINE_JOIN_STYLE_IMG            0x8EAF
#define GL_LINE_JOIN_NONE_IMG             0x6002
#define GL_LINE_JOIN_BEVEL_IMG            0x6003
#define GL_LINE_JOIN_ROUND_IMG            0x6004
#define GL_LINE_JOIN_MITRE_IMG            0x6005

/* IMG_line_cap_style */
#define GL_LINE_CAP_STYLE_IMG             0x6008
#define GL_LINE_CAP_NONE_IMG              0x6009
#define GL_LINE_CAP_BUTT_IMG              0x600A
#define GL_LINE_CAP_ROUND_IMG             0x600B
#define GL_LINE_CAP_SQUARE_IMG            0x600C


/*************************************************************/

GL_API void GL_APIENTRY glAccum (GLenum op, GLfloat value);
GL_API void GL_APIENTRY glActiveStencilFaceEXT (GLenum face);
GL_API void GL_APIENTRY glActiveTexture (GLenum texture);
GL_API void GL_APIENTRY glActiveTextureARB (GLenum texture);
GL_API void GL_APIENTRY glAlphaFunc (GLenum func, GLclampf ref);
GL_API GLboolean GL_APIENTRY glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences);
GL_API void GL_APIENTRY glArrayElement (GLint i);
GL_API void GL_APIENTRY glAttachObjectARB (GLhandleARB containerObj, GLhandleARB obj);
GL_API void GL_APIENTRY glAttachShader (GLuint program, GLuint shader);
GL_API void GL_APIENTRY glBegin (GLenum mode);
GL_API void GL_APIENTRY glBeginQuery (GLenum target, GLuint id);
GL_API void GL_APIENTRY glBeginQueryARB (GLenum target, GLuint id);
GL_API void GL_APIENTRY glBindAttribLocation (GLuint program, GLuint index, const GLcharARB *name);
GL_API void GL_APIENTRY glBindAttribLocationARB (GLhandleARB programObj, GLuint index, const GLcharARB *name);
GL_API void GL_APIENTRY glBindBuffer (GLenum target, GLuint buffer);
GL_API void GL_APIENTRY glBindBufferARB (GLenum target, GLuint buffer);
GL_API void GL_APIENTRY glBindFramebufferEXT (GLenum target, GLuint framebuffer);
GL_API void GL_APIENTRY glBindProgramARB (GLenum target, GLuint program);
GL_API void GL_APIENTRY glBindRenderbufferEXT (GLenum target, GLuint renderbuffer);
GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture);
GL_API void GL_APIENTRY glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
GL_API void GL_APIENTRY glBlendColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GL_API void GL_APIENTRY glBlendEquation (GLenum mode);
GL_API void GL_APIENTRY glBlendEquationSeparate (GLenum modeRGB, GLenum modeAlpha);
GL_API void GL_APIENTRY glBlendEquationSeparateEXT (GLenum modeRGB, GLenum modeAlpha);
GL_API void GL_APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor);
GL_API void GL_APIENTRY glBlendFuncSeparate (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GL_API void GL_APIENTRY glBufferData (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GL_API void GL_APIENTRY glBufferDataARB (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GL_API void GL_APIENTRY glBufferSubData (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
GL_API void GL_APIENTRY glBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
GL_API void GL_APIENTRY glCallList (GLuint list);
GL_API void GL_APIENTRY glCallLists (GLsizei n, GLenum type, const GLvoid *lists);
GL_API GLenum GL_APIENTRY glCheckFramebufferStatusEXT (GLenum target);
GL_API void GL_APIENTRY glClampColorARB (GLenum target, GLenum clamp);
GL_API void GL_APIENTRY glClear (GLbitfield mask);
GL_API void GL_APIENTRY glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GL_API void GL_APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GL_API void GL_APIENTRY glClearDepth (GLclampd depth);
GL_API void GL_APIENTRY glClearIndex (GLfloat c);
GL_API void GL_APIENTRY glClearStencil (GLint s);
GL_API void GL_APIENTRY glClearDeltaIMG (GLfloat x);
GL_API void GL_APIENTRY glClientActiveTexture (GLenum texture);
GL_API void GL_APIENTRY glClientActiveTextureARB (GLenum texture);
GL_API void GL_APIENTRY glClipPlane (GLenum plane, const GLdouble *equation);
GL_API void GL_APIENTRY glColor3b (GLbyte red, GLbyte green, GLbyte blue);
GL_API void GL_APIENTRY glColor3bv (const GLbyte *v);
GL_API void GL_APIENTRY glColor3d (GLdouble red, GLdouble green, GLdouble blue);
GL_API void GL_APIENTRY glColor3dv (const GLdouble *v);
GL_API void GL_APIENTRY glColor3f (GLfloat red, GLfloat green, GLfloat blue);
GL_API void GL_APIENTRY glColor3fv (const GLfloat *v);
GL_API void GL_APIENTRY glColor3i (GLint red, GLint green, GLint blue);
GL_API void GL_APIENTRY glColor3iv (const GLint *v);
GL_API void GL_APIENTRY glColor3s (GLshort red, GLshort green, GLshort blue);
GL_API void GL_APIENTRY glColor3sv (const GLshort *v);
GL_API void GL_APIENTRY glColor3ub (GLubyte red, GLubyte green, GLubyte blue);
GL_API void GL_APIENTRY glColor3ubv (const GLubyte *v);
GL_API void GL_APIENTRY glColor3ui (GLuint red, GLuint green, GLuint blue);
GL_API void GL_APIENTRY glColor3uiv (const GLuint *v);
GL_API void GL_APIENTRY glColor3us (GLushort red, GLushort green, GLushort blue);
GL_API void GL_APIENTRY glColor3usv (const GLushort *v);
GL_API void GL_APIENTRY glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
GL_API void GL_APIENTRY glColor4bv (const GLbyte *v);
GL_API void GL_APIENTRY glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
GL_API void GL_APIENTRY glColor4dv (const GLdouble *v);
GL_API void GL_APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
GL_API void GL_APIENTRY glColor4fv (const GLfloat *v);
GL_API void GL_APIENTRY glColor4i (GLint red, GLint green, GLint blue, GLint alpha);
GL_API void GL_APIENTRY glColor4iv (const GLint *v);
GL_API void GL_APIENTRY glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha);
GL_API void GL_APIENTRY glColor4sv (const GLshort *v);
GL_API void GL_APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
GL_API void GL_APIENTRY glColor4ubv (const GLubyte *v);
GL_API void GL_APIENTRY glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha);
GL_API void GL_APIENTRY glColor4uiv (const GLuint *v);
GL_API void GL_APIENTRY glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha);
GL_API void GL_APIENTRY glColor4usv (const GLushort *v);
GL_API void GL_APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GL_API void GL_APIENTRY glColorMaterial (GLenum face, GLenum mode);
GL_API void GL_APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glCompileShader (GLuint shader);
GL_API void GL_APIENTRY glCompileShaderARB (GLhandleARB shaderObj);
GL_API void GL_APIENTRY glCompressedTexImage1D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
GL_API void GL_APIENTRY glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
GL_API void GL_APIENTRY glCompressedTexImage3D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
GL_API void GL_APIENTRY glCompressedTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
GL_API void GL_APIENTRY glCompressedTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
GL_API void GL_APIENTRY glCompressedTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
GL_API void GL_APIENTRY glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
GL_API void GL_APIENTRY glCopyTexImage1D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
GL_API void GL_APIENTRY glCopyTexImage2D (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GL_API void GL_APIENTRY glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
GL_API void GL_APIENTRY glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GL_API void GL_APIENTRY glCopyTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GL_API GLuint GL_APIENTRY glCreateProgram (void);
GL_API GLhandleARB GL_APIENTRY glCreateProgramObjectARB (void);
GL_API GLuint GL_APIENTRY glCreateShader (GLenum type);
GL_API GLhandleARB GL_APIENTRY glCreateShaderObjectARB (GLenum shaderType);
GL_API void GL_APIENTRY glCullFace (GLenum mode);
GL_API void GL_APIENTRY glCurrentPaletteMatrixARB (GLint index);
GL_API void GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers);
GL_API void GL_APIENTRY glDeleteBuffersARB (GLsizei n, const GLuint *buffers);
GL_API void GL_APIENTRY glDeleteFramebuffersEXT (GLsizei n, const GLuint *framebuffers);
GL_API void GL_APIENTRY glDeleteLists (GLuint list, GLsizei range);
GL_API void GL_APIENTRY glDeleteObjectARB (GLhandleARB obj);
GL_API void GL_APIENTRY glDeleteProgram (GLuint program);
GL_API void GL_APIENTRY glDeleteProgramsARB (GLsizei n, const GLuint *programs);
GL_API void GL_APIENTRY glDeleteQueries (GLsizei n, const GLuint *ids);
GL_API void GL_APIENTRY glDeleteQueriesARB (GLsizei n, const GLuint *ids);
GL_API void GL_APIENTRY glDeleteRenderbuffersEXT (GLsizei n, const GLuint *renderbuffers);
GL_API void GL_APIENTRY glDeleteShader (GLuint shader);
GL_API void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures);
GL_API void GL_APIENTRY glDepthFunc (GLenum func);
GL_API void GL_APIENTRY glDepthMask (GLboolean flag);
GL_API void GL_APIENTRY glDepthRange (GLclampd zNear, GLclampd zFar);
GL_API void GL_APIENTRY glDetachObjectARB (GLhandleARB containerObj, GLhandleARB attachedObj);
GL_API void GL_APIENTRY glDetachShader (GLuint program, GLuint shader);
GL_API void GL_APIENTRY glDisable (GLenum cap);
GL_API void GL_APIENTRY glDisableClientState (GLenum array);
GL_API void GL_APIENTRY glDisableVertexAttribArray (GLuint index);
GL_API void GL_APIENTRY glDisableVertexAttribArrayARB (GLuint index);
GL_API void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count);
GL_API void GL_APIENTRY glDrawBuffer (GLenum mode);
GL_API void GL_APIENTRY glDrawBuffers (GLsizei n, const GLenum *bufs);
GL_API void GL_APIENTRY glDrawBuffersARB (GLsizei n, const GLenum *bufs);
GL_API void GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GL_API void GL_APIENTRY glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glDrawRangeElements (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
GL_API void GL_APIENTRY glEdgeFlag (GLboolean flag);
GL_API void GL_APIENTRY glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glEdgeFlagv (const GLboolean *flag);
GL_API void GL_APIENTRY glEnable (GLenum cap);
GL_API void GL_APIENTRY glEnableClientState (GLenum array);
GL_API void GL_APIENTRY glEnableVertexAttribArray (GLuint index);
GL_API void GL_APIENTRY glEnableVertexAttribArrayARB (GLuint index);
GL_API void GL_APIENTRY glEnd (void);
GL_API void GL_APIENTRY glEndList (void);
GL_API void GL_APIENTRY glEndQuery (GLenum target);
GL_API void GL_APIENTRY glEndQueryARB (GLenum target);
GL_API void GL_APIENTRY glEvalCoord1d (GLdouble u);
GL_API void GL_APIENTRY glEvalCoord1dv (const GLdouble *u);
GL_API void GL_APIENTRY glEvalCoord1f (GLfloat u);
GL_API void GL_APIENTRY glEvalCoord1fv (const GLfloat *u);
GL_API void GL_APIENTRY glEvalCoord2d (GLdouble u, GLdouble v);
GL_API void GL_APIENTRY glEvalCoord2dv (const GLdouble *u);
GL_API void GL_APIENTRY glEvalCoord2f (GLfloat u, GLfloat v);
GL_API void GL_APIENTRY glEvalCoord2fv (const GLfloat *u);
GL_API void GL_APIENTRY glEvalMesh1 (GLenum mode, GLint i1, GLint i2);
GL_API void GL_APIENTRY glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
GL_API void GL_APIENTRY glEvalPoint1 (GLint i);
GL_API void GL_APIENTRY glEvalPoint2 (GLint i, GLint j);
GL_API void GL_APIENTRY glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer);
GL_API void GL_APIENTRY glFinish (void);
GL_API void GL_APIENTRY glFlush (void);
GL_API void GL_APIENTRY glFogCoordPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glFogCoordd (GLdouble coord);
GL_API void GL_APIENTRY glFogCoorddv (const GLdouble *coord);
GL_API void GL_APIENTRY glFogCoordf (GLfloat coord);
GL_API void GL_APIENTRY glFogCoordfv (const GLfloat *coord);
GL_API void GL_APIENTRY glFogf (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glFogfv (GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glFogi (GLenum pname, GLint param);
GL_API void GL_APIENTRY glFogiv (GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glFramebufferRenderbufferEXT (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
GL_API void GL_APIENTRY glFramebufferTexture1DEXT (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_API void GL_APIENTRY glFramebufferTexture2DEXT (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GL_API void GL_APIENTRY glFramebufferTexture3DEXT (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
GL_API void GL_APIENTRY glFrontFace (GLenum mode);
GL_API void GL_APIENTRY glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GL_API void GL_APIENTRY glGenBuffers (GLsizei n, GLuint *buffers);
GL_API void GL_APIENTRY glGenBuffersARB (GLsizei n, GLuint *buffers);
GL_API void GL_APIENTRY glGenFramebuffersEXT (GLsizei n, GLuint *framebuffers);
GL_API GLuint GL_APIENTRY glGenLists (GLsizei range);
GL_API void GL_APIENTRY glGenProgramsARB (GLsizei n, GLuint *programs);
GL_API void GL_APIENTRY glGenQueries (GLsizei n, GLuint *ids);
GL_API void GL_APIENTRY glGenQueriesARB (GLsizei n, GLuint *ids);
GL_API void GL_APIENTRY glGenRenderbuffersEXT (GLsizei n, GLuint *renderbuffers);
GL_API void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures);
GL_API void GL_APIENTRY glGenerateMipmapEXT (GLenum target);
GL_API void GL_APIENTRY glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GL_API void GL_APIENTRY glGetActiveAttribARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GL_API void GL_APIENTRY glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GL_API void GL_APIENTRY glGetActiveUniformARB (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GL_API void GL_APIENTRY glGetAttachedObjectsARB (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
GL_API void GL_APIENTRY glGetAttachedShaders (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *obj);
GL_API GLint GL_APIENTRY glGetAttribLocation (GLuint program, const GLcharARB *name);
GL_API GLint GL_APIENTRY glGetAttribLocationARB (GLhandleARB programObj, const GLcharARB *name);
GL_API void GL_APIENTRY glGetBooleanv (GLenum pname, GLboolean *params);
GL_API void GL_APIENTRY glGetBufferParameteriv (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetBufferParameterivARB (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetBufferPointerv (GLenum target, GLenum pname, GLvoid* *params);
GL_API void GL_APIENTRY glGetBufferPointervARB (GLenum target, GLenum pname, GLvoid* *params);
GL_API void GL_APIENTRY glGetBufferSubData (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
GL_API void GL_APIENTRY glGetBufferSubDataARB (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
GL_API void GL_APIENTRY glGetClipPlane (GLenum plane, GLdouble *equation);
GL_API void GL_APIENTRY glGetCompressedTexImage (GLenum target, GLint level, GLvoid *img);
GL_API void GL_APIENTRY glGetDoublev (GLenum pname, GLdouble *params);
GL_API GLenum GL_APIENTRY glGetError (void);
GL_API void GL_APIENTRY glGetFloatv (GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivEXT (GLenum target, GLenum attachment, GLenum pname, GLint *params);
GL_API GLhandleARB GL_APIENTRY glGetHandleARB (GLenum pname);
GL_API void GL_APIENTRY glGetInfoLogARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
GL_API void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetLightiv (GLenum light, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetMapdv (GLenum target, GLenum query, GLdouble *v);
GL_API void GL_APIENTRY glGetMapfv (GLenum target, GLenum query, GLfloat *v);
GL_API void GL_APIENTRY glGetMapiv (GLenum target, GLenum query, GLint *v);
GL_API void GL_APIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetMaterialiv (GLenum face, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetObjectParameterfvARB (GLhandleARB obj, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetObjectParameterivARB (GLhandleARB obj, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetPixelMapfv (GLenum map, GLfloat *values);
GL_API void GL_APIENTRY glGetPixelMapuiv (GLenum map, GLuint *values);
GL_API void GL_APIENTRY glGetPixelMapusv (GLenum map, GLushort *values);
GL_API void GL_APIENTRY glGetPointerv (GLenum pname, GLvoid* *params);
GL_API void GL_APIENTRY glGetPolygonStipple (GLubyte *mask);
GL_API void GL_APIENTRY glGetProgramEnvParameterdvARB (GLenum target, GLuint index, GLdouble *params);
GL_API void GL_APIENTRY glGetProgramEnvParameterfvARB (GLenum target, GLuint index, GLfloat *params);
GL_API void GL_APIENTRY glGetProgramInfoLog (GLuint program, GLsizei bufSize, GLsizei *length, GLcharARB *infoLog);
GL_API void GL_APIENTRY glGetProgramLocalParameterdvARB (GLenum target, GLuint index, GLdouble *params);
GL_API void GL_APIENTRY glGetProgramLocalParameterfvARB (GLenum target, GLuint index, GLfloat *params);
GL_API void GL_APIENTRY glGetProgramStringARB (GLenum target, GLenum pname, GLvoid *string);
GL_API void GL_APIENTRY glGetProgramiv (GLuint program, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetProgramivARB (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetQueryObjectiv (GLuint id, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetQueryObjectivARB (GLuint id, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetQueryObjectuiv (GLuint id, GLenum pname, GLuint *params);
GL_API void GL_APIENTRY glGetQueryObjectuivARB (GLuint id, GLenum pname, GLuint *params);
GL_API void GL_APIENTRY glGetQueryiv (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetQueryivARB (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetRenderbufferParameterivEXT (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetShaderInfoLog (GLuint shader, GLsizei bufSize, GLsizei *length, GLcharARB *infoLog);
GL_API void GL_APIENTRY glGetShaderSource (GLuint shader, GLsizei bufSize, GLsizei *length, GLcharARB *source);
GL_API void GL_APIENTRY glGetShaderSourceARB (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
GL_API void GL_APIENTRY glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
GL_API const GLubyte * GL_APIENTRY glGetString (GLenum name);
GL_API void GL_APIENTRY glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetTexEnviv (GLenum target, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params);
GL_API void GL_APIENTRY glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetTexGeniv (GLenum coord, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
GL_API void GL_APIENTRY glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetTexParameteriv (GLenum target, GLenum pname, GLint *params);
GL_API GLint GL_APIENTRY glGetUniformLocation (GLuint program, const GLcharARB *name);
GL_API GLint GL_APIENTRY glGetUniformLocationARB (GLhandleARB programObj, const GLcharARB *name);
GL_API void GL_APIENTRY glGetUniformfv (GLuint program, GLint location, GLfloat *params);
GL_API void GL_APIENTRY glGetUniformfvARB (GLhandleARB programObj, GLint location, GLfloat *params);
GL_API void GL_APIENTRY glGetUniformiv (GLuint program, GLint location, GLint *params);
GL_API void GL_APIENTRY glGetUniformivARB (GLhandleARB programObj, GLint location, GLint *params);
GL_API void GL_APIENTRY glGetVertexAttribPointerv (GLuint index, GLenum pname, GLvoid* *pointer);
GL_API void GL_APIENTRY glGetVertexAttribPointervARB (GLuint index, GLenum pname, GLvoid* *pointer);
GL_API void GL_APIENTRY glGetVertexAttribdv (GLuint index, GLenum pname, GLdouble *params);
GL_API void GL_APIENTRY glGetVertexAttribdvARB (GLuint index, GLenum pname, GLdouble *params);
GL_API void GL_APIENTRY glGetVertexAttribfv (GLuint index, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetVertexAttribfvARB (GLuint index, GLenum pname, GLfloat *params);
GL_API void GL_APIENTRY glGetVertexAttribiv (GLuint index, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glGetVertexAttribivARB (GLuint index, GLenum pname, GLint *params);
GL_API void GL_APIENTRY glHint (GLenum target, GLenum mode);
GL_API void GL_APIENTRY glIndexMask (GLuint mask);
GL_API void GL_APIENTRY glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glIndexd (GLdouble c);
GL_API void GL_APIENTRY glIndexdv (const GLdouble *c);
GL_API void GL_APIENTRY glIndexf (GLfloat c);
GL_API void GL_APIENTRY glIndexfv (const GLfloat *c);
GL_API void GL_APIENTRY glIndexi (GLint c);
GL_API void GL_APIENTRY glIndexiv (const GLint *c);
GL_API void GL_APIENTRY glIndexs (GLshort c);
GL_API void GL_APIENTRY glIndexsv (const GLshort *c);
GL_API void GL_APIENTRY glIndexub (GLubyte c);
GL_API void GL_APIENTRY glIndexubv (const GLubyte *c);
GL_API void GL_APIENTRY glInitNames (void);
GL_API void GL_APIENTRY glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer);
GL_API GLboolean GL_APIENTRY glIsBuffer (GLuint buffer);
GL_API GLboolean GL_APIENTRY glIsBufferARB (GLuint buffer);
GL_API GLboolean GL_APIENTRY glIsEnabled (GLenum cap);
GL_API GLboolean GL_APIENTRY glIsFramebufferEXT (GLuint framebuffer);
GL_API GLboolean GL_APIENTRY glIsList (GLuint list);
GL_API GLboolean GL_APIENTRY glIsProgram (GLuint program);
GL_API GLboolean GL_APIENTRY glIsProgramARB (GLuint program);
GL_API GLboolean GL_APIENTRY glIsQuery (GLuint id);
GL_API GLboolean GL_APIENTRY glIsQueryARB (GLuint id);
GL_API GLboolean GL_APIENTRY glIsRenderbufferEXT (GLuint renderbuffer);
GL_API GLboolean GL_APIENTRY glIsShader (GLuint shader);
GL_API GLboolean GL_APIENTRY glIsTexture (GLuint texture);
GL_API void GL_APIENTRY glLightModelf (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glLightModelfv (GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glLightModeli (GLenum pname, GLint param);
GL_API void GL_APIENTRY glLightModeliv (GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glLightf (GLenum light, GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glLighti (GLenum light, GLenum pname, GLint param);
GL_API void GL_APIENTRY glLightiv (GLenum light, GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glLineParameterfIMG (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glLineParameterfvIMG (GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glLineParameteriIMG (GLenum pname, GLint param);
GL_API void GL_APIENTRY glLineParameterivIMG (GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glLineStipple (GLint factor, GLushort pattern);
GL_API void GL_APIENTRY glLineWidth (GLfloat width);
GL_API void GL_APIENTRY glLinkProgram (GLuint program);
GL_API void GL_APIENTRY glLinkProgramARB (GLhandleARB programObj);
GL_API void GL_APIENTRY glListBase (GLuint base);
GL_API void GL_APIENTRY glLoadIdentity (void);
GL_API void GL_APIENTRY glLoadMatrixd (const GLdouble *m);
GL_API void GL_APIENTRY glLoadMatrixf (const GLfloat *m);
GL_API void GL_APIENTRY glLoadName (GLuint name);
GL_API void GL_APIENTRY glLoadTransposeMatrixd (const GLdouble *m);
GL_API void GL_APIENTRY glLoadTransposeMatrixf (const GLfloat *m);
GL_API void GL_APIENTRY glLockArraysEXT (GLint first, GLsizei count);
GL_API void GL_APIENTRY glLogicOp (GLenum opcode);
GL_API void GL_APIENTRY glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
GL_API void GL_APIENTRY glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
GL_API void GL_APIENTRY glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
GL_API void GL_APIENTRY glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
GL_API GLvoid* GL_APIENTRY glMapBuffer (GLenum target, GLenum access);
GL_API GLvoid* GL_APIENTRY glMapBufferARB (GLenum target, GLenum access);
GL_API void GL_APIENTRY glMapGrid1d (GLint un, GLdouble u1, GLdouble u2);
GL_API void GL_APIENTRY glMapGrid1f (GLint un, GLfloat u1, GLfloat u2);
GL_API void GL_APIENTRY glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
GL_API void GL_APIENTRY glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
GL_API void GL_APIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glMateriali (GLenum face, GLenum pname, GLint param);
GL_API void GL_APIENTRY glMaterialiv (GLenum face, GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glMatrixIndexPointerARB (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glMatrixIndexubvARB (GLint size, const GLubyte *indices);
GL_API void GL_APIENTRY glMatrixIndexuivARB (GLint size, const GLuint *indices);
GL_API void GL_APIENTRY glMatrixIndexusvARB (GLint size, const GLushort *indices);
GL_API void GL_APIENTRY glMatrixMode (GLenum mode);
GL_API void GL_APIENTRY glMultMatrixd (const GLdouble *m);
GL_API void GL_APIENTRY glMultMatrixf (const GLfloat *m);
GL_API void GL_APIENTRY glMultTransposeMatrixd (const GLdouble *m);
GL_API void GL_APIENTRY glMultTransposeMatrixf (const GLfloat *m);
GL_API void GL_APIENTRY glMultiDrawArrays (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
GL_API void GL_APIENTRY glMultiDrawElements (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
GL_API void GL_APIENTRY glMultiTexCoord1d (GLenum target, GLdouble s);
GL_API void GL_APIENTRY glMultiTexCoord1dARB (GLenum target, GLdouble s);
GL_API void GL_APIENTRY glMultiTexCoord1dv (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord1dvARB (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord1f (GLenum target, GLfloat s);
GL_API void GL_APIENTRY glMultiTexCoord1fARB (GLenum target, GLfloat s);
GL_API void GL_APIENTRY glMultiTexCoord1fv (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord1fvARB (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord1i (GLenum target, GLint s);
GL_API void GL_APIENTRY glMultiTexCoord1iARB (GLenum target, GLint s);
GL_API void GL_APIENTRY glMultiTexCoord1iv (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord1ivARB (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord1s (GLenum target, GLshort s);
GL_API void GL_APIENTRY glMultiTexCoord1sARB (GLenum target, GLshort s);
GL_API void GL_APIENTRY glMultiTexCoord1sv (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord1svARB (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord2d (GLenum target, GLdouble s, GLdouble t);
GL_API void GL_APIENTRY glMultiTexCoord2dARB (GLenum target, GLdouble s, GLdouble t);
GL_API void GL_APIENTRY glMultiTexCoord2dv (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord2dvARB (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord2f (GLenum target, GLfloat s, GLfloat t);
GL_API void GL_APIENTRY glMultiTexCoord2fARB (GLenum target, GLfloat s, GLfloat t);
GL_API void GL_APIENTRY glMultiTexCoord2fv (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord2fvARB (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord2i (GLenum target, GLint s, GLint t);
GL_API void GL_APIENTRY glMultiTexCoord2iARB (GLenum target, GLint s, GLint t);
GL_API void GL_APIENTRY glMultiTexCoord2iv (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord2ivARB (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord2s (GLenum target, GLshort s, GLshort t);
GL_API void GL_APIENTRY glMultiTexCoord2sARB (GLenum target, GLshort s, GLshort t);
GL_API void GL_APIENTRY glMultiTexCoord2sv (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord2svARB (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord3d (GLenum target, GLdouble s, GLdouble t, GLdouble r);
GL_API void GL_APIENTRY glMultiTexCoord3dARB (GLenum target, GLdouble s, GLdouble t, GLdouble r);
GL_API void GL_APIENTRY glMultiTexCoord3dv (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord3dvARB (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord3f (GLenum target, GLfloat s, GLfloat t, GLfloat r);
GL_API void GL_APIENTRY glMultiTexCoord3fARB (GLenum target, GLfloat s, GLfloat t, GLfloat r);
GL_API void GL_APIENTRY glMultiTexCoord3fv (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord3fvARB (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord3i (GLenum target, GLint s, GLint t, GLint r);
GL_API void GL_APIENTRY glMultiTexCoord3iARB (GLenum target, GLint s, GLint t, GLint r);
GL_API void GL_APIENTRY glMultiTexCoord3iv (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord3ivARB (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord3s (GLenum target, GLshort s, GLshort t, GLshort r);
GL_API void GL_APIENTRY glMultiTexCoord3sARB (GLenum target, GLshort s, GLshort t, GLshort r);
GL_API void GL_APIENTRY glMultiTexCoord3sv (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord3svARB (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord4d (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GL_API void GL_APIENTRY glMultiTexCoord4dARB (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GL_API void GL_APIENTRY glMultiTexCoord4dv (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord4dvARB (GLenum target, const GLdouble *v);
GL_API void GL_APIENTRY glMultiTexCoord4f (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GL_API void GL_APIENTRY glMultiTexCoord4fARB (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GL_API void GL_APIENTRY glMultiTexCoord4fv (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord4fvARB (GLenum target, const GLfloat *v);
GL_API void GL_APIENTRY glMultiTexCoord4i (GLenum target, GLint s, GLint t, GLint r, GLint q);
GL_API void GL_APIENTRY glMultiTexCoord4iARB (GLenum target, GLint s, GLint t, GLint r, GLint q);
GL_API void GL_APIENTRY glMultiTexCoord4iv (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord4ivARB (GLenum target, const GLint *v);
GL_API void GL_APIENTRY glMultiTexCoord4s (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GL_API void GL_APIENTRY glMultiTexCoord4sARB (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GL_API void GL_APIENTRY glMultiTexCoord4sv (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glMultiTexCoord4svARB (GLenum target, const GLshort *v);
GL_API void GL_APIENTRY glNewList (GLuint list, GLenum mode);
GL_API void GL_APIENTRY glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz);
GL_API void GL_APIENTRY glNormal3bv (const GLbyte *v);
GL_API void GL_APIENTRY glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz);
GL_API void GL_APIENTRY glNormal3dv (const GLdouble *v);
GL_API void GL_APIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz);
GL_API void GL_APIENTRY glNormal3fv (const GLfloat *v);
GL_API void GL_APIENTRY glNormal3i (GLint nx, GLint ny, GLint nz);
GL_API void GL_APIENTRY glNormal3iv (const GLint *v);
GL_API void GL_APIENTRY glNormal3s (GLshort nx, GLshort ny, GLshort nz);
GL_API void GL_APIENTRY glNormal3sv (const GLshort *v);
GL_API void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GL_API void GL_APIENTRY glPassThrough (GLfloat token);
GL_API void GL_APIENTRY glPixelMapfv (GLenum map, GLint mapsize, const GLfloat *values);
GL_API void GL_APIENTRY glPixelMapuiv (GLenum map, GLint mapsize, const GLuint *values);
GL_API void GL_APIENTRY glPixelMapusv (GLenum map, GLint mapsize, const GLushort *values);
GL_API void GL_APIENTRY glPixelStoref (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glPixelStorei (GLenum pname, GLint param);
GL_API void GL_APIENTRY glPixelTransferf (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glPixelTransferi (GLenum pname, GLint param);
GL_API void GL_APIENTRY glPixelZoom (GLfloat xfactor, GLfloat yfactor);
GL_API void GL_APIENTRY glPointParameterf (GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glPointParameterfv (GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glPointParameteri (GLenum pname, GLint param);
GL_API void GL_APIENTRY glPointParameteriv (GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glPointSize (GLfloat size);
GL_API void GL_APIENTRY glPolygonMode (GLenum face, GLenum mode);
GL_API void GL_APIENTRY glPolygonOffset (GLfloat factor, GLfloat units);
GL_API void GL_APIENTRY glPolygonStipple (const GLubyte *mask);
GL_API void GL_APIENTRY glPopAttrib (void);
GL_API void GL_APIENTRY glPopClientAttrib (void);
GL_API void GL_APIENTRY glPopMatrix (void);
GL_API void GL_APIENTRY glPopName (void);
GL_API void GL_APIENTRY glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities);
GL_API void GL_APIENTRY glProgramEnvParameter4dARB (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GL_API void GL_APIENTRY glProgramEnvParameter4dvARB (GLenum target, GLuint index, const GLdouble *params);
GL_API void GL_APIENTRY glProgramEnvParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_API void GL_APIENTRY glProgramEnvParameter4fvARB (GLenum target, GLuint index, const GLfloat *params);
GL_API void GL_APIENTRY glProgramLocalParameter4dARB (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GL_API void GL_APIENTRY glProgramLocalParameter4dvARB (GLenum target, GLuint index, const GLdouble *params);
GL_API void GL_APIENTRY glProgramLocalParameter4fARB (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_API void GL_APIENTRY glProgramLocalParameter4fvARB (GLenum target, GLuint index, const GLfloat *params);
GL_API void GL_APIENTRY glProgramStringARB (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
GL_API void GL_APIENTRY glPushAttrib (GLbitfield mask);
GL_API void GL_APIENTRY glPushClientAttrib (GLbitfield mask);
GL_API void GL_APIENTRY glPushMatrix (void);
GL_API void GL_APIENTRY glPushName (GLuint name);
GL_API void GL_APIENTRY glRasterPos2d (GLdouble x, GLdouble y);
GL_API void GL_APIENTRY glRasterPos2dv (const GLdouble *v);
GL_API void GL_APIENTRY glRasterPos2f (GLfloat x, GLfloat y);
GL_API void GL_APIENTRY glRasterPos2fv (const GLfloat *v);
GL_API void GL_APIENTRY glRasterPos2i (GLint x, GLint y);
GL_API void GL_APIENTRY glRasterPos2iv (const GLint *v);
GL_API void GL_APIENTRY glRasterPos2s (GLshort x, GLshort y);
GL_API void GL_APIENTRY glRasterPos2sv (const GLshort *v);
GL_API void GL_APIENTRY glRasterPos3d (GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glRasterPos3dv (const GLdouble *v);
GL_API void GL_APIENTRY glRasterPos3f (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glRasterPos3fv (const GLfloat *v);
GL_API void GL_APIENTRY glRasterPos3i (GLint x, GLint y, GLint z);
GL_API void GL_APIENTRY glRasterPos3iv (const GLint *v);
GL_API void GL_APIENTRY glRasterPos3s (GLshort x, GLshort y, GLshort z);
GL_API void GL_APIENTRY glRasterPos3sv (const GLshort *v);
GL_API void GL_APIENTRY glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GL_API void GL_APIENTRY glRasterPos4dv (const GLdouble *v);
GL_API void GL_APIENTRY glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_API void GL_APIENTRY glRasterPos4fv (const GLfloat *v);
GL_API void GL_APIENTRY glRasterPos4i (GLint x, GLint y, GLint z, GLint w);
GL_API void GL_APIENTRY glRasterPos4iv (const GLint *v);
GL_API void GL_APIENTRY glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w);
GL_API void GL_APIENTRY glRasterPos4sv (const GLshort *v);
GL_API void GL_APIENTRY glReadBuffer (GLenum mode);
GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
GL_API void GL_APIENTRY glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
GL_API void GL_APIENTRY glRectdv (const GLdouble *v1, const GLdouble *v2);
GL_API void GL_APIENTRY glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
GL_API void GL_APIENTRY glRectfv (const GLfloat *v1, const GLfloat *v2);
GL_API void GL_APIENTRY glRecti (GLint x1, GLint y1, GLint x2, GLint y2);
GL_API void GL_APIENTRY glRectiv (const GLint *v1, const GLint *v2);
GL_API void GL_APIENTRY glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
GL_API void GL_APIENTRY glRectsv (const GLshort *v1, const GLshort *v2);
GL_API GLint GL_APIENTRY glRenderMode (GLenum mode);
GL_API void GL_APIENTRY glRenderbufferStorageEXT (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
GL_API void GL_APIENTRY glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glSampleCoverage (GLclampf value, GLboolean invert);
GL_API void GL_APIENTRY glScaled (GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height);
GL_API void GL_APIENTRY glSecondaryColor3b (GLbyte red, GLbyte green, GLbyte blue);
GL_API void GL_APIENTRY glSecondaryColor3bEXT (GLbyte red, GLbyte green, GLbyte blue);
GL_API void GL_APIENTRY glSecondaryColor3bv (const GLbyte *v);
GL_API void GL_APIENTRY glSecondaryColor3bvEXT (const GLbyte *v);
GL_API void GL_APIENTRY glSecondaryColor3d (GLdouble red, GLdouble green, GLdouble blue);
GL_API void GL_APIENTRY glSecondaryColor3dEXT (GLdouble red, GLdouble green, GLdouble blue);
GL_API void GL_APIENTRY glSecondaryColor3dv (const GLdouble *v);
GL_API void GL_APIENTRY glSecondaryColor3dvEXT (const GLdouble *v);
GL_API void GL_APIENTRY glSecondaryColor3f (GLfloat red, GLfloat green, GLfloat blue);
GL_API void GL_APIENTRY glSecondaryColor3fEXT (GLfloat red, GLfloat green, GLfloat blue);
GL_API void GL_APIENTRY glSecondaryColor3fv (const GLfloat *v);
GL_API void GL_APIENTRY glSecondaryColor3fvEXT (const GLfloat *v);
GL_API void GL_APIENTRY glSecondaryColor3i (GLint red, GLint green, GLint blue);
GL_API void GL_APIENTRY glSecondaryColor3iEXT (GLint red, GLint green, GLint blue);
GL_API void GL_APIENTRY glSecondaryColor3iv (const GLint *v);
GL_API void GL_APIENTRY glSecondaryColor3ivEXT (const GLint *v);
GL_API void GL_APIENTRY glSecondaryColor3s (GLshort red, GLshort green, GLshort blue);
GL_API void GL_APIENTRY glSecondaryColor3sEXT (GLshort red, GLshort green, GLshort blue);
GL_API void GL_APIENTRY glSecondaryColor3sv (const GLshort *v);
GL_API void GL_APIENTRY glSecondaryColor3svEXT (const GLshort *v);
GL_API void GL_APIENTRY glSecondaryColor3ub (GLubyte red, GLubyte green, GLubyte blue);
GL_API void GL_APIENTRY glSecondaryColor3ubEXT (GLubyte red, GLubyte green, GLubyte blue);
GL_API void GL_APIENTRY glSecondaryColor3ubv (const GLubyte *v);
GL_API void GL_APIENTRY glSecondaryColor3ubvEXT (const GLubyte *v);
GL_API void GL_APIENTRY glSecondaryColor3ui (GLuint red, GLuint green, GLuint blue);
GL_API void GL_APIENTRY glSecondaryColor3uiEXT (GLuint red, GLuint green, GLuint blue);
GL_API void GL_APIENTRY glSecondaryColor3uiv (const GLuint *v);
GL_API void GL_APIENTRY glSecondaryColor3uivEXT (const GLuint *v);
GL_API void GL_APIENTRY glSecondaryColor3us (GLushort red, GLushort green, GLushort blue);
GL_API void GL_APIENTRY glSecondaryColor3usEXT (GLushort red, GLushort green, GLushort blue);
GL_API void GL_APIENTRY glSecondaryColor3usv (const GLushort *v);
GL_API void GL_APIENTRY glSecondaryColor3usvEXT (const GLushort *v);
GL_API void GL_APIENTRY glSecondaryColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glSecondaryColorPointerEXT (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glSelectBuffer (GLsizei size, GLuint *buffer);
GL_API void GL_APIENTRY glShadeModel (GLenum mode);
GL_API void GL_APIENTRY glShaderBinaryIMG (GLint n, const GLuint *shaders, GLenum binaryformat, const GLvoid *binary, GLint length);
GL_API void GL_APIENTRY glShaderSource (GLuint shader, GLsizei count, const GLcharARB* *string, const GLint *length);
GL_API void GL_APIENTRY glShaderSourceARB (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
GL_API void GL_APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask);
GL_API void GL_APIENTRY glStencilFuncSeparate (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
GL_API void GL_APIENTRY glStencilMask (GLuint mask);
GL_API void GL_APIENTRY glStencilMaskSeparate (GLenum face, GLuint mask);
GL_API void GL_APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass);
GL_API void GL_APIENTRY glStencilOpSeparate (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
GL_API void GL_APIENTRY glTexCoord1d (GLdouble s);
GL_API void GL_APIENTRY glTexCoord1dv (const GLdouble *v);
GL_API void GL_APIENTRY glTexCoord1f (GLfloat s);
GL_API void GL_APIENTRY glTexCoord1fv (const GLfloat *v);
GL_API void GL_APIENTRY glTexCoord1i (GLint s);
GL_API void GL_APIENTRY glTexCoord1iv (const GLint *v);
GL_API void GL_APIENTRY glTexCoord1s (GLshort s);
GL_API void GL_APIENTRY glTexCoord1sv (const GLshort *v);
GL_API void GL_APIENTRY glTexCoord2d (GLdouble s, GLdouble t);
GL_API void GL_APIENTRY glTexCoord2dv (const GLdouble *v);
GL_API void GL_APIENTRY glTexCoord2f (GLfloat s, GLfloat t);
GL_API void GL_APIENTRY glTexCoord2fv (const GLfloat *v);
GL_API void GL_APIENTRY glTexCoord2i (GLint s, GLint t);
GL_API void GL_APIENTRY glTexCoord2iv (const GLint *v);
GL_API void GL_APIENTRY glTexCoord2s (GLshort s, GLshort t);
GL_API void GL_APIENTRY glTexCoord2sv (const GLshort *v);
GL_API void GL_APIENTRY glTexCoord3d (GLdouble s, GLdouble t, GLdouble r);
GL_API void GL_APIENTRY glTexCoord3dv (const GLdouble *v);
GL_API void GL_APIENTRY glTexCoord3f (GLfloat s, GLfloat t, GLfloat r);
GL_API void GL_APIENTRY glTexCoord3fv (const GLfloat *v);
GL_API void GL_APIENTRY glTexCoord3i (GLint s, GLint t, GLint r);
GL_API void GL_APIENTRY glTexCoord3iv (const GLint *v);
GL_API void GL_APIENTRY glTexCoord3s (GLshort s, GLshort t, GLshort r);
GL_API void GL_APIENTRY glTexCoord3sv (const GLshort *v);
GL_API void GL_APIENTRY glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GL_API void GL_APIENTRY glTexCoord4dv (const GLdouble *v);
GL_API void GL_APIENTRY glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GL_API void GL_APIENTRY glTexCoord4fv (const GLfloat *v);
GL_API void GL_APIENTRY glTexCoord4i (GLint s, GLint t, GLint r, GLint q);
GL_API void GL_APIENTRY glTexCoord4iv (const GLint *v);
GL_API void GL_APIENTRY glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q);
GL_API void GL_APIENTRY glTexCoord4sv (const GLshort *v);
GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param);
GL_API void GL_APIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glTexGend (GLenum coord, GLenum pname, GLdouble param);
GL_API void GL_APIENTRY glTexGendv (GLenum coord, GLenum pname, const GLdouble *params);
GL_API void GL_APIENTRY glTexGenf (GLenum coord, GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param);
GL_API void GL_APIENTRY glTexGeniv (GLenum coord, GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param);
GL_API void GL_APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params);
GL_API void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param);
GL_API void GL_APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params);
GL_API void GL_APIENTRY glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
GL_API void GL_APIENTRY glTranslated (GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glUniform1f (GLint location, GLfloat v0);
GL_API void GL_APIENTRY glUniform1fARB (GLint location, GLfloat v0);
GL_API void GL_APIENTRY glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform1fvARB (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform1i (GLint location, GLint v0);
GL_API void GL_APIENTRY glUniform1iARB (GLint location, GLint v0);
GL_API void GL_APIENTRY glUniform1iv (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform1ivARB (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform2f (GLint location, GLfloat v0, GLfloat v1);
GL_API void GL_APIENTRY glUniform2fARB (GLint location, GLfloat v0, GLfloat v1);
GL_API void GL_APIENTRY glUniform2fv (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform2fvARB (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform2i (GLint location, GLint v0, GLint v1);
GL_API void GL_APIENTRY glUniform2iARB (GLint location, GLint v0, GLint v1);
GL_API void GL_APIENTRY glUniform2iv (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform2ivARB (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform3f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GL_API void GL_APIENTRY glUniform3fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GL_API void GL_APIENTRY glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform3fvARB (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform3i (GLint location, GLint v0, GLint v1, GLint v2);
GL_API void GL_APIENTRY glUniform3iARB (GLint location, GLint v0, GLint v1, GLint v2);
GL_API void GL_APIENTRY glUniform3iv (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform3ivARB (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GL_API void GL_APIENTRY glUniform4fARB (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GL_API void GL_APIENTRY glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform4fvARB (GLint location, GLsizei count, const GLfloat *value);
GL_API void GL_APIENTRY glUniform4i (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GL_API void GL_APIENTRY glUniform4iARB (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GL_API void GL_APIENTRY glUniform4iv (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniform4ivARB (GLint location, GLsizei count, const GLint *value);
GL_API void GL_APIENTRY glUniformMatrix2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix2fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix2x3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix2x4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix3fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix3x2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix3x4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix4fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix4fvARB (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix4x2fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUniformMatrix4x3fv (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_API void GL_APIENTRY glUnlockArraysEXT (void);
GL_API GLboolean GL_APIENTRY glUnmapBuffer (GLenum target);
GL_API GLboolean GL_APIENTRY glUnmapBufferARB (GLenum target);
GL_API void GL_APIENTRY glUseProgram (GLuint program);
GL_API void GL_APIENTRY glUseProgramObjectARB (GLhandleARB programObj);
GL_API void GL_APIENTRY glValidateProgram (GLuint program);
GL_API void GL_APIENTRY glValidateProgramARB (GLhandleARB programObj);
GL_API void GL_APIENTRY glVertex2d (GLdouble x, GLdouble y);
GL_API void GL_APIENTRY glVertex2dv (const GLdouble *v);
GL_API void GL_APIENTRY glVertex2f (GLfloat x, GLfloat y);
GL_API void GL_APIENTRY glVertex2fv (const GLfloat *v);
GL_API void GL_APIENTRY glVertex2i (GLint x, GLint y);
GL_API void GL_APIENTRY glVertex2iv (const GLint *v);
GL_API void GL_APIENTRY glVertex2s (GLshort x, GLshort y);
GL_API void GL_APIENTRY glVertex2sv (const GLshort *v);
GL_API void GL_APIENTRY glVertex3d (GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glVertex3dv (const GLdouble *v);
GL_API void GL_APIENTRY glVertex3f (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glVertex3fv (const GLfloat *v);
GL_API void GL_APIENTRY glVertex3i (GLint x, GLint y, GLint z);
GL_API void GL_APIENTRY glVertex3iv (const GLint *v);
GL_API void GL_APIENTRY glVertex3s (GLshort x, GLshort y, GLshort z);
GL_API void GL_APIENTRY glVertex3sv (const GLshort *v);
GL_API void GL_APIENTRY glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GL_API void GL_APIENTRY glVertex4dv (const GLdouble *v);
GL_API void GL_APIENTRY glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_API void GL_APIENTRY glVertex4fv (const GLfloat *v);
GL_API void GL_APIENTRY glVertex4i (GLint x, GLint y, GLint z, GLint w);
GL_API void GL_APIENTRY glVertex4iv (const GLint *v);
GL_API void GL_APIENTRY glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w);
GL_API void GL_APIENTRY glVertex4sv (const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib1d (GLuint index, GLdouble x);
GL_API void GL_APIENTRY glVertexAttrib1dARB (GLuint index, GLdouble x);
GL_API void GL_APIENTRY glVertexAttrib1dv (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib1dvARB (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib1f (GLuint index, GLfloat x);
GL_API void GL_APIENTRY glVertexAttrib1fARB (GLuint index, GLfloat x);
GL_API void GL_APIENTRY glVertexAttrib1fv (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib1fvARB (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib1s (GLuint index, GLshort x);
GL_API void GL_APIENTRY glVertexAttrib1sARB (GLuint index, GLshort x);
GL_API void GL_APIENTRY glVertexAttrib1sv (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib1svARB (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib2d (GLuint index, GLdouble x, GLdouble y);
GL_API void GL_APIENTRY glVertexAttrib2dARB (GLuint index, GLdouble x, GLdouble y);
GL_API void GL_APIENTRY glVertexAttrib2dv (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib2dvARB (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib2f (GLuint index, GLfloat x, GLfloat y);
GL_API void GL_APIENTRY glVertexAttrib2fARB (GLuint index, GLfloat x, GLfloat y);
GL_API void GL_APIENTRY glVertexAttrib2fv (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib2fvARB (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib2s (GLuint index, GLshort x, GLshort y);
GL_API void GL_APIENTRY glVertexAttrib2sARB (GLuint index, GLshort x, GLshort y);
GL_API void GL_APIENTRY glVertexAttrib2sv (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib2svARB (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib3d (GLuint index, GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glVertexAttrib3dARB (GLuint index, GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glVertexAttrib3dv (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib3dvARB (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib3f (GLuint index, GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glVertexAttrib3fARB (GLuint index, GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glVertexAttrib3fv (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib3fvARB (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib3s (GLuint index, GLshort x, GLshort y, GLshort z);
GL_API void GL_APIENTRY glVertexAttrib3sARB (GLuint index, GLshort x, GLshort y, GLshort z);
GL_API void GL_APIENTRY glVertexAttrib3sv (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib3svARB (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib4Nbv (GLuint index, const GLbyte *v);
GL_API void GL_APIENTRY glVertexAttrib4NbvARB (GLuint index, const GLbyte *v);
GL_API void GL_APIENTRY glVertexAttrib4Niv (GLuint index, const GLint *v);
GL_API void GL_APIENTRY glVertexAttrib4NivARB (GLuint index, const GLint *v);
GL_API void GL_APIENTRY glVertexAttrib4Nsv (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib4NsvARB (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib4Nub (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GL_API void GL_APIENTRY glVertexAttrib4NubARB (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GL_API void GL_APIENTRY glVertexAttrib4Nubv (GLuint index, const GLubyte *v);
GL_API void GL_APIENTRY glVertexAttrib4NubvARB (GLuint index, const GLubyte *v);
GL_API void GL_APIENTRY glVertexAttrib4Nuiv (GLuint index, const GLuint *v);
GL_API void GL_APIENTRY glVertexAttrib4NuivARB (GLuint index, const GLuint *v);
GL_API void GL_APIENTRY glVertexAttrib4Nusv (GLuint index, const GLushort *v);
GL_API void GL_APIENTRY glVertexAttrib4NusvARB (GLuint index, const GLushort *v);
GL_API void GL_APIENTRY glVertexAttrib4bv (GLuint index, const GLbyte *v);
GL_API void GL_APIENTRY glVertexAttrib4bvARB (GLuint index, const GLbyte *v);
GL_API void GL_APIENTRY glVertexAttrib4d (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GL_API void GL_APIENTRY glVertexAttrib4dARB (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GL_API void GL_APIENTRY glVertexAttrib4dv (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib4dvARB (GLuint index, const GLdouble *v);
GL_API void GL_APIENTRY glVertexAttrib4f (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_API void GL_APIENTRY glVertexAttrib4fARB (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GL_API void GL_APIENTRY glVertexAttrib4fv (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib4fvARB (GLuint index, const GLfloat *v);
GL_API void GL_APIENTRY glVertexAttrib4iv (GLuint index, const GLint *v);
GL_API void GL_APIENTRY glVertexAttrib4ivARB (GLuint index, const GLint *v);
GL_API void GL_APIENTRY glVertexAttrib4s (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
GL_API void GL_APIENTRY glVertexAttrib4sARB (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
GL_API void GL_APIENTRY glVertexAttrib4sv (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib4svARB (GLuint index, const GLshort *v);
GL_API void GL_APIENTRY glVertexAttrib4ubv (GLuint index, const GLubyte *v);
GL_API void GL_APIENTRY glVertexAttrib4ubvARB (GLuint index, const GLubyte *v);
GL_API void GL_APIENTRY glVertexAttrib4uiv (GLuint index, const GLuint *v);
GL_API void GL_APIENTRY glVertexAttrib4uivARB (GLuint index, const GLuint *v);
GL_API void GL_APIENTRY glVertexAttrib4usv (GLuint index, const GLushort *v);
GL_API void GL_APIENTRY glVertexAttrib4usvARB (GLuint index, const GLushort *v);
GL_API void GL_APIENTRY glVertexAttribPointer (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glVertexAttribPointerARB (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glVertexBlendARB (GLint count);
GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height);
GL_API void GL_APIENTRY glWeightPointerARB (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GL_API void GL_APIENTRY glWeightbvARB (GLint size, const GLbyte *weights);
GL_API void GL_APIENTRY glWeightdvARB (GLint size, const GLdouble *weights);
GL_API void GL_APIENTRY glWeightfvARB (GLint size, const GLfloat *weights);
GL_API void GL_APIENTRY glWeightivARB (GLint size, const GLint *weights);
GL_API void GL_APIENTRY glWeightsvARB (GLint size, const GLshort *weights);
GL_API void GL_APIENTRY glWeightubvARB (GLint size, const GLubyte *weights);
GL_API void GL_APIENTRY glWeightuivARB (GLint size, const GLuint *weights);
GL_API void GL_APIENTRY glWeightusvARB (GLint size, const GLushort *weights);
GL_API void GL_APIENTRY glWindowPos2d (GLdouble x, GLdouble y);
GL_API void GL_APIENTRY glWindowPos2dv (const GLdouble *v);
GL_API void GL_APIENTRY glWindowPos2f (GLfloat x, GLfloat y);
GL_API void GL_APIENTRY glWindowPos2fv (const GLfloat *v);
GL_API void GL_APIENTRY glWindowPos2i (GLint x, GLint y);
GL_API void GL_APIENTRY glWindowPos2iv (const GLint *v);
GL_API void GL_APIENTRY glWindowPos2s (GLshort x, GLshort y);
GL_API void GL_APIENTRY glWindowPos2sv (const GLshort *v);
GL_API void GL_APIENTRY glWindowPos3d (GLdouble x, GLdouble y, GLdouble z);
GL_API void GL_APIENTRY glWindowPos3dv (const GLdouble *v);
GL_API void GL_APIENTRY glWindowPos3f (GLfloat x, GLfloat y, GLfloat z);
GL_API void GL_APIENTRY glWindowPos3fv (const GLfloat *v);
GL_API void GL_APIENTRY glWindowPos3i (GLint x, GLint y, GLint z);
GL_API void GL_APIENTRY glWindowPos3iv (const GLint *v);
GL_API void GL_APIENTRY glWindowPos3s (GLshort x, GLshort y, GLshort z);
GL_API void GL_APIENTRY glWindowPos3sv (const GLshort *v);
GL_API void GL_APIENTRY glDeltaFuncIMG (GLenum func, GLfloat ref);
GL_API void GL_APIENTRY glDeltaMaskIMG (GLboolean x);

#ifdef __cplusplus
}
#endif

#endif /* __gl_h_ */
