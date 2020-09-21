#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage
{
public:
    SimpleLRU(size_t max_size = 1024):
        _max_size(max_size),
        _occup_size(0),
        _lru_head(nullptr),
        _lru_tail(nullptr)
        {}

    ~SimpleLRU()
    {
        _lru_index.clear();
//TODO!!!
        if(_lru_tail != nullptr)
        {
            printf("\n\n\nDestructor\n\n\n");

            //from head?
            while(_lru_tail->prev != nullptr)
            {
                // if (i > 20000) exit(3);
                printf("\ntail = %p", _lru_tail);
                printf("\ntail prev = %p", _lru_tail->prev.get());
                // printf("\nhead = %p", _lru_head.get());
                // printf("\nhead prev = %p", _lru_head->prev.get());
                // printf("\nhead next = %p", _lru_head->next);


                _lru_tail = _lru_tail->prev.get();
                printf("\n\nFF");
                printf("\n\ndeleting.. = %p", _lru_tail->next);
                // if (i > 0) delete _lru_tail->next;
            }
        // delete _lru_tail;
        _lru_tail = nullptr;
        _lru_head.reset();
        }
        printf("\n\nEnd_Destr\n\n");
    }

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
    // LRU cache node
    using lru_node = struct lru_node
    {
        lru_node(const std::string &key, const std::string &value):
            key(key),
            value(value),
            prev(nullptr),
            next(nullptr)
            {}

        const std::string key;
        std::string value;
        std::unique_ptr<lru_node> prev;
        lru_node *next;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;

    //Number of occupied bytes
    std::size_t _occup_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;

    //good not to go through list to reach tail
    lru_node* _lru_tail;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
