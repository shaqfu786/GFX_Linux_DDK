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

modules := useasm sgxsupport errata useparser uselexer cparser clexer uselink libuseasm libuseasm_host

######################
# libuseasm for host

libuseasm_host_type := host_static_library
libuseasm_host_target := libuseasm_host.a
libuseasm_host_src := \
 useasm.c \
 usedisasm.c \
 userutils.c \
 useopt.c \
 usetab.c \
 utils.c \
 specialregs.c \
 specialregs_vec.c

libuseasm_host_includes := \
 include4 hwdefs $(THIS_DIR) \
 $(HOST_INTERMEDIATES)/useasm \
 $(HOST_INTERMEDIATES)/sgxsupport \
 $(HOST_INTERMEDIATES)/errata \
 $(HOST_INTERMEDIATES)/useparser \
 $(HOST_INTERMEDIATES)/cparser

libuseasm_host_cflags := -DUSER

$(addprefix $(HOST_INTERMEDIATES)/libuseasm_host/,useasm.o usedisasm.o usetab.o main.o utils.o): \
 $(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h \
 $(HOST_INTERMEDIATES)/errata/errata.h

########################
# libuseasm for target

libuseasm_type := static_library
libuseasm_src := \
 useasm.c \
 userutils.c \
 useopt.c \
 usetab.c \
 utils.c \
 specialregs.c \
 specialregs_vec.c

libuseasm_includes := $(libuseasm_host_includes)

ifeq ($(BUILD),debug)
libuseasm_src += usedisasm.c
libuseasm_cflags += -DUSER
endif

$(addprefix $(TARGET_INTERMEDIATES)/libuseasm/,useasm.o usedisasm.o usetab.o main.o utils.o): \
 $(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h \
 $(HOST_INTERMEDIATES)/errata/errata.h

###########################
# other tools and headers

useasm_type := host_executable

useasm_src := \
 usetab.c \
 ctree.c \
 main.c \
 scopeman.c

useasm_gensrc := useparser/use.tab.c uselexer/use.l.c cparser/cparser.tab.c clexer/clexer.l.c
useasm_genheaders := useparser/use.tab.h cparser/cparser.tab.h
useasm_cflags := -DUSER
useasm_includes := $(libuseasm_host_includes)
useasm_staticlibs := useasm_host

useparser_type := host_bison_parser
useparser_src := use.y
useparser_bisonflags := --verbose

uselexer_type := host_flex_lexer
uselexer_src := use.l
uselexer_flexflags := -i

cparser_type := host_bison_parser
cparser_src := cparser.y
cparser_bisonflags := --verbose -pyz

clexer_type := host_flex_lexer
clexer_src := clexer.l
clexer_flexflags := -s -Pyz

# These things depend on the generated header sgxsupport.h
$(addprefix $(HOST_INTERMEDIATES)/useasm/,useasm.o usedisasm.o usetab.o main.o utils.o): \
 $(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h \
 $(HOST_INTERMEDIATES)/errata/errata.h

sgxsupport_type := host_custom

sgxsupport: $(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h
$(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h: THIS_DIR := $(THIS_DIR)
$(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h: | $(HOST_INTERMEDIATES)/sgxsupport
$(HOST_INTERMEDIATES)/sgxsupport/sgxsupport.h: $(THIS_DIR)/gensgxsupport.gawk $(THIS_DIR)/utils.c
	$(if $(V),,@echo "  GAWK    " $(call relative-to-top,$@))
	$(GAWK) -f$(THIS_DIR)/gensgxsupport.gawk <$(THIS_DIR)/utils.c >$@

errata_type := host_custom

errata: $(HOST_INTERMEDIATES)/errata/errata.h
$(HOST_INTERMEDIATES)/errata/errata.h: THIS_DIR := $(THIS_DIR)
$(HOST_INTERMEDIATES)/errata/errata.h: | $(HOST_INTERMEDIATES)/errata
$(HOST_INTERMEDIATES)/errata/errata.h: $(THIS_DIR)/gensgxutils.gawk $(TOP)/hwdefs/sgxerrata.h
	$(if $(V),,@echo "  GAWK    " $(call relative-to-top,$@))
	$(GAWK) -f$(THIS_DIR)/gensgxutils.gawk $(TOP)/hwdefs/sgxerrata.h >$@

uselink_type := host_executable
uselink_includes := include4 hwdefs $(HOST_INTERMEDIATES)/errata
uselink_src := uselink.c utils.c specialregs.c specialregs_vec.c

$(HOST_INTERMEDIATES)/uselink/utils.o: $(HOST_INTERMEDIATES)/errata/errata.h
