#pragma once


class Database;


class Framework
{
public:
    Framework(Database* database);

    static Framework* get() 
    { 
        // TODO(gpascualg): Assert _instance is not null (ie. setup has been run)
        return _instance;
    }

    inline Database* database() { return _database; }

private:
    static Framework *_instance;
    Database* _database;
};
