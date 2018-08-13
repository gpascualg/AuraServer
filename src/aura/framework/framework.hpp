#pragma once

#include "debug/debug.hpp"

class Database;


class Framework
{
public:
    Framework(Database* database);

    static Framework* get() 
    { 
        LOG_ASSERT(_instance != nullptr, "No Framework instance has been created");
        return _instance;
    }

    inline Database* database() { return _database; }

private:
    static Framework *_instance;
    Database* _database;
};
