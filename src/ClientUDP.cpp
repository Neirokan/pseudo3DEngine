//
// Created by Neirokan on 30.04.2020
//

#include "ClientUDP.h"
#include "Time.h"
#include "MsgType.h"
#include "settings.h"
#include <thread>

ClientUDP::ClientUDP(World& world) : world(world), lastBroadcast(-INFINITY), working(false)
{
    socket.setTimeoutCallback(std::bind(&ClientUDP::timeout, this, std::placeholders::_1));
}

bool ClientUDP::connected() const
{
    return socket.ownId();
}

bool ClientUDP::isWorking() const
{
    return working;
}

Camera& ClientUDP::camera()
{
    return players.at(socket.ownId());
}

void ClientUDP::shoot(std::string& name, double damage, double distance)
{
    sf::Packet packet;
    for (auto&& player : players)
    {
        if (player.second.getName() == name)
        {
            packet << MsgType::Shoot << player.first << damage << distance;
            socket.sendRely(packet, socket.serverId());
            break;
        }
    }
}

void ClientUDP::connect(sf::IpAddress ip, sf::Uint16 port)
{
    sf::Packet packet;
    packet << MsgType::Connect;
    socket.bind(0);
    working = true;
    socket.addConnection(socket.serverId(), ip, port);
    socket.sendRely(packet, socket.serverId());
}

void ClientUDP::update()
{
    if (!working)
        return;

    while (process());

    // World state broadcast

    if (Time::time() - lastBroadcast > 1 / WORLD_UPDATE_RATE && connected())
    {
        sf::Packet updatePacket;
        updatePacket << MsgType::PlayerUpdate << players.at(socket.ownId()).x() << players.at(socket.ownId()).y();
        socket.send(updatePacket, socket.serverId());
        lastBroadcast = Time::time();
    }

    // Socket update

    socket.update();
}

void ClientUDP::disconnect()
{
    for (auto it = players.begin(); it != players.end();)
    {
        world.removeObject2D(it->second.getName());
        players.erase(it++);
    }
    sf::Packet packet;
    packet << MsgType::Disconnect << socket.ownId();
    socket.send(packet, socket.serverId());
    socket.unbind();
    working = false;
}

bool ClientUDP::timeout(sf::Uint16 id)
{
    if (id != socket.serverId())
        return true;
    disconnect();
    return false;
}


// Recive and process message.
// Returns true, if some message was received.
bool ClientUDP::process()
{
    sf::Packet packet;
    sf::Uint16 senderId;
    MsgType type;

    if ((type = socket.receive(packet, senderId)) == MsgType::None)
        return false;
    if (!connected() && type != MsgType::WorldInit)
        return true;

    sf::Packet sendPacket;
    sf::Packet extraPacket;
    sf::Uint16 targetId;
    bool revive;
    double buf[3];

    switch (type)
    {

    case MsgType::Connect:
        packet >> targetId;
        players.insert({ targetId, Camera(world, {2.5, 0}) });
        world.addObject2D(players.at(targetId), "Player" + std::to_string(targetId));
        players.at(socket.ownId()).addCamera(players.at(targetId).getName(), players.at(targetId));
        break;

    case MsgType::Disconnect:
        packet >> targetId;
        if (targetId != socket.ownId() && players.count(targetId))
        {
            world.removeObject2D(players.at(targetId).getName());
            players.at(socket.ownId()).removeCamera(players.at(targetId).getName());
            players.erase(targetId);
        }
        break;

    case MsgType::WorldInit:
        packet >> targetId;
        socket.setId(targetId);
        while (packet >> targetId >> buf[0] >> buf[1] >> buf[2])
        {
            players.insert({ targetId, Camera(world, {2.5, 0}) });
            Camera& camera = players.at(targetId);
            world.addObject2D(camera, "Player" + std::to_string(targetId));
            camera.setPosition({ buf[0], buf[1] });
            camera.setHealth(buf[2]);
        }
        for (auto&& player : players)
        {
            if (player.first != socket.ownId())
            {
                players.at(socket.ownId()).addCamera(player.second.getName(), player.second);
            }
        }
        break;

    case MsgType::WorldUpdate:
        while (packet >> targetId >> buf[0] >> buf[1] >> buf[2])
        {
            if (players.count(targetId))
            {
                Camera& camera = players.at(targetId);
                world.addObject2D(camera, "Player" + std::to_string(targetId));
                if (targetId != socket.ownId())
                    camera.setPosition({ buf[0], buf[1] });
                camera.setHealth(buf[2]);
            }
        }
        break;

    case MsgType::Shoot:
        packet >> revive >> buf[0] >> buf[1];
        if (revive)
            players.at(socket.ownId()).setPosition({ buf[0], buf[1] });
        else
            players.at(socket.ownId()).shiftPrecise({ buf[0], buf[1] });
        break;
    }
    return true;
}
