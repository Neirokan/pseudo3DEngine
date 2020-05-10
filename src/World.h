//
// Created by ivan- on 18.01.2020.
//

#ifndef PSEUDO3DENGINE_WORLD_H
#define PSEUDO3DENGINE_WORLD_H

#include "Object2D.h"
#include <SFML/Audio.hpp>
#include <string>
#include <map>

class World : virtual public Idrawable
{
private:
    mutable std::map<std::string, Object2D&> map_objects;

    double d_length = 0;
    double d_width = 0;

    sf::Texture& T_sky_texture;
    sf::Texture& T_floor_texture;
    sf::Sprite S_floor;

public:
    World(double length, double width, std::string sky_texture = SKY_TEXTURE, std::string floor_texture = FLOOR_TEXTURE) : d_length(length), d_width(width), T_floor_texture(*ResourceManager::loadTexture(floor_texture)), T_sky_texture(*ResourceManager::loadTexture(sky_texture))
    {
        S_floor.setTexture(floorTexture());
    }

    sf::Sprite& floor()
    {
        return S_floor;
    }

    bool addObject2D(Object2D&  object, std::string name)
    {
        object.setName(name);
        return map_objects.insert({name, object}).second;
    }
    Object2D& findObject2D(std::string name) { return map_objects.at(name); }
    Object2D& findObject2D(std::string name) const { return map_objects.at(name); }
    bool isExist(std::string name) const {return map_objects.count(name) != 0; }

    bool removeObject2D(std::string name) { return map_objects.erase(name) > 0; }

    Object2D& operator[](std::string name) { return findObject2D(name); }

    Object2D& operator[](std::string name) const { return findObject2D(name); }

    double width() const { return d_width; }
    double length() const { return d_length; }

    void draw(sf::RenderTarget& window) override;

    const std::map<std::string, Object2D&>& objects() const { return map_objects; }

    const sf::Texture& skyTexture()
    {
        return T_sky_texture;
    }

    const sf::Texture& floorTexture()
    {
        return T_floor_texture;
    }
};


#endif //PSEUDO3DENGINE_WORLD_H
