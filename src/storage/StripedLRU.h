#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <vector>

#include "afina/Storage.h"
#include "ThreadSafeSimpleLRU.h"

#define MEMORY_LIMIT 1024
#define STRIPE_COUNT 4

namespace Afina {
namespace Backend {

class StripedLRU: public Afina::Storage {
public:
    StripedLRU();

    ~StripedLRU() = default;

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    size_t getNumOfShard(const std::string &key);

    const std::size_t _memory_limit;

    const std::size_t _stripe_count;

    std::vector<std::unique_ptr<ThreadSafeSimpleLRU>> shards;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_LRU_H