#include "aura_server.hpp"
#include "aura_client.hpp"
#include "debug.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


const uint16_t MAX_PACKET_LEN = 1000;

void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    AuraClient* auraClient = (AuraClient*)client;
    auraClient->setId(AtomicAutoIncrement<0>::get());
    _clients.emplace(auraClient->getId(), auraClient);

    Server::handleAccept(client, error);
    client->scheduleRead(12);
}

void AuraServer::handleRead(Client* client, const boost::system::error_code& error, size_t size)
{
#if IF_LOG(LOG_PACKETS)
    if (!error)
    {
        printf("READ: %d\n", size);
        for (int i = 0; i < client->totalRead(); ++i)
        {
            printf("%2X ", (uint8_t)client->data()[i]);
        }
        printf("\n");
    }
#endif

    bool disconnect = false;

    if (!error)
    {
        uint16_t len = 2;
        bool reset = false;

        switch (client->readPhase())
        {
            // Opcode
            case 1:
                break;

            // Length
            case 2:
                len = *(uint16_t*)(client->data() + 2);
                if (len >= MAX_PACKET_LEN)
                {
                    disconnect = true;
                }
                break;

            case 3:
                reset = true;
                break;
        }

        if (!disconnect)
        {
            if (reset)
            {
                // TODO(gpascualg): Add packet to parsing list
            }

            client->scheduleRead(len, reset);
        }
    }
    
    if (error || disconnect)
    {
        client->socket()->close();
    }
}
