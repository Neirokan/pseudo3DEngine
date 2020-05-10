//
// Created by Neirokan on 30.04.2020
//

#include "ServerUDP.h"
#include "Time.h"
#include "MsgType.h"
#include "settings.h"

ServerUDP::ServerUDP(World& world) : world(world), lastBroadcast(-INFINITY), working(false)
{
    socket.setTimeoutCallback(std::bind(&ServerUDP::timeout, this, std::placeholders::_1));
}

bool ServerUDP::isWorking() const
{
    return working;
}

bool ServerUDP::start(sf::Uint16 port)
{
    return working = socket.bind(port);
}

void ServerUDP::update()
{
    if (!working)
        return;

    while (process());

    // World state broadcast

    if (Time::time() - lastBroadcast > 1 / WORLD_UPDATE_RATE)
    {
        sf::Packet updatePacket;
        updatePacket << MsgType::WorldUpdate;

        for (auto&& player : players)
        {
            Camera& camera = player.second;
            camera.reduceHealth(-1 * (Time::time() - lastBroadcast));
            updatePacket << player.first << camera.position().x << camera.position().y << camera.health();
        }

        for (auto&& player : players)
        {
            socket.send(updatePacket, player.first);
        }

        lastBroadcast = Time::time();
    }

    // Socket update

    socket.update();
}

void ServerUDP::stop()
{
    for (auto it = players.begin(); it != players.end();)
    {
        players.erase(it++);
    }
    socket.unbind();
    working = false;
}

void ServerUDP::addSpawn(Point2D point)
{
    spawns.push_back(point);
}

void ServerUDP::clearSpawns()
{
    spawns.clear();
}

bool ServerUDP::timeout(sf::Uint16 playerId)
{
    sf::Packet packet;
    packet << MsgType::Disconnect << playerId;

    players.erase(playerId);
    for (auto&& player : players)
    {
        socket.sendRely(packet, player.first);
    }

    return true;
}

// Recive and process message.
// Returns true, if some message was received.
bool ServerUDP::process()
{
    sf::Packet packet;
    sf::Uint16 senderId;
    MsgType type;

    if ((type = socket.receive(packet, senderId)) == MsgType::None)
        return false;

    sf::Packet sendPacket;
    sf::Packet extraPacket;
    sf::Uint16 targetId;
    double buf[2];

    switch (type)
    {

    case MsgType::Connect:
        extraPacket << MsgType::Connect << senderId;
        sendPacket << MsgType::WorldInit << senderId;
        players.insert({ senderId, Camera(world, spawns[senderId % spawns.size()]) });
        for (auto&& player : players)
        {
            Camera& camera = player.second;
            sendPacket << player.first << camera.x() << camera.y() << camera.health();
            if (player.first != senderId)
                socket.sendRely(extraPacket, player.first);
        }
        socket.sendRely(sendPacket, senderId);

        break;

    case MsgType::Disconnect:
        sendPacket << MsgType::Disconnect << senderId;
        players.erase(senderId);
        socket.removeConnection(senderId);
        for (auto&& player : players)
        {
            socket.sendRely(sendPacket, player.first);
        }
        break;

    case MsgType::PlayerUpdate:
        packet >> buf[0] >> buf[1];
        players.at(senderId).setPosition({ buf[0], buf[1] });
        break;

    case MsgType::Shoot:
        packet >> targetId >> buf[0] >> buf[1];
        sendPacket << MsgType::Shoot;
        
        if (players.at(targetId).reduceHealth(buf[0] / buf[1]))
        {
            sendPacket << true << spawns[targetId % spawns.size()].x << spawns[targetId % spawns.size()].y;
            players.at(targetId).setHealth(100);
            players.at(targetId).setPosition(spawns[targetId % spawns.size()]);
        }
        else
        {
            double dir = 2 * PI * rand() / RAND_MAX;
            sendPacket << false << 0.05 * cos(dir) << 0.05 * sin(dir);
        }
        socket.sendRely(sendPacket, targetId);
        break;
    }
    return true;
}
