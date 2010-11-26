# Readme:
# use: make config
# use: make dir
# use: make GlobalClean
# use: make clean
# use: make rebuild
# use: make cleanall
# use: make test
# use: make

# firs of all, use "make config" or "make dir" to 
# build a source files struct. and then,
# put your source files into the DIR src
# link libs to the DIR lib
# set as a C++ Makefile
# use: ":1,$ s/\.c/\.cpp/g" in vi


SHELL=/bin/sh
CC=gcc
MAKE=make
MAKE_DIR=$(PWD)
SRC_DIR=$(MAKE_DIR)/src/
OBJ_DIR=$(MAKE_DIR)/obj/
LIB_DIR=$(MAKE_DIR)/lib/
INCLUDE_DIR=$(MAKE_DIR)/include
DEBUG_DIR=$(MAKE_DIR)/debug/
RELEASE_DIR=$(MAKE_DIR)/release/
OUTPUT_DIR=
INCLUDE=-I$(INCLUDE_DIR) -I$(SRC_DIR) -I/usr/local/include/opencv
LIB=-L$(LIB_DIR) -L$(OBJ_DIR) -L/usr/local/lib -lcv -lhighgui
OUTPUT_FILE=movit

vpath %.c $(SRC_DIR)
vpath %.o $(OBJ_DIR)
vpath %.d $(OBJ_DIR)

SRC_FILES:=$(wildcard $(SRC_DIR)*.c)
SRC_FILES:=$(notdir $(SRC_FILES))
OBJ_FILES:=$(patsubst %.c,%.o,$(SRC_FILES) ) 
DEP_FILES:=$(patsubst %.c,%.d,$(SRC_FILES) ) 

FLAG_DEBUG=-g
FLAG_COMPLE=-c
FLAG_LINK=

DEBUG=1
ifeq ($(DEBUG),1)
OUTPUT_DIR:=$(DEBUG_DIR)
FLAG_COMPLE:=$(FLAG_COMPLE) $(FLAG_DEBUG)
FLAG_LINK:=
else
OUTPUT_DIR:=$(RELEASE_DIR)
FLAG_COMPLE:=$(FLAG_COMPLE)
FLAG_LINK:=
endif

OUT=$(OUTPUT_DIR)$(OUTPUT_FILE)

$(OUT): $(OBJ_FILES)
	@echo  "building: $(notdir $@) \t\t\t please wait ..."
	@$(CC) $(FLAG_LINK) $(addprefix $(OBJ_DIR),$(notdir $^)) $(LIB) -o $@
%.o:%.c %.d
	@echo  "building: $(notdir $@) \t\t\t please wait ..."
	@$(CC) $(FLAG_COMPLE) $< $(INCLUDE) -o $(OBJ_DIR)$@
$(OBJ_DIR)%.d:%.c
	@echo  "building: $(notdir $@) \t\t\t please wait ..."
	@$(CC) $< $(INCLUDE) -MM -MD -o $@
#	@$(CC) $< $(INCLUDE) -o $@
-include $(addprefix $(OBJ_DIR),$(DEP_FILES))
config: dir
dir:
	mkdir -p $(SRC_DIR)
	mkdir -p $(OBJ_DIR)
	mkdir -p $(LIB_DIR)
	mkdir -p $(INCLUDE_DIR)
	mkdir -p $(DEBUG_DIR)
	mkdir -p $(RELEASE_DIR)
clean:
	@rm -f $(OBJ_DIR)* *.d *.o
	@rm -f $(OUT)
	@clear
rebuild: clean all
cleanall:
	@rm -f $(OBJ_DIR)*
	@rm -f $(RELEASE_DIR)*
	@rm -f $(DEBUG_DIR)*
test:
	$(OUT)
.PHONY: all config rebuild test cleanall
.SUFFIXES:
GlobalClean:
	@find . -type f -name "Makefile" |sed -n '2,$$p'|sed s/Makefile/\ \`pwd\`/g|awk ' {ECHO="echo"};{CD="cd "};{MAKE="&& make clean&&"};{print ECHO,CD,$$1,MAKE,CD,$$2  } '   |sh

	
