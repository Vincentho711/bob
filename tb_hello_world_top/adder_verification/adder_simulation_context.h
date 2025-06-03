#include "simulation_context.h"

class AdderSimulationContext : public SimulationContext {
public:
    ~AdderSimulationContext() override;

    void set_pipeline_flushed(bool flag);
    bool is_pipeline_flushed() const;

private:
    bool pipeline_flushed_ = false;
};
