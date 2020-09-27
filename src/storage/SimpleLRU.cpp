#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
void SimpleLRU::_put(const std::string &key, const std::string &value, const Map_t::const_iterator &it)
{
    int size_delta = value.size(); //delta can be < 0
    bool key_in = false;
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
    while(size_delta + _occup_size > _max_size)
    {
        //cutting heads
        _lru_index.erase(_lru_head->key);
        if(_lru_head.get() == _lru_tail)
        {
            //if last left
            _occup_size -= (_lru_tail->value.size() + _lru_tail->key.size());

            _lru_head.reset();
            _lru_tail = nullptr;
        }

        else
        {
            _lru_head->next->prev = nullptr;
            auto tmp = _lru_head->next.release();
            _lru_head.reset(tmp);
            _occup_size -= (_lru_head->value.size() + _lru_head->key.size());
        }
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
        _occup_size += key.size() + value.size();
        auto new_node = new lru_node(key, value);
        if (_lru_head != nullptr)
        {
            new_node->prev = _lru_tail;
            new_node->next.reset();

            _lru_tail->next.reset(new_node);
            _lru_tail = _lru_tail->next.get();
        }

        else//first elem in storage
        {
            _lru_tail = new_node;
            _lru_head.reset(new_node);
        }

       _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),std::reference_wrapper<lru_node>(*_lru_tail)));
    }
}

bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    std::size_t curr_sz = key.size() + value.size();
    if(curr_sz > _max_size)
    {
        return false;
    }
    auto it = _lru_index.find(key);

    _put(key, value, it);

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(key);
    if(it == _lru_index.end())
    {
        //no key
        _put(key, value, it);
        return true;
    }

    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(key);
    if(it == _lru_index.end())
    {
        //no key
        return false;
    }

    else
    {
        //no double iteration
        _put(key, value, it);
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
    auto it = _lru_index.find(key);
    if (it != _lru_index.end())
    {
        _delete(key, it);
        return true;
    }

    return false;
}

void SimpleLRU::_delete(const std::string &key, const Map_t::const_iterator &it)
{
    auto node = &it->second.get();

    _occup_size -= key.size() + it->second.get().value.size();
    _lru_index.erase(it->first.get());

    if((node == _lru_tail) && (node == _lru_head.get()))
    {
        _lru_head.reset();
        _lru_tail = nullptr;
    }

    else if (node == _lru_tail)
    {
        _lru_tail = _lru_tail->prev;
        _lru_tail -> next.reset();
    }


    else if (node == _lru_head.get())
    {
        _lru_head->next->prev = nullptr;
        auto tmp = _lru_head->next.release();
        _lru_head.reset(tmp);
        }

    else
    {
        node->next->prev = node->prev;
        std::swap(node->prev->next, node->next);
    }
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    // printf("\n\nGet\n\n");
    auto it = _lru_index.find(key);
    if (it != _lru_index.end())
    {
        value = it->second.get().value;

        //now put it to tail
        auto node = &it->second.get();
        if (node != _lru_tail)
        {
            //if node is head, then it has no prev
            if (node == _lru_head.get())
            {
                //swapping head and tail
                std::swap(_lru_head, node->next);
                _lru_head->prev = nullptr;
                //now node->next == tail is head
                //then just swap nexts
            }
            else
            {
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
    else
    {
        return false;
    }
}

} // namespace Backend
} // namespace Afina
