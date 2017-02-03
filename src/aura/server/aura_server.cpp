#include "aura_server.hpp"
#include "aura_client.hpp"
#include "debug.hpp"
#include "packet.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


const uint16_t MAX_PACKET_LEN = 1000;

void onSend(const boost::system::error_code & error, std::size_t size, Packet* packet)
{
    LOG(LOG_DEBUG, "Written (%d): %d", (int)error.value(), (uint16_t)size);
    packet->destroy();
}

void AuraServer::handleAccept(AuraClient* client, const boost::system::error_code& error)
{
    client->setId(AtomicAutoIncrement<0>::get());
    _clients.emplace(client->id(), client);

    Server::handleAccept(client, error);
    client->scheduleRead(2);

    Packet* packet = Packet::create();
    *packet << uint16_t{ 0x7BCD };
    *packet << uint16_t{ 0x1 };
    *packet << uint8_t{ 0x8 };

    boost::asio::async_write(client->socket(), packet->sendBuffer(), 
        [packet](const boost::system::error_code & error, std::size_t size)
        {
            onSend(error, size, packet);
        }
    );
}

void AuraServer::handleRead(AuraClient* client, const boost::system::error_code& error, size_t size)
{
#if IF_LOG(LOG_PACKETS) || 1
    if (!error)
    {
        printf("%" PRId64 " - READ: %d\n", time(NULL), size);
        for (uint16_t i = 0; i < client->packet()->size(); ++i)
        {
            printf("%.2X ", (uint8_t)client->packet()->data()[i]);
        }
        printf("\n");
    }
#endif

    bool disconnect = false;
    uint16_t len = 2;
    bool reset = false;

    switch (client->readPhase())
    {
        // Opcode
        case 1:
            break;

        // Length
        case 2:
            len = client->packet()->peek<uint16_t>(2);
            if (len >= MAX_PACKET_LEN)
            {
                disconnect = true;
            }
            
            // No data in packet
            reset = len == 0;
            break;

        case 3:
            reset = true;
            break;
    }

    if (!disconnect)
    {
        if (reset)
        {
            // Force read 2 bytes now (opcode)
            len = 2;

            LOG(LOG_DEBUG, "[PACKET DONE]");

            // TODO(gpascualg): Add packet to parsing list
            Packet* packet = Packet::create();
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };
            *packet << uint16_t{ 0x0001 } << uint16_t{ 0x0 };
            *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x2 } << uint16_t{ 0x8080 };

            boost::asio::async_write(client->socket(), packet->sendBuffer(), 
                [packet](const boost::system::error_code & error, std::size_t size)
                {
                    onSend(error, size, packet);
                }
            );
        }

        client->scheduleRead(len, reset);
    }
    else
    {
        client->close();
    }
}
