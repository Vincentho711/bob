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

# Run Verilator and build simulation executable
$(TASK_OUTDIR)/V$(TOP_MODULE): $(RTL_SRC_FILES) $(CPP_SRC_FILES) | $(TASK_OUTDIR)
	$(VERILATOR) --cc $(RTL_SRC_FILES) \
		--top-module $(TOP_MODULE) \
		--exe $(CPP_SRC_FILES) \
		--build \
		--Mdir $(TASK_OUTDIR) \
		-CFLAGS "$(CXXFLAGS)" \
		--output-dir $(TASK_OUTDIR)

# Optional renaming of executable
$(TASK_OUTDIR)/$(OUTPUT_EXECUTABLE): $(TASK_OUTDIR)/V$(TOP_MODULE)
	cp $< $@

# Clean rule
clean:
	rm -rf $(TASK_OUTDIR)

