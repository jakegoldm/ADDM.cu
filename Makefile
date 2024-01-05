NVCC := /usr/local/cuda-12.2/bin/nvcc
CXX := g++

SIM_EXECS := addm_simulate 
MLE_EXECS := addm_mle 
TEST_EXECS := addm_test
RUN_EXECS := tutorial 

LIB_DIR := lib
OBJ_DIR := obj
INC_DIR := include
BUILD_DIR := bin
SRC_DIR := sample
TEST_DIR := tests

CXXFLAGS := -O3 -march=native -fPIC $(MACROS)
NVCCFLAGS := -O3 -Xcompiler -fPIC
SHAREDFLAGS = -I $(INC_DIR) -lpthread
LDFLAGS := -shared
LIB := -L lib -lpthread
INC := -I $(INC_DIR)

UNAME_S := $(shell uname -s)

INSTALL_LIB_DIR := /usr/lib
ifeq ($(UNAME_S), Linux)
	INSTALL_INC_DIR := /usr/include
endif
ifeq ($(UNAME_S), Darwin)
	INSTALL_INC_DIR := /usr/local/include 
endif 


PY_SUFFIX := $(shell python3-config --extension-suffix)
PY_INCLUDES := $(shell python3 -m pybind11 --includes)
PY_SO_FILE := $(addsuffix $(PY_SUFFIX), addm_toolbox_cuda)

CPP_FILES := $(filter-out $(LIB_DIR)/bindings.cpp, $(wildcard $(LIB_DIR)/*.cpp))
CU_FILES := $(wildcard $(LIB_DIR)/*.cu)
CPP_OBJ_FILES := $(patsubst $(LIB_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_FILES))
CU_OBJ_FILES := $(patsubst $(LIB_DIR)/%.cu,$(OBJ_DIR)/%.o,$(CU_FILES)) 

$(OBJ_DIR): 
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR): 
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $(SHAREDFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.cu
	$(NVCC) $(NVCCFLAGS) -c $(SHAREDFLAGS) -o $@ $<

define compile_target
	$(CXX) $(CXXFLAGS) -c $(addprefix $(SRC_DIR)/, $1.cpp) $(LIB) $(INC) -o $(addprefix $(OBJ_DIR)/, $1.o)
	$(NVCC) $(addprefix $(OBJ_DIR)/, $1.o) $(CPP_OBJ_FILES) $(CU_OBJ_FILES) -o $(addprefix $(BUILD_DIR)/, $1)
endef

define compile_test 
	$(NVCC) $(addprefix $(TEST_DIR)/, $1.cpp) -laddm -o $(addprefix $(BUILD_DIR)/, $1)
endef

install: $(OBJ_DIR) $(BUILD_DIR) $(CPP_OBJ_FILES) $(CU_OBJ_FILES)
	$(NVCC) $(LDFLAGS) $(MACROS) -o $(INSTALL_LIB_DIR)/libaddm.so $(CPP_OBJ_FILES) $(CU_OBJ_FILES)
	@echo Installing for $(UNAME_S) in $(INSTALL_INC_DIR)
	cp -TRv $(INC_DIR) $(INSTALL_INC_DIR)/addm

sim: $(OBJ_DIR) $(BUILD_DIR) $(CPP_OBJ_FILES) $(CU_OBJ_FILES)
	$(foreach source, $(SIM_EXECS), $(call compile_target, $(source));)


mle: $(OBJ_DIR) $(BUILD_DIR) $(CPP_OBJ_FILES) $(CU_OBJ_FILES)
	$(foreach source, $(MLE_EXECS), $(call compile_target, $(source));)

test: 
	$(foreach source, $(TEST_EXECS), $(call compile_test, $(source));)

run: $(OBJ_DIR) $(BUILD_DIR) $(CPP_OBJ_FILES) $(CU_OBJ_FILES)
	$(foreach source, $(RUN_EXECS), $(call compile_target, $(source));)

all: sim mle run


pybind: 
	$(NVCC) $(LDFLAGS) $(NVCCFLAGS) $(PY_INCLUDES) $(INC) $(CPP_FILES) $(CU_FILES) $(LIB_DIR)/bindings.cpp -o $(PY_SO_FILE)


.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BUILD_DIR)
	rm -rf docs