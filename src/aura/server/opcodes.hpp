#pragma once

enum class PacketOpcodes
{
    ENTITY_SPAWN =                  0x0101,
    ENTITY_DESPAWN =                0x0102,
    FORWARD_CHANGE =                0x0103,
    SPEED_CHANGE =                  0x0104,

    FORWARD_CHANGE_RESP =           0x0A03,
    SPEED_CHANGE_RESP =             0x0A04,
    
    PING =                          0x0001,
    DISCONNECTION =                 0x0002,
    CHAT =                          0x0011,
    SET_ID =                        0x0012,
};
