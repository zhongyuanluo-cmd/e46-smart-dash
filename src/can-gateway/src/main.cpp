#include <cstdio>
#include "GatewayDaemon.h"

int main(int argc, char* argv[])
{
    e46::gateway::GatewayDaemon daemon;

    if (!daemon.init(argc, argv)) {
        return 1;
    }

    return daemon.run();
}
