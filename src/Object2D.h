//
// Created by ivan- on 18.01.2020.
//

#ifndef PSEUDO3DENGINE_OBJECT2D_H
#define PSEUDO3DENGINE_OBJECT2D_H

#include <utility>
#include <vector>
#include <cmath>
#include "Idrawable.h"
#include "settings.h"
#include "Point2D.h"
#include "ResourceManager.h"

class Object2D : virtual public Idrawable
{
protected:
    std::vector<Point2D> v_points2D;

    Point2D p_position;
    Point2D p_velocity;

    std::string s_name;
    double d_height = 1;
    bool b_isMirror = false;

    sf::Texture& T_texture;
public:
    Object2D(Point2D position = { 0, 0 }, std::vector<Point2D> points = {}, double height = 1, std::string texture = WALL_TEXTURE, Point2D velocity = { 0, 0 })
        : p_position(position), v_points2D(points), p_velocity(velocity), d_height(height), T_texture(*ResourceManager::loadTexture(texture)) {}

    Object2D(const Object2D& object2D) : T_texture(object2D.T_texture) // copy constructor
    {
        v_points2D = object2D.v_points2D;
        p_position = object2D.p_position;
        p_velocity = object2D.p_velocity;
        s_name = object2D.s_name;
        d_height = object2D.d_height;
        b_isMirror = object2D.b_isMirror;
    }

    const double x() { return p_position.x; }
    const double y() { return p_position.y; }

    void setPosition(Point2D position) { p_position = position; };

    void shift(Point2D vector) { p_position += vector; }

    std::vector<Point2D>& nodes() { return v_points2D; }

    static bool segments_crossing(std::pair<Point2D, Point2D> segment1, std::pair<Point2D, Point2D> segment2, Point2D& point);
    virtual bool cross(const std::pair<Point2D, Point2D>& ray, std::pair<Point2D, Point2D>& wall, Point2D& point, double& uv);

    void setName(std::string name) { s_name = name; }
    std::string getName() const { return s_name; }

    Point2D position() const { return p_position; }

    bool isMirror() const { return b_isMirror; }
    void makeMirror() { b_isMirror = true; }
    void makeItNotMirror() { b_isMirror = false; }

    void draw(sf::RenderTarget& window) override;

    double height() const { return d_height; }

    sf::Texture& loadTexture() { return T_texture; }

    virtual int type() { return 0; }
    void setPoints2D(std::vector<Point2D> points2D) { v_points2D = std::move(points2D); }
};


#endif //PSEUDO3DENGINE_OBJECT2D_H
