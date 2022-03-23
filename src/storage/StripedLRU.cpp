#include <functional>
#include <stdexcept>

#include "StripedLRU.h"

namespace Afina {
namespace Backend {

StripedLRU::StripedLRU () : _memory_limit(MEMORY_LIMIT * STRIPE_COUNT), _stripe_count(STRIPE_COUNT) {
    std::size_t stripe_limit = _memory_limit / _stripe_count;
    if (stripe_limit < 1024) {
        throw std::runtime_error("Shard's size is too small. Choose another parameters");
    }
    shards.reserve(_stripe_count);
    for (size_t i = 0; i < _stripe_count; ++i) {
        shards.emplace_back(new ThreadSafeSimpleLRU(stripe_limit));
    }
}

bool StripedLRU::Put(const std::string &key, const std::string &value) {
    return shards[getNumOfShard(key)]->Put(key, value);
}

bool StripedLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    return shards[getNumOfShard(key)]->PutIfAbsent(key, value);
}

bool StripedLRU::Set(const std::string &key, const std::string &value) {
    return shards[getNumOfShard(key)]->Set(key, value);
}

bool StripedLRU::Delete(const std::string &key) {
    return shards[getNumOfShard(key)]->Delete(key);
}

bool StripedLRU::Get(const std::string &key, std::string &value) {
    return shards[getNumOfShard(key)]->Get(key, value);
}

size_t StripedLRU::getNumOfShard(const std::string &key) {
    return std::hash<std::string>{}(key) % shards.size();
}

} // namespace Backend
} // namespace Afina