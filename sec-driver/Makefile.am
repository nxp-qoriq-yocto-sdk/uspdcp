# Copyright (c) 2011 Freescale Semiconductor, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#	notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#	notice, this list of conditions and the following disclaimer in the
#	documentation and/or other materials provided with the distribution.
#     * Neither the name of Freescale Semiconductor nor the
#	names of its contributors may be used to endorse or promote products
#	derived from this software without specific prior written permission.
#
#
# ALTERNATIVELY, this software may be distributed under the terms of the
# GNU General Public License ("GPL") as published by the Free Software
# Foundation, either version 2 of that License or (at your option) any
# later version.
#
# THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

lib_LIBRARIES = sec-driver sec-driver-dbg

AM_CFLAGS := -I$(TOP_LEVEL)/sec-driver/include -I$(TOP_LEVEL)/utils/of/include -I$(TOP_LEVEL)/utils/dma_mem/include -I$(TOP_LEVEL)/sec-driver/src
ifeq (,$(findstring USDPAA, $(EXTRA_DEFINE)))
AM_CFLAGS += -I${KERNEL_DIR}/drivers/misc
AM_CFLAGS += -I${IPC_DIR}/ipc/include
endif

SRC=src
sec-driver_SOURCES := $(SRC)/sec_driver.c $(SRC)/list.c $(SRC)/sec_contexts.c \
$(SRC)/sec_config.c $(SRC)/sec_job_ring.c $(SRC)/sec_hw_specific.c \
$(SRC)/sec_pdcp.c $(SRC)/sec_rlc.c

sec-driver-dbg_SOURCES := $(SRC)/sec_driver.c $(SRC)/list.c $(SRC)/sec_contexts.c \
$(SRC)/sec_config.c $(SRC)/sec_job_ring.c $(SRC)/sec_hw_specific.c \
$(SRC)/sec_pdcp.c $(SRC)/sec_rlc.c

ifneq (,$(findstring USDPAA, $(EXTRA_DEFINE)))
endif
sec-driver-dbg_CFLAGS += -DDEBUG
