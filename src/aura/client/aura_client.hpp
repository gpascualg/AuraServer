#pragma once

#include "client.hpp"


class AuraClient : public Client
{
public:
    using Client::Client;

    inline void setId(uint64_t id) { _id = id; }
    inline uint64_t getId() { return _id; }

private:
    uint64_t _id;
};
