#include "framework/framework.hpp"

Framework* Framework::_instance = nullptr;

Framework::Framework(Database* database)
{
    LOG_ASSERT(_instance == nullptr, "A Framework instance already exists");

    _instance = this;
    _database = database;
}
