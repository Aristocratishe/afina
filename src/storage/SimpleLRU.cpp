#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::addNewNode(const std::string &key, const std::string &value) {
    std::unique_ptr<lru_node> newNode(new lru_node{key, value, nullptr, nullptr});
    currSize += key.size() + value.size();
    if (!_lru_head) {
        if (currSize > _max_size) {
            currSize -= key.size() + value.size();
            return false;
        }
        _lru_head = std::move(newNode);
        _lru_tail = _lru_head.get();
        _lru_index.emplace(_lru_tail->key, *_lru_head);
    }
    else {
        while (currSize > _max_size) {
            deleteNode(*_lru_head);
        }
        _lru_tail->next = std::move(newNode);
        _lru_tail->next->prev = _lru_tail;
        _lru_tail = _lru_tail->next.get();
        _lru_index.emplace(_lru_tail->key, *_lru_tail);
    }
    return true;
}

void SimpleLRU::deleteNode(std::reference_wrapper<lru_node> currNode) {
    if (!currNode.get().prev && !currNode.get().next) {
        currSize = 0;
        _lru_index.erase(currNode.get().key);
        _lru_head.reset();
        _lru_tail = nullptr;
    }
    else if (!currNode.get().prev) {
        currSize -= currNode.get().key.size() + currNode.get().value.size();
        _lru_index.erase(currNode.get().key);
        currNode.get().next->prev = nullptr;
        _lru_head = std::move(currNode.get().next);
    }
    else if (!currNode.get().next) {
        currSize -= currNode.get().key.size() + currNode.get().value.size();
        _lru_index.erase(currNode.get().key);
        _lru_tail = currNode.get().prev;
        currNode.get().prev->next.reset();
    }
    else {
        currSize -= currNode.get().key.size() + currNode.get().value.size();
        _lru_index.erase(currNode.get().key);
        currNode.get().next->prev = currNode.get().prev;
        currNode.get().prev->next = std::move(currNode.get().next);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.empty()) {
        return false;
    }
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return addNewNode(key, value);
    }
    currSize += value.size() - it->second.get().value.size();
    if (currSize > _max_size && _lru_index.size() == 1) {
        currSize += it->second.get().value.size() - value.size();
        return false;
    }
    while (currSize > _max_size && _lru_index.size() > 1) {
        deleteNode(*_lru_head);
    }
    it->second.get().value = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) == _lru_index.end()) {
        return addNewNode(key, value);
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    currSize -= it->second.get().value.size();
    it->second.get().value = value;
    currSize += value.size();
    while (currSize >= _max_size) {
        deleteNode(*_lru_head);
    }
    deleteNode(it->second);
    return addNewNode(key, value);;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    deleteNode(it->second);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    value = it->second.get().value;
    deleteNode(it->second);
    return addNewNode(key, value);
}

} // namespace Backend
} // namespace Afina
