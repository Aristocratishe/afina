#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.empty()) {
        return false;
    }
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        std::unique_ptr<lru_node> newNode(new lru_node{key, value, nullptr, nullptr});
        if (!_lru_head) {
            _lru_head = std::move(newNode);
            _lru_index.emplace(key, *_lru_head);
            currSize += key.size() + value.size();
        }
        else if (!_lru_head->next) {
            currSize += key.size() + value.size();
            while (currSize > _max_size) {
                Delete(_lru_head->key);
            }
            lru_node* tempNode = nullptr;
            _lru_head->next = std::move(newNode);
            tempNode = _lru_head->next.get();
            tempNode->prev = _lru_head.get();
            _lru_tail = tempNode;
            _lru_index.emplace(key, *_lru_tail);
        }
        else {
            currSize += key.size() + value.size();
            while (currSize > _max_size) {
                Delete(_lru_head->key);
            }
            _lru_tail->next = std::move(newNode);
            lru_node* tempNode = nullptr;
            tempNode = _lru_tail->next.get();
            tempNode->prev = _lru_tail;
            _lru_tail = tempNode;
            _lru_index.emplace(key, *_lru_tail);
        }
        return true;
    }
    currSize -= it->second.get().value.size() - value.size();
    it->second.get().value = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (_lru_index.find(key) == _lru_index.end()) {
        Put(key, value);
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
    currSize -= it->second.get().value.size();
    it->second.get().value = value;
    currSize += value.size();
    while (currSize >= _max_size) {
        Delete(_lru_head->key);
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
        currSize = 0;
        _lru_index.erase(key);
        _lru_head.reset();
        _lru_tail = nullptr;
        return true;
    }
    else if (currNode.get().prev == nullptr) {
        currSize -= key.size() + currNode.get().value.size();
        _lru_index.erase(key);
        currNode.get().next->prev = nullptr;
        _lru_head = std::move(currNode.get().next);
    }
    else if (currNode.get().next == nullptr) {
        currSize -= key.size() + currNode.get().value.size();
        _lru_index.erase(key);
        _lru_tail = currNode.get().prev;
        currNode.get().prev->next.reset();
    }
    else {
        currSize -= key.size() + currNode.get().value.size();
        _lru_index.erase(key);
        currNode.get().next->prev = currNode.get().prev;
        currNode.get().prev->next = std::move(currNode.get().next);
    }
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
