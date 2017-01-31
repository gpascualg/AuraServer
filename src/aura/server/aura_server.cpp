#include "aura_server.hpp"
#include "aura_client.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    AuraClient* auraClient = (AuraClient*)client;
    auraClient->setId(AtomicAutoIncrement<0>::get());
    _clients.emplace(auraClient->getId(), auraClient);

    Server::handleAccept(client, error);
    client->scheduleRead(2);
}

void AuraServer::handleRead(Client* client, const boost::system::error_code& error, size_t size)
{
    if (!error)
    {
        client->data()[client->totalRead()] = '\0';
        printf("MSG (%d): %s\n", error.value(), client->data());
        client->scheduleRead(1);
    }
    else
    {
        client->socket()->close();
    }
}
