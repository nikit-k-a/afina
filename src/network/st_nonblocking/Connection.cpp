#include "Connection.h"

#include <iostream>



#include <sys/socket.h>
#include <unistd.h>

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() {
    _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
    /*
    EPOLLHUP and EPOLLERR
    are always monitored
    why should I add them?

    EPOLLRDHUP for OnCLose
    */

    //in man - epollerr not necessary
    //EPOLLPRI - for urgent data (such as unexpected end of connection by keyboard)
    //EPOLLPRI is not used in ServerImpl

    _event.data.fd = _socket;
    _event.data.ptr = this;
    _logger->debug("Start connection on socket {}", _socket);
}

// See Connection.h
void Connection::OnError() {
    _logger->warn("Error on socket {}, connection would be closed", _socket);
    _isAlive = false;
}

// See Connection.h
void Connection::OnClose() {
    _logger->debug("Client on socket {} closed connection", _socket);
    _isAlive = false;
    //debug because its regular shutdown
    //unexpexted shutdown is on EPOLLHUP
 }

// See Connection.h
void Connection::DoRead() {
    _logger->debug("Start reading on socket {}", _socket);
    try {
        int readed_bytes = -1;
        while ((readed_bytes = read(_socket, _client_buf, sizeof(_client_buf))) > 0) {
            _logger->debug("Got {} bytes from socket", readed_bytes);

            // Single block of data readed from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            while (readed_bytes > 0) {
                _logger->debug("Process {} bytes", readed_bytes);
                // There is no command yet
                if (!_command_to_execute) {
                    std::size_t parsed = 0;
                    if (_parser.Parse(_client_buf, readed_bytes, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        _logger->debug("Found new command: {} in {} bytes", _parser.Name(), parsed);
                        _command_to_execute = _parser.Build(_arg_remains);
                        if (_arg_remains > 0) {
                            _arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(_client_buf, _client_buf + parsed, readed_bytes - parsed);
                        readed_bytes -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (_command_to_execute && _arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", readed_bytes, _arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(_arg_remains, std::size_t(readed_bytes));
                    _argument_for_command.append(_client_buf, to_read);

                    std::memmove(_client_buf, _client_buf + to_read, readed_bytes - to_read);
                    _arg_remains -= to_read;
                    readed_bytes -= to_read;
                }

                // There is command & argument - RUN!
                if (_command_to_execute && _arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (_argument_for_command.size()) {
                        _argument_for_command.resize(_argument_for_command.size() - 2);
                    }
                    _command_to_execute->Execute(*_storage, _argument_for_command, result);

                    // Send response
                    result += "\r\n";


                    bool empty_before = _result_queue.empty();
                    /*because a situation may occur:
                    events is already set to EPOLLOUT,
                    but queue is empty -> error writing
                    */
                    _result_queue.push_back(result);

                    if(empty_before){
                        _event.events = EPOLLIN | EPOLLRDHUP | EPOLLOUT| EPOLLERR | EPOLLHUP;
                    }

                    // Prepare for the next command
                    _command_to_execute.reset();
                    _argument_for_command.resize(0);
                    _parser.Reset();
                }
            } // while (readed_bytes)
        }

        if (readed_bytes == 0) {
            _read_all = true;
            _logger->debug("Everything was read");
        }

        else {
            printf("bytes = %d", readed_bytes);
            throw std::runtime_error(std::string(strerror(errno)));
        }
    }
    catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
        _isAlive = false;
    }
 }

// See Connection.h
void Connection::DoWrite() {
    _logger->debug("Start reading on socket {}", _socket);
    if(!_isAlive){
        _logger->debug("Connection on socket {} is dead, cancel writing", _socket);
        return;
    }

    int ret = -1;
    auto it = _result_queue.begin();
    do {
        std::string &qhead = *it;
        ret = write(_socket, &qhead[0] + _head_offset, qhead.size() - _head_offset);

        if (ret > 0) {
            _head_offset += ret;
            if (_head_offset >= it->size()) {
                it++;
                _head_offset = 0;
            }
        }
    } while (ret > 0 && it != _result_queue.end());

    //remove what was read from head
    it--;
    _result_queue.erase(_result_queue.begin(), it);

    if (-1 == ret){
        _isAlive = false;
        _logger->error("Failed to write on socket {}", _socket);
    }

    if (_result_queue.empty()){
        _event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP;
        if(_read_all){
            _logger->debug("Connection on socket {} done", _socket);
            _isAlive = false;
        }
    }

}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
