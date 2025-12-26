#Default target

all : $(TASK_OUTDIR)/${OUTPUT_EXECUTABLE}

# Detect the Operating System
UNAME_S := $(shell uname -s)

# PROFILE_BUILD variable to enabling profiling with gprof
PROFILE_BUILD ?= 1

# GCC_OPT_LEVEL variable to control the optimisation level of C++ compilation
GCC_OPT_LEVEL ?= -O2

# Path of verilator passed in via environment variable
ifeq ($(origin VERILATOR_BIN_PATH), undefined)
  $(error [ERROR] VERILATOR_BIN_PATH environment variable is not defined. Please check your tool_config.yaml or Bob)
else
  $(info [INFO] Using Verilator: $(VERILATOR_BIN_PATH))
endif

VERILATOR ?= $(VERILATOR_BIN_PATH)
CXX ?= g++
CXXFLAGS ?= $(GCC_OPT_LEVEL) -Wall -Werror -Wno-unused -std=c++20

# Verilator make flags
# Apple silicon gcc appends OPT_GLOBAL as -Os automatically, override the flag
ifeq ($(UNAME_S), Darwin)
	# Verilator make  specific variable, configure optimisation level
	OPT_FAST ?= $(GCC_OPT_LEVEL)
	OPT_SLOW ?= $(GCC_OPT_LEVEL)
	OPT_GLOBAL ?= $(GCC_OPT_LEVEL)
endif

ifeq ($(UNAME_S), Linux)
    CXXFLAGS += -fsanitize=address,undefined
    ifeq ($(PROFILE_BUILD), 1)
        CXXFLAGS += -pg
	  endif
endif

# Verilator trace to allow waveform to be dumped, can be --trace or --trace-fst
VERILATOR_TRACE_ARGS ?= --trace
# Extra args, like assigning random values to 'x' values to catch uninitialised behaviour
VERILATOR_EXTRA_ARGS ?= --x-assign unique --x-initial unique
EXTERNAL_OBJECTS ?=
INCLUDE_DIRS ?=

# Automatically find all header files in the include header directories
INCLUDE_DIRS_HEADERS := $(foreach dir,$(INCLUDE_DIRS),$(wildcard $(dir)/*.h $(dir)/*.hpp))
$(info $$INCLUDE_DIRS_HEADERS is [${INCLUDE_DIRS_HEADERS}])
# Append the INCLUDE_DIRS_HEADERS to TB_HEADER_SRC_FILES such that changes within INCLUDE_DIRS_HEADERS also cause rebuild
TB_HEADER_SRC_FILES += $(INCLUDE_DIRS_HEADERS)

# Conditionally append profiling options
# Only add -pg and use asan for Linux
ifeq ($(UNAME_S), Linux)
    LINKERFLAGS ?= -fsanitize=address,undefined
    ifeq ($(PROFILE_BUILD), 1)
        VERILATOR_EXTRA_ARGS += --prof-cfuncs
        LINKERFLAGS += -pg
		endif
else
    $(info Profiling: Skipping -pg and -fsanitize=address flag because $(UNAME_S) does not support them.)
endif

$(info $$RTL_SRC_FILES is [${RTL_SRC_FILES}])
$(info $$TB_CPP_SRC_FILES is [${TB_CPP_SRC_FILES}])
$(info $$TB_HEADER_SRC_FILES is [${TB_HEADER_SRC_FILES}])
$(info $$VERILATOR_TRACE_ARGS is [${VERILATOR_TRACE_ARGS}])
$(info $$VERILATOR_EXTRA_ARGS is [${VERILATOR_EXTRA_ARGS}])
$(info $$EXTERNAL_OBJECTS is [${EXTERNAL_OBJECTS}])
$(info $$INCLUDE_DIRS is [${INCLUDE_DIRS}])

# Transform INCLUDE_DIRS into proper -I flags
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

# Make sure task_outdir directory exists
$(TASK_OUTDIR):
	mkdir -p $(TASK_OUTDIR)

# Run Verilator and build simulation executable
$(TASK_OUTDIR)/V$(TOP_MODULE): $(RTL_SRC_FILES) $(TB_CPP_SRC_FILES) $(TB_HEADER_SRC_FILES) | $(TASK_OUTDIR)
	$(VERILATOR) --cc $(RTL_SRC_FILES) \
		--top-module $(TOP_MODULE) \
		--exe $(TB_CPP_SRC_FILES) \
		--build \
		--Mdir $(TASK_OUTDIR) \
		-MAKEFLAGS "OPT_SLOW=$(OPT_SLOW) OPT_FAST=$(OPT_FAST) OPT_GLOBAL=$(OPT_GLOBAL)" \
		$(VERILATOR_TRACE_ARGS) \
		-CFLAGS "$(CXXFLAGS) $(INCLUDE_FLAGS)" \
		-LDFLAGS "$(EXTERNAL_OBJECTS) $(LINKERFLAGS)"

# Optional renaming of executable
$(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE): $(TASK_OUTDIR)/V$(TOP_MODULE)
	cp $< $@

# Clean rule
clean:
	rm -rf $(TASK_OUTDIR)

