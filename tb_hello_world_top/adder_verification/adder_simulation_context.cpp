#include "adder_simulation_context.h"

AdderSimulationContext::~AdderSimulationContext() = default;

void AdderSimulationContext::set_pipeline_flushed(bool flag) {
    pipeline_flushed_ = flag;
}

bool AdderSimulationContext::is_pipeline_flushed() const {
    return pipeline_flushed_;
}
