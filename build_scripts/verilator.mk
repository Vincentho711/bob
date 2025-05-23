#Default target

all : $(TASK_OUTDIR)/${OUTPUT_EXECUTABLE}

# Variables passed in via environment
VERILATOR ?= verilator
CXX ?= g++
CXXFLAGS ?= -O2 -Wall

$(info $$RTL_SRC_FILES is [${RTL_SRC_FILES}])
$(info $$CPP_SRC_FILES is [${CPP_SRC_FILES}])

# Make sure task_outdir directory exists
$(TASK_OUTDIR):
	mkdir -p $(TASK_OUTDIR)

# Verilate the RTL
$(TASK_OUTDIR)/V$(TOP_MODULE)__ALL.cpp: $(RTL_SRC_FILES) | $(TASK_OUTDIR)
	$(VERILATOR) --cc $(RTL_SRC_FILES) \
		--top-module $(TOP_MODULE) \
		--exe $(CPP_SRC_FILES) \
		--build \
		--Mdir $(TASK_OUTDIR) \
		-CFLAGS "$(CXXFLAGS)" \
		--output-dir $(TASK_OUTDIR)

# Alias for executable output
$(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE): $(TASK_OUTDIR)/V$(TOP_MODULE)__ALL.cpp
	cp $(TASK_OUTDIR)/V$(TOP_MODULE) $(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE)

clean:
	rm -rf $(TASK_OUTDIR)

