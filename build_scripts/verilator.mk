#Default target

all : $(TASK_OUTDIR)/${OUTPUT_EXECUTABLE}

# Variables passed in via environment
VERILATOR ?= verilator
CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Werror -Wno-unused
# Verilator trace to allow waveform to be dumped, can be --trace or --trace-fst
VERILATOR_TRACE_ARGS ?= --trace
# Extra args, like assigning random values to 'x' values to catch uninitialised behaviour
VERILATOR_EXTRA_ARGS ?= --x-assign unique --x-initial unique
EXTERNAL_OBJECTS ?=
INCLUDE_DIRS ?=

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
		$(VERILATOR_TRACE_ARGS) \
		-CFLAGS "$(CXXFLAGS) $(INCLUDE_FLAGS)" \
		-LDFLAGS "$(EXTERNAL_OBJECTS)"

# Optional renaming of executable
$(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE): $(TASK_OUTDIR)/V$(TOP_MODULE)
	cp $< $@

# Clean rule
clean:
	rm -rf $(TASK_OUTDIR)

