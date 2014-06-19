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

modules := pdsasm pdsasmlexer pdsasmparser

pdsasm_type := host_executable
pdsasm_src := main.c pdsdisasm.c
pdsasm_gensrc := pdsasmparser/pdsasm.tab.c pdsasmlexer/pdsasm.l.c
pdsasm_genheaders := pdsasmparser/pdsasm.tab.h
pdsasm_includes := include4 hwdefs $(THIS_DIR) \
 $(HOST_INTERMEDIATES)/pdsasmlexer \
 $(HOST_INTERMEDIATES)/pdsasmparser

pdsasmlexer_type := host_flex_lexer
pdsasmlexer_src := pdsasm.l
pdsasmlexer_flexflags := -d -i

pdsasmparser_type := host_bison_parser
pdsasmparser_src := pdsasm.y
pdsasmparser_bisonflags := --verbose -bpdsasm
