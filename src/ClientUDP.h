//
// Created by Neirokan on 30.04.2020
//

#ifndef PSEUDO3DENGINE_CLIENTUDP_H
#define PSEUDO3DENGINE_CLIENTUDP_H

#include "World.h"
#include "Camera.h"
#include "ReliableMsg.h"
#include "UDPSocket.h"

class ClientUDP
{
private:
    World& world;
    UDPSocket socket;
    double lastBroadcast;
    bool working;

    std::map<sf::Uint16, Camera> players;
    std::vector<Point2D> spawns = { {1.5, 1.5}, {1.5, 9} };

public:
    ClientUDP(World& world);
    bool isWorking() const;
    bool connected() const;
    Camera& camera();
    void connect(sf::IpAddress ip, sf::Uint16 port);
    void disconnect();
    void update();
    bool process();
    bool timeout(sf::Uint16 id);
    void shoot(std::string& name, double damage, double distance);
};


#endif //PSEUDO3DENGINE_CLIENTUDP_H
