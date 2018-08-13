#include "database/database.hpp"
#include "framework/framework.hpp"
#include "map/cell.hpp"
#include "map/map-cluster/cluster.hpp"
#include "map/map.hpp"
#include "map/map_aware_entity.hpp"
#include "server/server.hpp"
#include "server/client.hpp"
#include "server/aura_server.hpp"

#include <stdio.h>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <thread>

#include <boost/pool/object_pool.hpp>
#include <boost/asio.hpp>

int main()
{
    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::nanoseconds ns;

#ifdef _MSC_VER
    HWND console = GetConsoleWindow();
    RECT r;
    GetWindowRect(console, &r);
    
    SetWindowPos(console, HWND_TOPMOST, r.left, r.top, 900, 300, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    MoveWindow(console, r.left, r.top, 900, 300, TRUE);
#endif

    Database database({}, 1);
    Framework framework(&database);
    AuraServer server(12345);

    server.startAccept();
    std::thread mainThread(&AuraServer::mainloop, &server);
    std::thread ioThread(&AuraServer::updateIO, &server);
    
    mainThread.join();
    ioThread.join();

    return 0;
}
