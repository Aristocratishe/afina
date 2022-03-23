#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


bool SimpleLRU::addNewNode(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size)
        return false;
    std::unique_ptr<lru_node> newNode(new lru_node{key, value, nullptr, nullptr});
    if (!_lru_head) {
        _lru_head = std::move(newNode);
        _lru_tail = _lru_head.get();
        _lru_index.emplace(_lru_head->key, *_lru_head);
        currSize += key.size() + value.size();
        return true;
    }
    while (currSize + key.size() + value.size() > _max_size) {
        deleteNode(*_lru_tail);
    }
    _lru_head->prev = newNode.get();
    newNode->next = std::move(_lru_head);
    _lru_head = std::move(newNode);
    _lru_index.emplace(_lru_head->key, *_lru_head);
    currSize += key.size() + value.size();
    return true;
}

void SimpleLRU::deleteNode(std::reference_wrapper<lru_node> currNode) {
    if (!currNode.get().prev && !currNode.get().next) {
        currSize = 0;
        _lru_index.erase(currNode.get().key);
        _lru_head.reset();
        _lru_tail = nullptr;
        return;
    }
    currSize -= currNode.get().key.size() + currNode.get().value.size();
    _lru_index.erase(currNode.get().key);
    if (!currNode.get().prev) {
        currNode.get().next->prev = nullptr;
        _lru_head = std::move(currNode.get().next);
    }
    else if (!currNode.get().next) {
        _lru_tail = currNode.get().prev;
        currNode.get().prev->next.reset();
    }
    else {
        currNode.get().next->prev = currNode.get().prev;
        currNode.get().prev->next = std::move(currNode.get().next);
    }
}

bool SimpleLRU::moveToHead(std::reference_wrapper<lru_node> node) {
    if (!node.get().prev)
        return true;
    std::unique_ptr<lru_node> curr_node(std::move(node.get().prev->next));
    if (node.get().next)
        node.get().next->prev = node.get().prev;
    node.get().prev->next = std::move(node.get().next);
    node.get().prev = nullptr;
    _lru_head->prev = curr_node.get();
    node.get().next = std::move(_lru_head);
    _lru_head = std::move(curr_node);
    return true;
}

bool SimpleLRU::updateNode(std::reference_wrapper<lru_node> node, const std::string &value) {
    if (node.get().key.size() + value.size() > _max_size)
        return false;
    currSize += value.size() - node.get().value.size();
    moveToHead(node);
    while (currSize > _max_size && _lru_index.size() > 1) {
        deleteNode(*_lru_tail);
    }
    node.get().value = value;
    return true;
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
    return updateNode(it->second, value);
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
    if (it == _lru_index.end())
        return false;
    return updateNode(it->second, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end())
        return false;
    deleteNode(it->second);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end())
        return false;
    value = it->second.get().value;
    return moveToHead(it->second);
}

} // namespace Backend
} // namespace Afina