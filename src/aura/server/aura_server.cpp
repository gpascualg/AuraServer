#include "aura_server.hpp"
#include "aura_client.hpp"
#include "debug.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


const uint16_t MAX_PACKET_LEN = 1000;

void on_write(const boost::system::error_code & error, std::size_t size)
{
    printf("WRITE (%d): %d\n", error.value(), size);
}

void AuraServer::handleAccept(AuraClient* client, const boost::system::error_code& error)
{
    client->setId(AtomicAutoIncrement<0>::get());
    _clients.emplace(client->id(), client);

    Server::handleAccept(client, error);
    client->scheduleRead(2);

    boost::asio::async_write(client->socket(), boost::asio::buffer("OK!", 3), &on_write);
}

void AuraServer::handleRead(AuraClient* client, const boost::system::error_code& error, size_t size)
{
#if IF_LOG(LOG_PACKETS) || 1
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
                boost::asio::async_write(client->socket(), boost::asio::buffer("OK!", 3), &on_write);
                //boost::asio::du
            }

            client->scheduleRead(len, reset);
        }
    }
    
    if (error || disconnect)
    {
        client->close();
    }
}
