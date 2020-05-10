//
// Created by Neirokan on 30.04.2020
//

#ifndef PSEUDO3DENGINE_SERVERUDP_H
#define PSEUDO3DENGINE_SERVERUDP_H

#include "World.h"
#include "Camera.h"
#include "ReliableMsg.h"
#include "UDPSocket.h"

class ServerUDP
{
private:
    World& world;
    UDPSocket socket;
    double lastBroadcast;
    bool working;

    std::map<sf::Uint16, Camera> players;
    std::vector<Point2D> spawns;

    bool process();
    bool timeout(sf::Uint16 id);

public:
    ServerUDP(World& world);
    bool isWorking() const;
    bool start(sf::Uint16 port);
    void stop();
    void update();
    void addSpawn(Point2D point);
    void clearSpawns();
};


#endif //PSEUDO3DENGINE_SERVERUDP_H
