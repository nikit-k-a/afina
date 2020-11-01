#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>

#include <sys/epoll.h>

#include <spdlog/logger.h>

#include <afina/execute/Command.h>


#include "protocol/Parser.h"


namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> &pl, std::shared_ptr<Afina::Storage> &ps) :
        _socket(s), _logger(pl), _storage(ps) {
        _isAlive = true;
        _read_all = false;
        _arg_remains = 0;
        _head_offset = 0;
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive() const { return _isAlive; }

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;

    bool _isAlive;
    bool _read_all;

    std::shared_ptr<spdlog::logger> _logger;
    std::shared_ptr<Afina::Storage> _storage;
    std::unique_ptr<Execute::Command> _command_to_execute;
    Protocol::Parser _parser;

    char _client_buf [4096];
    std::size_t _arg_remains;
    std::size_t _head_offset;
    std::string _argument_for_command;
    std::vector<std::string> _result_queue;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
