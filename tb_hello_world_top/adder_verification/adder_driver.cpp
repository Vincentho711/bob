#include "adder_driver.h"
#include <iostream>

// Constructor with default configuration
AdderDriver::AdderDriver(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx)
    : Base(name, dut, ctx), ctx_(ctx), config_(Config{}) {
    current_inputs_ = {0, 0, 0, false};
    reset();
    log_info("AdderDriver initialized with pipeline depth " + std::to_string(config_.pipeline_depth));
}

// Constructor with custom configuration
AdderDriver::AdderDriver(const std::string& name, DutPtr dut, AdderSimulationContextPtr ctx, const Config& config)
    : Base(name, dut, ctx), ctx_(ctx), config_(config) {
    current_inputs_ = {0, 0, 0, false};
    reset();
    log_info("AdderDriver initialized with pipeline depth " + std::to_string(config_.pipeline_depth));
}

void AdderDriver::generate_corner_cases(const std::string& name_prefix) {
    auto corner_cases = AdderTransactionFactory::get_all_corner_cases();

    for (size_t i = 0; i < corner_cases.size(); ++i) {
        std::string name = name_prefix + "_" + std::to_string(i);
        auto txn = AdderTransactionFactory::create_corner_case(corner_cases[i], name);
        add_transaction(txn);
    }

    log_info("Generated " + std::to_string(corner_cases.size()) + " corner case transactions");
}

void AdderDriver::generate_random_transactions(size_t count, std::mt19937& rng,
                                              const std::string& name_prefix) {
    for (size_t i = 0; i < count; ++i) {
        std::string name = name_prefix + "_" + std::to_string(i);
        auto txn = AdderTransactionFactory::create_random(rng, name);
        add_transaction(txn);
    }

    log_info("Generated " + std::to_string(count) + " random transactions");
}

void AdderDriver::add_directed_transaction(uint8_t a, uint8_t b, const std::string& name) {
    auto txn = AdderTransactionFactory::create_directed(a, b, name);
    add_transaction(txn);
    log_debug("Added directed transaction: a=" + std::to_string(a) + 
              ", b=" + std::to_string(b) + ", name=" + name);
}

void AdderDriver::drive_idle_cycles(uint32_t cycles) {
    for (uint32_t i = 0; i < cycles; ++i) {
        std::string name = "idle_cycle_" + std::to_string(i);
        auto idle_txn = create_idle_transaction(name);
        add_transaction(idle_txn);
    }
    log_info("Added " + std::to_string(cycles) + " idle cycles");
}

void AdderDriver::set_idle_values(uint8_t a, uint8_t b) {
    config_.idle_value_a = a;
    config_.idle_value_b = b;
    log_debug("Set idle values: a=" + std::to_string(a) + ", b=" + std::to_string(b));
}

void AdderDriver::update_config(const Config& new_config) {
    config_ = new_config;
    log_info("Configuration updated");
}

void AdderDriver::reset() {
    Base::reset();
    
    // Reset DUT inputs to known state
    auto dut = get_dut();
    dut->a_i = config_.idle_value_a;
    dut->b_i = config_.idle_value_b;
    
    // Reset internal state
    current_inputs_ = {config_.idle_value_a, config_.idle_value_b, 0, true};
    adder_stats_ = AdderStats{};
    
    log_info("AdderDriver reset to idle state");
}

void AdderDriver::drive_transaction(const AdderTransaction& txn) {
    uint8_t a = txn.get_a();
    uint8_t b = txn.get_b();
    
    // Validate inputs if enabled
    if (config_.enable_input_validation && !validate_inputs(a, b)) {
        adder_stats_.validation_failures++;
        log_error("Input validation failed for transaction: " + txn.convert2string());
        return;
    }
    
    // Drive the actual inputs to the DUT
    auto dut = get_dut();
    dut->a_i = a;
    dut->b_i = b;
    
    // Update tracking
    update_current_inputs(a, b);
    classify_and_update_stats(txn);
    
    log_debug("Drove inputs: a=" + std::to_string(a) + 
              ", b=" + std::to_string(b));
}

void AdderDriver::pre_drive(const AdderTransaction& txn) {
    uint64_t current_cycle = ctx_->current_cycle();
    // Pipeline tracking or pre-processing can be added here
    if (config_.enable_pipeline_tracking) {
        log_debug("Pre-drive: Pipeline cycle " + std::to_string(current_cycle % config_.pipeline_depth));
    }
}

void AdderDriver::post_drive(const AdderTransaction& txn) {
    // Post-processing or additional validation can be added here
    if (config_.enable_pipeline_tracking) {
        log_debug("Post-drive: Transaction will appear at output in " + 
                 std::to_string(config_.pipeline_depth) + " cycles");
    }
}

bool AdderDriver::validate_inputs(uint8_t a, uint8_t b) const {
    // For a simple adder, all 8-bit combinations are valid
    // This method can be extended for more complex validation rules
    static_cast<void>(a);  // Suppress unused parameter warning
    static_cast<void>(b);
    return true;
}

void AdderDriver::update_current_inputs(uint8_t a, uint8_t b) {
    uint64_t current_cycle = ctx_->current_cycle();
    current_inputs_.a = a;
    current_inputs_.b = b;
    current_inputs_.cycle = current_cycle;
    current_inputs_.valid = true;
}

void AdderDriver::classify_and_update_stats(const AdderTransaction& txn) {
    const std::string& name = txn.get_name();
    
    if (name.find("corner") != std::string::npos) {
        adder_stats_.corner_cases_driven++;
    } else if (name.find("random") != std::string::npos) {
        adder_stats_.random_cases_driven++;
    } else if (name.find("idle") != std::string::npos) {
        adder_stats_.idle_cycles_driven++;
    } else {
        adder_stats_.directed_cases_driven++;
    }
}

std::shared_ptr<AdderTransaction> AdderDriver::create_idle_transaction(const std::string& name) {
    return AdderTransactionFactory::create_directed(
        config_.idle_value_a, 
        config_.idle_value_b, 
        name
    );
}

void AdderDriver::print_report() const {
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "ADDER DRIVER REPORT: " << get_name() << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    // Base driver statistics
    const auto& base_stats = get_stats();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        base_stats.last_activity - base_stats.start_time);
    
    std::cout << "Base Driver Statistics:" << std::endl;
    std::cout << "  Total Transactions: " << base_stats.transactions_driven << std::endl;
    std::cout << "  Active Cycles: " << base_stats.cycles_active << std::endl;
    std::cout << "  Runtime: " << duration.count() << " ms" << std::endl;
    
    // Adder-specific statistics
    std::cout << "\nAdder-Specific Statistics:" << std::endl;
    std::cout << "  Corner Cases: " << adder_stats_.corner_cases_driven << std::endl;
    std::cout << "  Random Cases: " << adder_stats_.random_cases_driven << std::endl;
    std::cout << "  Directed Cases: " << adder_stats_.directed_cases_driven << std::endl;
    std::cout << "  Idle Cycles: " << adder_stats_.idle_cycles_driven << std::endl;
    std::cout << "  Validation Failures: " << adder_stats_.validation_failures << std::endl;
    
    // Current state
    std::cout << "\nCurrent State:" << std::endl;
    if (current_inputs_.valid) {
        std::cout << "  Last Inputs: a=" << +current_inputs_.a 
                  << ", b=" << +current_inputs_.b 
                  << " (cycle " << current_inputs_.cycle << ")" << std::endl;
    } else {
        std::cout << "  Last Inputs: None" << std::endl;
    }
    
    std::cout << "  Pending Transactions: " << pending_count() << std::endl;
    
    // Configuration
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Input Validation: " << (config_.enable_input_validation ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Pipeline Tracking: " << (config_.enable_pipeline_tracking ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Pipeline Depth: " << config_.pipeline_depth << std::endl;
    std::cout << "  Idle Values: a=" << +config_.idle_value_a << ", b=" << +config_.idle_value_b << std::endl;
    
    std::cout << std::string(50, '=') << std::endl;
}
