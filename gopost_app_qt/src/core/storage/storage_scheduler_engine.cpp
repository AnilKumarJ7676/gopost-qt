#include "storage_scheduler_engine.h"

namespace gopost::storage {

StorageSchedulerEngine::StorageSchedulerEngine(const IDeviceCapability& deviceCap,
                                               ILogger& logger, IEventBus& eventBus)
    : logger_(logger)
    , profiler_(deviceCap)
    , profile_(profiler_.buildProfile())
    , aligner_(profile_.recommendedBlockSize)
    , monitor_(eventBus, profile_.estimatedSequentialMBps)
    , scheduler_(profile_, aligner_, predictor_, monitor_) {

    logger_.info(QStringLiteral("Storage"),
                 QStringLiteral("StorageSchedulerEngine initialized — type=%1 block=%2 concurrent=%3 directIO=%4 est=%5 MB/s")
                     .arg(static_cast<int>(profile_.storageType))
                     .arg(profile_.recommendedBlockSize)
                     .arg(profile_.maxConcurrentReads)
                     .arg(profile_.supportsDirectIO ? "YES" : "NO")
                     .arg(profile_.estimatedSequentialMBps, 0, 'f', 0));

    scheduler_.start();
}

StorageSchedulerEngine::~StorageSchedulerEngine() {
    scheduler_.stop();
}

Result<ReadTicket> StorageSchedulerEngine::scheduleRead(const ReadRequest& request) {
    if (request.path.empty())
        return Result<ReadTicket>::failure("Empty path in ReadRequest");
    if (request.length == 0)
        return Result<ReadTicket>::failure("Zero-length read");

    auto ticket = scheduler_.enqueue(request);
    return Result<ReadTicket>::success(ticket);
}

void StorageSchedulerEngine::cancelRead(ReadTicket ticket) {
    scheduler_.cancel(ticket);
}

StorageProfile StorageSchedulerEngine::getStorageProfile() const {
    return profile_;
}

} // namespace gopost::storage
