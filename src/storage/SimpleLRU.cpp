#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    int size_delta = key.size() + value.size();
    if(size_delta > _max_size) {
        return false;
    }

    auto it = _lru_index.find(key);
    bool key_in = false;
    if (it != _lru_index.end()) {
        size_delta -= (it->second.get().value.size() + key.size());
        key_in = true;
    }
    //now size is known, cutting heads
    while(size_delta + _occup_size > _max_size) {
        _delete_head();
    }

    if(key_in) { //update value
        auto& old_value = it->second.get().value;
        _occup_size += size_delta;
        old_value = value;
    }
    else { //insert element
        _insert_tail(key, value);
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if(it == _lru_index.end()) {
        while(value.size() + key.size() + _occup_size > _max_size) {
            _delete_head();
        }

        _insert_tail(key, value);
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(key);
    if(it == _lru_index.end()) { //no key
        return false;
    }

    else {
        int size_delta = value.size() - it->second.get().value.size();

        while(size_delta + _occup_size > _max_size) {
            _delete_head();
        }

        //update_value
        auto& old_value = it->second.get().value;
        _occup_size += size_delta;
        old_value = value;

        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        auto node = &it->second.get();

        _occup_size -= key.size() + it->second.get().value.size();
        _lru_index.erase(it->first.get());

        if((node == _lru_tail) && (node == _lru_head.get())) {
            _lru_head.reset();
            _lru_tail = nullptr;
        }

        else if (node == _lru_tail) {
            _lru_tail = _lru_tail->prev;
            _lru_tail -> next.reset();
        }

        else if (node == _lru_head.get()) {
            _lru_head->next->prev = nullptr;
            auto tmp = _lru_head->next.release();
            _lru_head.reset(tmp);
            }

        else {
            node->next->prev = node->prev;
            std::swap(node->prev->next, node->next);
        }
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        value = it->second.get().value;

        //now put it to tail
        auto node = &it->second.get();
        if (node != _lru_tail) {
            //if node is head, then it has no prev
            if (node == _lru_head.get()) {
                //swapping head and tail
                std::swap(_lru_head, node->next);
                _lru_head->prev = nullptr;
                //now node->next == tail is head
                //then just swap nexts
            }
            else {
                node->next->prev = node->prev;
                std::swap(node->prev->next, node->next);
                //now node->next points on itself
            }

            std::swap(_lru_tail->next, node->next);
            node->prev = _lru_tail;
            _lru_tail = node;
        }
        return true;
    }
    else {
        return false;
    }
}

void SimpleLRU::_delete_head(){
    _lru_index.erase(_lru_head->key);
    if(_lru_head.get() == _lru_tail) {
        //if last left
        _occup_size -= (_lru_tail->value.size() + _lru_tail->key.size());

        _lru_head.reset();
        _lru_tail = nullptr;
    }

    else {
        _lru_head->next->prev = nullptr;
        auto tmp = _lru_head->next.release();
        _lru_head.reset(tmp);
        _occup_size -= (_lru_head->value.size() + _lru_head->key.size());
    }
}

void SimpleLRU::_insert_tail(const std::string &key, const std::string &value){
    _occup_size += key.size() + value.size();
    auto new_node = new lru_node(key, value);
    if (_lru_head != nullptr) {
        new_node->prev = _lru_tail;
        new_node->next.reset();

        _lru_tail->next.reset(new_node);
        _lru_tail = _lru_tail->next.get();
    }

    //first elem in storage
    else {
        _lru_tail = new_node;
        _lru_head.reset(new_node);
    }

    _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),std::reference_wrapper<lru_node>(*_lru_tail)));
}

} // namespace Backend
} // namespace Afina
