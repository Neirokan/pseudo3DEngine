//
// Created by ivan- on 18.01.2020.
//

#ifndef PSEUDO3DENGINE_CAMERA_H
#define PSEUDO3DENGINE_CAMERA_H

#include <algorithm>
#include "Object2D.h"
#include "World.h"
#include "settings.h"
#include "Menu.h"
#include "Weapon.h"

class ClientUDP;

struct RayCastStructure
{
    double distance;    // How far is this texture
    double progress;    // progress defines the point of texture we should load
    Object2D* object;   // hitted object. We need this to get it's texture.
    double height;      // objects has different height

    std::vector<RayCastStructure> v_mirrorRayCast; // When we have mirror, we should know about all objects we can see
};

struct CollisionInformation
{
    double distance;
    Point2D collisionPoint;
    std::pair<Point2D, Point2D> edge;
};

class Camera : public Circle2D
{
private:
    std::vector<std::vector<RayCastStructure>> v_distances;

    //For collision detection
    std::vector<CollisionInformation> allCollisions;

    double d_direction;
    double d_fieldOfView;
    double d_eyesHeight;
    double d_depth;

    double d_walkSpeed;
    double d_viewSpeed;

    double i_health;

    bool b_collision = true;
    bool b_hadFocus = false;

    World& W_world;

    sf::Vector2i localMousePosition;

    bool b_textures = true;
    bool b_smooth = false;

    std::vector<Weapon> v_weapons;
    int i_selectedWeapon = 0;

    sf::Sound walkSound;

    void objectsRayCrossed(const std::pair<Point2D, Point2D>& ray, std::vector<RayCastStructure>& v_rayCastStruct, const std::string& name, int reflections = 0);
    void hiddenObjectsRayCrossed(const std::pair<Point2D, Point2D>& ray, const std::string& name);
    void drawVerticalStrip(sf::RenderTarget& window, const RayCastStructure& obj, int shift, int f);
    void recursiveDrawing(sf::RenderTarget& window, const std::vector<RayCastStructure>& v_RayCastStructure, int shift, int rec = 1);
    static void recursiveIncreaseDistance(std::vector<RayCastStructure>& v_RayCastStructure, double distance);
    std::pair<double, double> heightInPixels(double distance, double height);

    static double scalarWithNormal(Point2D edge, Point2D vector);

    void fire();
    std::pair<Object2D*, double> cameraRayCheck(RayCastStructure& structure);

    std::map<std::string, Camera&> m_playersOnTheScreen;
    static double directionSin;
    static double directionCos;
    static double horizontalCos[DISTANCES_SEGMENTS];
    static double horizontalSin[DISTANCES_SEGMENTS];
    static double verticalTan[SCREEN_HEIGHT];
    static double angleInited;

    static void drawHealth(sf::RenderTarget& window, int x, int y, int width, int health);
public:
    explicit Camera(World& world, Point2D position, double direction = 0, std::string texture = SKIN, int health = 100, double fieldOfView = PI/2, double height = 0.6, double eyesHeight = 0.5, double depth = 25, double walkSpeed = 1.7, double viewSpeed = .005)
    : W_world(world), Circle2D(COLLISION_DISTANCE, position, height, texture, 4), d_direction(direction), d_fieldOfView(fieldOfView), d_eyesHeight(eyesHeight), d_depth(depth), d_walkSpeed(walkSpeed), d_viewSpeed(viewSpeed), i_health(health)
    {
        Weapon weapon1(100000);
        weapon1.choiceWeapon("shotgun");
        v_weapons.push_back(weapon1);

        walkSound.setBuffer(*ResourceManager::loadSoundBuffer(WALK_SOUND));
        walkSound.setLoop(true);
        walkSound.setVolume(50.f);
    }

    Camera(const Camera& camera) : Circle2D(camera), W_world(camera.W_world) // copy constructor
    {
        d_height = camera.d_height;
        v_points2D = camera.v_points2D;
        p_position = camera.p_position;
        v_distances = camera.v_distances;
        allCollisions = camera.allCollisions;
        d_direction = camera.d_direction;
        d_depth = camera.d_depth;
        d_fieldOfView = camera.d_fieldOfView;
        d_eyesHeight = camera.d_eyesHeight;
        d_walkSpeed = camera.d_walkSpeed;
        d_viewSpeed = camera.d_viewSpeed;
        i_health = camera.i_health;
        b_collision = camera.b_collision;
        b_textures = camera.b_textures;
        b_smooth = camera.b_smooth;
        localMousePosition = camera.localMousePosition;
        v_weapons = camera.v_weapons;
        i_selectedWeapon = camera.i_selectedWeapon;
        walkSound = camera.walkSound;
        setName(camera.getName());
    }

    bool cross(const std::pair<Point2D, Point2D>& ray, std::pair<Point2D, Point2D>& wall, Point2D& point, double& uv) override
    {
        return Object2D::cross(ray, wall, point, uv);
    }

    ClientUDP* client = nullptr;

    void updateDistances(const World& world);
    void drawCameraView(sf::RenderTarget& window);

    void draw(sf::RenderTarget& window) override;

    bool keyboardControl(double elapsedTime, sf::RenderWindow& window);

    void shiftPrecise(Point2D vector);

    bool isSmooth() { return b_smooth; }
    void switchSmooth() { b_smooth = !b_smooth; }
    bool isCollision() { return b_collision; }
    void switchCollision() { b_collision = !b_collision; }
    bool isTextures() { return b_textures; }
    void switchTextures() { b_textures = !b_textures; }

    void previousWeapon();
    void nextWeapon();

    double health() const { return i_health; }
    bool reduceHealth(double damage = 0);
    void fullHealth () { i_health = 100; }
    void setHealth(double h) {i_health = h; }

    int type() override { return 1; }

    void addCamera(std::string name, Camera& camera);
    void removeCamera(std::string name);
};


#endif //PSEUDO3DENGINE_CAMERA_H
