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

modules := hal_server_test hal_client_test

hal_server_test_src := server.c fdsocket.c
hal_server_test_type := executable
hal_server_test_target := hal_server_test
hal_server_test_extlibs := hardware
hal_server_test_includes := include4

hal_client_test_src := client.c fdsocket.c
hal_client_test_type := executable
hal_client_test_target := hal_client_test
hal_client_test_extlibs := hardware
hal_client_test_includes := include4
