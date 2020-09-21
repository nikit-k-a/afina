#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    printf("\n\nPut\n\n");
    std::size_t curr_sz = key.size() + value.size();
    if(curr_sz > _max_size)
    {
        return false;
    }
    //ищем, есть ли такой ключ уже
    int size_delta = value.size(); //can be < 0
    bool key_in = false;
    auto it = _lru_index.find(key);
    if (it != _lru_index.end())
    {
        //found
        size_delta -= it->second.get().value.size();
        key_in = true;
    }
    else
    {
        //not found
        size_delta += key.size();
    }
    //now size is known
    int i = 0;
    while(size_delta + _occup_size > _max_size)
    {
        //govno
        printf("\n\nFREING SIZE_0\n\n");
        //free the space, cut tail
        _lru_tail = _lru_tail->prev.get();
        printf("\n\nFREING SIZE_1\n\n");

        _occup_size -= _lru_tail->next->value.size()+ _lru_tail->next->key.size();
        // _lru_tail->next = nullptr;//???
        printf("\n\nFREING SIZE_2\n\n");

        if(i > 0) delete _lru_tail->next;
        i++;

        _lru_index.erase(_lru_head->key);
    }

    if(key_in)
    {
        auto& old_value = it->second.get().value;
        _occup_size += size_delta;
        old_value = value;
    }

    else
    {
        //enough space, time to insert
        _occup_size += curr_sz;
        auto new_node = new lru_node(key, value);
        if (_lru_head != nullptr)
        {
            /* this works, but bad destructor
            _lru_head->prev = std::unique_ptr<lru_node>(new lru_node (key, value));
            _lru_head->prev->next = _lru_head.get();

            _lru_head.swap(_lru_head->prev);
            */

            new_node->next = _lru_head.get();
            new_node->prev.reset();

            auto tmp = _lru_head.release();
            _lru_head.reset(new_node);

            tmp->prev.reset(new_node);
        }
        else//first elem in storage
        {
            _lru_head.reset(new_node);
            _lru_tail = new_node;//_lru_head.get();

        }

       _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_head->key),std::reference_wrapper<lru_node>(*_lru_head)));
    }

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    //not effective
    if(_lru_index.count(key))
    {
        return false;
    }
    return Put(key, value);

    // return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) { return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    printf("\n\nGet\n\n");
    auto it = _lru_index.find(key);
    if (it != _lru_index.end())
    {
        value = it->second.get().value;
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace Backend
} // namespace Afina
