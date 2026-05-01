#ifndef HESIA_CORE_HPP
#define HESIA_CORE_HPP

#include "hesia/core.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace hesia {

class Error : public std::runtime_error {
public:
    explicit Error(const hesia_error_t &error)
        : std::runtime_error(hesia_error_message(&error)), status_(error.status) {}

    hesia_status_t status() const noexcept { return status_; }

private:
    hesia_status_t status_;
};

inline void throw_if_failed(hesia_status_t status, const hesia_error_t &error) {
    if (status != HESIA_STATUS_OK) {
        throw Error(error);
    }
}

inline std::string version() {
    return std::string(hesia_version());
}

class Watchdog {
public:
    Watchdog(uint64_t timeout_ms, uint32_t max_missed) {
        hesia_error_t error;
        hesia_error_init(&error);
        throw_if_failed(hesia_watchdog_init(&watchdog_, timeout_ms, max_missed, &error), error);
    }

    void arm() {
        hesia_error_t error;
        hesia_error_init(&error);
        throw_if_failed(hesia_watchdog_arm(&watchdog_, &error), error);
    }

    void heartbeat() {
        hesia_error_t error;
        hesia_error_init(&error);
        throw_if_failed(hesia_watchdog_heartbeat(&watchdog_, &error), error);
    }

    void check() {
        hesia_error_t error;
        hesia_error_init(&error);
        throw_if_failed(hesia_watchdog_check(&watchdog_, &error), error);
    }

    void disarm() noexcept {
        hesia_watchdog_disarm(&watchdog_);
    }

private:
    hesia_watchdog_t watchdog_{};
};

inline void validate_inference(
    const hesia_model_contract_t &contract,
    const hesia_inference_request_t &request) {
    hesia_error_t error;
    hesia_error_init(&error);
    throw_if_failed(hesia_inference_validate(&contract, &request, &error), error);
}

inline std::vector<uint8_t> random_bytes(std::size_t len) {
    std::vector<uint8_t> out(len);
    hesia_error_t error;
    hesia_error_init(&error);
    throw_if_failed(hesia_random(out.data(), out.size(), &error), error);
    return out;
}

} // namespace hesia

#endif
