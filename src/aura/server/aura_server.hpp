#pragma once

#include "server.hpp"
#include "client.hpp"

#include "atomic_autoincrement.hpp"

#include <atomic>
#include <unordered_map>


class AuraClient;

class AuraServer : public Server<AuraClient>
{
public:
    using Server::Server;

    void handleAccept(AuraClient* client, const boost::system::error_code& error) override;
    void handleRead(AuraClient* client, const boost::system::error_code& error, size_t size) override;

protected:
    std::unordered_map<uint64_t, AuraClient*> _clients;
};
