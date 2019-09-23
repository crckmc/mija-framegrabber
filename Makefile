CROSS_COMPILE ?=arm-linux-gnueabihf-
GCC ?= $(CROSS_COMPILE)gcc
STRIP ?= $(CROSS_COMPILE)strip

MI_TOP = $(shell pwd)/../..

C_FLAGS := -w -O2 -march=armv7-a
C_FLAGS += -fPIC -DPIC -DOMXILCOMPONENTSPATH=\"/$(BUILD_DIR)\" -DCONFIG_DEBUG_LEVEL=255

OMX_TOP ?= $(shell pwd)

IPCM_OUT := $(shell pwd)/../../../out



LDFLAGS := -L$(MI_TOP)/lib -lev -lshbf -lshbfev -lc-2.25 -lpthread-2.25

C_INCLUDES := \
	-I$(MI_TOP)/inc \
	-I$(MI_TOP)/include

PROGS = mija-framegrab

OMX_COMP_C_SRCS=$(wildcard ./src/*.c)
OMX_COMP_C_SRCS_NO_DIR=$(notdir $(OMX_COMP_C_SRCS))
OBJECTS=$(patsubst %.c, %.c.o,  $(OMX_COMP_C_SRCS_NO_DIR))

OBJDIR ?= $(shell pwd)/obj
BINDIR ?= $(shell pwd)/bin
INSTALLDIR ?= $(shell pwd)/install
INCDIR ?= $(shell pwd)/inc

OBJPROG = $(addprefix $(OBJDIR)/, $(PROGS))

.PHONY: clean prepare PROGS


all: prepare $(OBJPROG)

prepare:

clean:
	@rm -Rf $(OBJDIR)
	@rm -Rf $(INSTALLDIR)
	@rm -Rf $(BINDIR)

install:
	@mkdir -p $(OMX_TOP)/install
	cp -f ${BINDIR}/* ${INSTALLDIR}
	cp -f ${INCDIR}/*.h  ${INSTALLDIR}

$(OBJPROG):	$(addprefix $(OBJDIR)/, $(OBJECTS))
	@mkdir -p $(OMX_TOP)/bin
	@echo "  BIN $@"
	@$(GCC) $(LDFLAGS) -o $@ $(addprefix $(OBJDIR)/, $(OBJECTS))
	@echo ""
	cp -f ${OBJDIR}/$(PROGS) ${BINDIR}

$(OBJDIR)/%.c.o : src/%.c
	@mkdir -p obj
	@echo "  CC  $<"
	@$(GCC) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@

