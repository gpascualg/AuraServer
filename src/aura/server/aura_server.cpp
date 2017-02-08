#include "aura_server.hpp"
#include "debug.hpp"
#include "packet.hpp"
#include "cell.hpp"
#include "client.hpp"
#include "map.hpp"
#include "cluster.hpp"
#include "map_aware_entity.hpp"

#include <thread>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>


const uint16_t MAX_PACKET_LEN = 1000;

Map map;

void AuraServer::mainloop()
{
    while (1)
    {
        map.runScheduledOperations();
        map.cluster()->update(0);
        map.cluster()->runScheduledOperations();

        _packets.consume_all([](auto recv)
            {
                // TODO(gpascualg): Remove this broadcast placeholder
                auto packet = Packet::create();
                *packet << uint16_t{ 0x7BCD } << uint16_t{ 0x0002 } << uint16_t{ 0x8080 };
                recv.client->entity()->cell()->broadcast(packet, true);

                // TODO(gpascualg): Handle packets based on opcode
                recv.packet->destroy();
            }
        );

        runScheduledOperations();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void onSend(const boost::system::error_code & error, std::size_t size, Packet* packet)
{
    LOG(LOG_PACKET_SEND, "Written (%d): %d", (int)error.value(), (uint16_t)size);
    packet->destroy();
}

void AuraServer::handleAccept(Client* client, const boost::system::error_code& error)
{
    // Setup entity
    _clients.emplace(client->id(), client);
    map.addTo(0, 0, client->entity());

    // Start receiving!
    Server::handleAccept(client, error);
    client->scheduleRead(2);
}

void AuraServer::handleRead(Client* client, const boost::system::error_code& error, size_t size)
{
#if IF_LOG(LOG_PACKET_RECV)
    if (!error)
    {
        printf("Recv bytes at [%" PRId64 "] - Size: %d\n", time(NULL), size);
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

            LOG(LOG_PACKET_RECV, "[PACKET DONE]");

            _packets.push({
                Packet::copy(client->packet(), client->packet()->size()),
                client
            });
        }

        client->scheduleRead(len, reset);
    }
    else
    {
        client->close();
    }
}
