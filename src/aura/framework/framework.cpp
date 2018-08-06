#include "framework/framework.hpp"

Framework* Framework::_instance = nullptr;

Framework::Framework(Database* database)
{
    // TODO(gpascualg): Assert none exists yet
    // assert _instance == nullptr

    _database = database;
}
