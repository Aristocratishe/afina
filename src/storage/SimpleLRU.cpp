#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.empty()) {
        return false;
    }
    auto it = _lru_index.find(key);
    if (_lru_index.find(key) == _lru_index.end()) {
        std::unique_ptr<lru_node> newNode(new lru_node{key, value, nullptr, nullptr});
        if (!_lru_head) {
            _lru_head = std::move(newNode);
            _lru_tail = _lru_head.get();
            _lru_index.emplace(std::ref(key), *_lru_head);
            currSize += key.size() + value.size();
            return true;
        }
        currSize += key.size() + value.size();
        while (currSize > _max_size) {
            Delete(_lru_tail->key);
        }
        newNode->next = std::move(_lru_head);
        newNode->next->prev = newNode.get();
        _lru_head = std::move(newNode);
        _lru_index.emplace(key, *_lru_head);
        return true;
    }
    currSize -= it->second.get().value.size() - value.size();
    it->second.get().value = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) == _lru_index.end()) {
        std::unique_ptr<lru_node> newNode(new lru_node{key, value, nullptr, nullptr});
        if (!_lru_head) {
            _lru_head = std::move(newNode);
            _lru_index.emplace(key, *_lru_head);
            currSize += key.size() + value.size();
            return true;
        }
        currSize += key.size() + value.size();
        while (currSize > _max_size) {
            Delete(_lru_tail->key);
        }
        newNode->next = std::move(_lru_head);
        newNode->next->prev = newNode.get();
        _lru_head = std::move(newNode);
        _lru_index.emplace(key, *_lru_head);
        currSize += key.size() + value.size();
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    it->second.get().value = value;
    currSize -= it->second.get().value.size();
    currSize += value.size();
    while (currSize >= _max_size) {
        Delete(_lru_tail->key);
    }
    Delete(key);
    Put(key, value);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    auto currNode = it->second;
    if (!currNode.get().prev && !currNode.get().next) {
        _lru_index.erase(key);
        _lru_head.reset();
        _lru_tail = nullptr;
        currSize = 0;
        return true;
    }
    if(currNode.get().prev == nullptr) {
        currSize -= key.size() + currNode.get().value.size();
        _lru_index.erase(key);
        currNode.get().next->prev = nullptr;
        _lru_head = std::move(currNode.get().next);
        return true;
    }
    if (currNode.get().next == nullptr) {
        currSize -= key.size() + currNode.get().value.size();
        _lru_index.erase(key);
        currNode.get().prev->next = nullptr;
        _lru_tail = currNode.get().prev;
        return true;
    }
    currSize -= key.size() + currNode.get().value.size();
    _lru_index.erase(key);
    currNode.get().prev->next = std::move(currNode.get().next);
    currNode.get().prev->next->prev = currNode.get().prev;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    value = it->second.get().value;
    Delete(key);
    Put(key, value);
    return true;
}

} // namespace Backend
} // namespace Afina
