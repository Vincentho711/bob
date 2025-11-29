#ifndef SEQUENCER_H
#define SEQUENCER_H
#include "simulation_object_pool.h"
#include "simulation_task.h"
#include "transaction.h"
#include <string>
#include <iostream>
#include <memory>

template <typename TransactionT>
class BaseSequencer : public std::enable_shared_from_this<BaseSequencer<TransactionT>>{
public:
    using TxnType = TransactionT;
    using TxnPtr = std::shared_ptr<TxnType>;

    simulation::SharedObjectPool<TxnType> pool;

    virtual ~BaseSequencer() = default;

    template<typename SeqType>
    simulation::Task start_sequence(std::unique_ptr<SeqType> seq) {
        // Get a shared_ptr to the current BaseSequencer
        // Warning: This throws std::bad_weak_ptr if 'this' wasn't created via make_shared
        auto base_sequencer_ptr = this->shared_from_this();

        // Identify the concrete sequencer type expected by Sequence
        using TargetSequencerType = typename SeqType::SequencerType;

        // Downcast safely with static cast since we already know the type relationship
        auto concrete_sequencer_ptr = std::static_pointer_cast<TargetSequencerType>(base_sequencer_ptr);

        // Assign the shared_ptr to the sequence object
        seq->p_sequencer = concrete_sequencer_ptr;

        // Execute
        co_await seq->body();
    }
};

// /**
//  * @brief Enumeration for sequencer log levels
//  */
// enum class SequencerLogLevel {
//     DEBUG = 0,
//     INFO = 1,
//     WARNING = 2,
//     ERROR = 3,
//     FATAL = 4
// };
//
// /**
//  * @brief Configuration structure for checker behavior
//  */
// struct SequencerConfig {
//     SequencerLogLevel log_level = SequencerLogLevel::INFO;
//
//     SequencerConfig() = default;
// };
//
// inline std::ostream& operator<<(std::ostream& os, SequencerLogLevel level) {
//     switch (level) {
//         case SequencerLogLevel::DEBUG:   return os << "DEBUG";
//         case SequencerLogLevel::INFO:    return os << "INFO";
//         case SequencerLogLevel::WARNING: return os << "WARNING";
//         case SequencerLogLevel::ERROR:   return os << "ERROR";
//         default:                       return os << "UNKNOWN";
//     }
// }
// template<typename DUT_TYPE, typename TRANSACTION_TYPE>
// class BaseSequencer {
// public:
//     using DutPtr = std::shared_ptr<DUT_TYPE>;
//     using TransactionPtr = std::shared_ptr<TRANSACTION_TYPE>;
//
// protected:
//     std::string name_;
//     DutPtr dut_;
//     SequencerConfig config_;
//
// public:
//     /**
//      * @brief Construct a new BaseSequencer
//      * 
//      * @param name Unique name for this checker instance
//      * @param dut Shared pointer to the DUT
//      * @param config Configuration for checker behaviour
//      */
//     explicit BaseSequencer(const std::string& name, DutPtr dut,  const SequencerConfig& config = SequencerConfig{})
//         : name_(name), dut_(dut), config_(config) {
//         if (!dut_) {
//             throw std::invalid_argument("DUT pointer cannot be null");
//         }
//         log_info("BaseSequencer '" + name_ + "' initialised");
//     }
//
//     virtual ~BaseSequencer() = default;
//
//     // Disable copy constructor and assignment (checkers should be unique)
//     BaseSequencer(const BaseSequencer&) = delete;
//     BaseSequencer& operator=(const BaseSequencer&) = delete;
//
//     // Enable move constructor and assignment
//     BaseSequencer(BaseSequencer&&) = default;
//     BaseSequencer& operator=(BaseSequencer&&) = default;
//
//     /**
//      * @brief Reset the checker to initial state
//      */
//     virtual void reset() {
//         log_info("Sequencer '" + name_ + "' reset");
//     }
//
//     // Getters
//     const std::string& get_name() const { return name_; }
//
// protected:
//     /**
//      * @brief Get the DUT pointer (for derived classes)
//      */
//     DutPtr get_dut() const { return dut_; }
//
//     // Logging methods
//     void log_debug(const std::string& message) const {
//         if (config_.log_level <= SequencerLogLevel::DEBUG) {
//             log_message("DEBUG", message);
//         }
//     }
//
//     void log_info(const std::string& message) const {
//         if (config_.log_level <= SequencerLogLevel::INFO) {
//             log_message("INFO", message);
//         }
//     }
//
//     void log_warning(const std::string& message) const {
//         if (config_.log_level <= SequencerLogLevel::WARNING) {
//             log_message("WARNING", message);
//         }
//     }
//
//     void log_error(const std::string& message) const {
//         if (config_.log_level <= SequencerLogLevel::ERROR) {
//             log_message("ERROR", message);
//         }
//     }
//
//     void log_fatal(const std::string& message) const {
//         log_message("FATAL", message);
//     }
//
// private:
//     void log_message(const std::string& level, const std::string& message) const {
//         std::cout << "[" << level << "] [" << name_ << "] "
//                   << message << std::endl;
//     }
// };
#endif // SEQUENCER_H
