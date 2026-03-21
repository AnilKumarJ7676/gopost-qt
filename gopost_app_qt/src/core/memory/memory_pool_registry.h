#pragma once

#include "core/interfaces/IMemoryBudget.h"
#include "core/memory/memory_pool.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace gopost::memory {

using core::interfaces::MemoryBudgets;

class MemoryPoolRegistry {
public:
    void initialize(const MemoryBudgets& budgets);

    MemoryPool* pool(const std::string& name);
    const MemoryPool* pool(const std::string& name) const;
    std::vector<std::string> poolNames() const;

private:
    std::unordered_map<std::string, std::unique_ptr<MemoryPool>> pools_;
};

} // namespace gopost::memory
