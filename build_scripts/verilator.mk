#Default target

all : $(TASK_OUTDIR)/${OUTPUT_EXECUTABLE}

# Variables passed in via environment
VERILATOR ?= verilator
CXX ?= g++
CXXFLAGS ?= -O2 -Wall
RTL_SOURCES ?=
CPP_SOURCES ?= 

# Make sure task_outdir directory exists
$(TASK_OUTDIR):
	mkdir -p $(TASK_OUTDIR)

# Verilate the RTL
$(TASK_OUTDIR)/V$(TOP_MODULE)__ALL.cpp: $(RTL_SOURCES) | $(TASK_OUTDIR)
	$(VERILATOR) --cc $(RTL_SOURCES) \
		--top-module $(TOP_MODULE) \
		--exe $(CPP_SOURCES) \
		--build \
		--Mdir $(TASK_OUTDIR) \
		-CFLAGS "$(CXXFLAGS)" \
		--output-dir $(TASK_OUTDIR)

# Alias for executable output
$(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE): $(TASK_OUTDIR)/V$(TOP_MODULE)__ALL.cpp
	cp $(TASK_OUTDIR)/V$(TOP_MODULE) $(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE)

clean:
	rm -rf $(TASK_OUTDIR)

