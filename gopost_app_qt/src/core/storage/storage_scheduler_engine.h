#pragma once

#include "core/interfaces/IStorageScheduler.h"
#include "core/interfaces/IDeviceCapability.h"
#include "core/interfaces/ILogger.h"
#include "core/interfaces/IEventBus.h"
#include "storage_profiler.h"
#include "block_aligner.h"
#include "read_ahead_predictor.h"
#include "io_bandwidth_monitor.h"
#include "read_scheduler.h"

#include <memory>

namespace gopost::storage {

using core::interfaces::IStorageScheduler;
using core::interfaces::IDeviceCapability;
using core::interfaces::ILogger;
using core::interfaces::IEventBus;
using core::interfaces::ReadRequest;
using core::interfaces::ReadTicket;
using core::interfaces::StorageProfile;

class StorageSchedulerEngine : public IStorageScheduler {
public:
    StorageSchedulerEngine(const IDeviceCapability& deviceCap, ILogger& logger, IEventBus& eventBus);
    ~StorageSchedulerEngine();

    Result<ReadTicket> scheduleRead(const ReadRequest& request) override;
    void cancelRead(ReadTicket ticket) override;
    StorageProfile getStorageProfile() const override;

    // Expose sub-components for testing
    ReadAheadPredictor& readAheadPredictor() { return predictor_; }
    IOBandwidthMonitor& bandwidthMonitor() { return monitor_; }

private:
    ILogger& logger_;
    StorageProfiler profiler_;
    StorageProfile profile_;
    BlockAligner aligner_;
    ReadAheadPredictor predictor_;
    IOBandwidthMonitor monitor_;
    ReadScheduler scheduler_;
};

} // namespace gopost::storage
