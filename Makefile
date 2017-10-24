include .config.mk

SRC=src
OBJ=obj
INC=inc
BIN=bin

WARNINGS=-Wall -Wshadow -Wcast-qual -Wpointer-arith \
		-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
		-Wredundant-decls -Wnested-externs -Werror

INCLUDES=-I$(INC)

ifeq ($(DEBUG), 1)
OPT=-g -DDEBUG
else
OPT=-O2
endif

ifeq ($(OS), LINUX)
LDFLAGS=-lpthread
endif

ifeq ($(OS), FREEBSD)
LDFLAGS=-pthread -D_THREAD_SAFE
endif

ifeq ($(OS), NETBSD)
LDFLAGS=-lpthread
endif

ifeq ($(OS), DARWIN)
LDFLAGS=
endif

ifeq ($(VERBOSE), 1)
RUN_PRINT=@\#
RUN_EXEC=
else
RUN_PRINT=
RUN_EXEC=@
endif

PRINTF1=@printf "  %-7s %-25s\n"
PRINTF2=@printf "  %-7s %-25s -> %s\n"


CFLAGS=$(OPT) $(WARNINGS) $(INCLUDES) -DOS_$(OS) -DPATH='"$(PREFIX)"' -DCC_$(CCNAME) -DDUMPINTERVAL=$(DUMPINTERVAL)

all: banner prepare $(BIN)/piranha $(BIN)/ptoa $(BIN)/piranhactl
	$(PRINTF1) INFO "Compilation done"

help:
	@echo "Piranha Makefile help"
	@echo ""
	@echo "make help      : this message"
	@echo "make clean     : cleanup"
	@echo "make distclean : complete cleanup"
	@echo "make all       : compile everything"
	@echo "make install   : instal to $(PREFIX)"
	@echo ""

banner:
	@echo '/*******************************************************************************/'
	@echo '/*                                                                             */'
	@echo '/*        ::::::    ::                                ::                       */'
	@echo '/*        ::    ::      ::  ::::    ::::::  ::::::    ::::::      ::::::       */'
	@echo '/*        ::::::    ::  ::::      ::    ::  ::    ::  ::    ::  ::    ::       */'
	@echo '/*        ::        ::  ::        ::    ::  ::    ::  ::    ::  ::    ::       */'
	@echo '/*        ::        ::  ::          ::::::  ::    ::  ::    ::    ::::::       */'
	@echo '/*                                                                             */'
	@echo '/*                                                                             */'
	@echo '/*******************************************************************************/'
	@echo '/*                                                                             */'
	@echo '/*  Copyright 2004-2017 Pascal Gloor                                           */'
	@echo '/*                                                                             */'
	@echo '/*  Licensed under the Apache License, Version 2.0 (the "License");            */'
	@echo '/*  you may not use this file except in compliance with the License.           */'
	@echo '/*  You may obtain a copy of the License at                                    */'
	@echo '/*                                                                             */'
	@echo '/*     http://www.apache.org/licenses/LICENSE-2.0                              */'
	@echo '/*                                                                             */'
	@echo '/*  Unless required by applicable law or agreed to in writing, software        */'
	@echo '/*  distributed under the License is distributed on an "AS IS" BASIS,          */'
	@echo '/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */'
	@echo '/*  See the License for the specific language governing permissions and        */'
	@echo '/*  limitations under the License.                                             */'
	@echo '/*                                                                             */'
	@echo '/*******************************************************************************/'

prepare:
	$(RUN_PRINT)$(PRINTF1) MKDIR "$(OBJ) $(BIN)"
	$(RUN_EXEC)$(MKDIR) -p $(OBJ) $(BIN)

$(BIN)/piranha: $(OBJ)/p_tools.o $(OBJ)/p_config.o $(OBJ)/p_socket.o $(OBJ)/p_log.o $(OBJ)/p_dump.o $(OBJ)/p_piranha.o
	$(RUN_PRINT)$(PRINTF2) LINK $@ "$^"
	$(RUN_EXEC)$(CC) -o $@ $^ $(LDFLAGS)
	$(PRINTF2) INFO "Compilation done" $@

$(BIN)/piranhactl:
	$(RUN_PRINT)$(PRINTF2) SED utils/piranhactl.in $(BIN)/piranhactl
	$(RUN_EXEC)$(CAT) utils/piranhactl.in | $(SED) "s@%PATH%@$(PREFIX)@g" > $(BIN)/piranhactl

$(BIN)/ptoa: $(OBJ)/p_tools.o $(OBJ)/p_undump.o $(OBJ)/p_ptoa.o
	$(RUN_PRINT)$(PRINTF2) LINK $@ "$^"
	$(RUN_EXEC)$(CC) -o $@ $^ $(LDFLAGS)
	$(PRINTF2) INFO "Compilation done" $@

clean:
	$(RUN_PRINT)$(PRINTF1) RM "$(OBJ) $(BIN)"
	$(RUN_EXEC)$(RM) -rf $(OBJ) $(BIN)
	$(PRINTF1) INFO "Cleanup done"

distclean: clean
	$(RUN_PRINT)$(PRINTF1) RM ".config.mk"
	$(RUN_EXEC)$(RM) -f .config.mk
	$(PRINTF1) INFO "Distcleanup done"

test: $(BIN)/ptoa
	$(RUN_PRINT)$(PRINTF1) TEST test/test.sh
	$(RUN_EXEC)cd test && ./test.sh

install:
	$(RUN_PRINT)$(PRINTF1) MKDIR $(PREFIX)/$(BIN)
	$(RUN_EXEC)$(MKDIR) -p $(PREFIX)/$(BIN)

	$(RUN_PRINT)$(PRINTF1) MKDIR $(PREFIX)/etc
	$(RUN_EXEC)$(MKDIR) -p $(PREFIX)/etc

	$(RUN_PRINT)$(PRINTF1) MKDIR $(PREFIX)/var/dump
	$(RUN_EXEC)$(MKDIR) -p $(PREFIX)/var/dump

	$(RUN_PRINT)$(PRINTF2) CP $(BIN)/piranha $(PREFIX)/$(BIN)/
	$(RUN_EXEC)$(CP) $(BIN)/piranha $(PREFIX)/$(BIN)/
	$(RUN_EXEC)$(CHMOD) 755 $(PREFIX)/$(BIN)/piranha

	$(RUN_PRINT)$(PRINTF2) CP $(BIN)/ptoa $(PREFIX)/$(BIN)/
	$(RUN_EXEC)$(CP) $(BIN)/ptoa $(PREFIX)/$(BIN)/
	$(RUN_EXEC)$(CHMOD) 755 $(PREFIX)/$(BIN)/ptoa

	$(RUN_PRINT)$(PRINTF2) CP etc/piranha_sample.conf $(PREFIX)/etc/
	$(RUN_EXEC)$(CP) etc/piranha_sample.conf $(PREFIX)/etc/
	$(RUN_EXEC)$(CHMOD) 644 $(PREFIX)/etc/piranha_sample.conf

	$(RUN_PRINT)$(PRINTF2) CP $(BIN)/piranhactl $(PREFIX)/$(BIN)/
	$(RUN_EXEC)$(CP) $(BIN)/piranhactl $(PREFIX)/$(BIN)/
	$(RUN_EXEC)$(CHMOD) 755 $(PREFIX)/$(BIN)/piranhactl

	$(RUN_PRINT)$(PRINTF2) CP "man" $(PREFIX)/
	$(RUN_EXEC)$(CP) -r man $(PREFIX)/

	$(PRINTF1) INFO "Installation done"

$(OBJ)/%.o: $(SRC)/%.c
	$(RUN_PRINT)$(PRINTF2) CC $< $@
	$(RUN_EXEC)$(CC) $(CFLAGS) -o $@ -c $<
