//
// Created by ivan- on 18.01.2020.
//

#include "Camera.h"
#include <cmath>
#include <windows.h>
#include "ClientUDP.h"

using namespace std;

double Camera::angleInited = 0;
double Camera::directionSin = 0;
double Camera::directionCos = 0;
double Camera::horizontalCos[DISTANCES_SEGMENTS];
double Camera::horizontalSin[DISTANCES_SEGMENTS];
double Camera::verticalTan[SCREEN_HEIGHT];

// Just increase distance for all collisions in v_RayCastStructure
void Camera::recursiveIncreaseDistance(std::vector<RayCastStructure>& v_RayCastStructure, double distance)
{
    for(auto& rcs : v_RayCastStructure)
    {
        rcs.distance += distance;
        recursiveIncreaseDistance(rcs.v_mirrorRayCast, distance);
    }
}

// Add player to the list of existing players
void Camera::addCamera(std::string name, Camera& camera)
{
    m_playersOnTheScreen.insert({ name, camera });
}

// Remove player from the list of existing players
void Camera::removeCamera(std::string name)
{
    m_playersOnTheScreen.erase(name);
}

// Find all objects hitted by ray, except object with given name.
// Repeats raycasting if mirror hitted and reflections < 40
void Camera::objectsRayCrossed(const pair<Point2D, Point2D>& ray, std::vector<RayCastStructure>& v_rayCastStruct, const std::string& name, int reflections)
{
    pair<Point2D, Point2D> edge;
    Point2D nearCross;
    double len = 0;
    double closest = d_depth;
    for (auto object : W_world.objects())
    {
        if (object.first == name || object.second.nodes().size() < 2)
            continue;

        // Check crossing

        Point2D crossPoint = ray.second;
        std::vector<RayCastStructure> v_reflectedRayCastStructure;

        pair<Point2D, Point2D> wall;
        // If ray crossed with object
        if (object.second.cross(ray, wall, crossPoint, len))
        {
            // If it was mirror
            if (object.second.isMirror() && scalarWithNormal(wall.second - wall.first, ray.second - ray.first) < 0 && reflections < 40)
            {
                Point2D edgeVector = wall.second - wall.first;
                Point2D rayVector = ray.second - ray.first;
                double twistAngle = 2 * acos((edgeVector.x * rayVector.x + edgeVector.y * rayVector.y) / (edgeVector.abs() * rayVector.abs()));
                Point2D twistedRayVector = { rayVector.x * cos(twistAngle) + rayVector.y * sin(twistAngle), -rayVector.x * sin(twistAngle) + rayVector.y * cos(twistAngle) };
                pair<Point2D, Point2D> newRay = { crossPoint, crossPoint + twistedRayVector };

                objectsRayCrossed(newRay, v_reflectedRayCastStructure, object.first, reflections + 1);
                recursiveIncreaseDistance(v_reflectedRayCastStructure, (ray.first - crossPoint).abs());
            }
            v_rayCastStruct.push_back({ (ray.first - crossPoint).abs(), len, &object.second, object.second.height(), v_reflectedRayCastStructure });
            // for collision
            double dist = (crossPoint - position()).abs();
            if (dist < closest)
            {
                edge = std::move(wall);
                closest = dist;
                nearCross = crossPoint;
            }
        }
    }
    // collision
    if (b_collision && name == getName() && COLLISION_AREA >= closest)
    {
        CollisionInformation newCollision;
        newCollision.distance = (nearCross - position()).abs();
        newCollision.edge = std::move(edge);
        newCollision.collisionPoint = nearCross;
        allCollisions.push_back(newCollision);
    }
    // Sort hits by descending of distance
    std::sort(v_rayCastStruct.begin(), v_rayCastStruct.end(), [](const RayCastStructure& lh, const RayCastStructure& rh) { return lh.distance > rh.distance; });
}

// Find objects crossed by ray near enough for collisions.
// Must be use only on invisible segments.
void Camera::hiddenObjectsRayCrossed(const pair<Point2D, Point2D>& ray, const std::string& name)
{
    Object2D* obj = nullptr;
    std::pair<Point2D, Point2D> edge;
    double len = 0;
    Point2D nearCross = ray.second;
    for (auto object : W_world.objects())
    {
        if (object.first == name || object.second.nodes().size() < 2)
            continue;

        // Check collision

        Point2D crossPoint = ray.second;

        // If object hitted and near closer than already finded - rember it
        pair<Point2D, Point2D> wall;
        if (object.second.cross(ray, wall, crossPoint, len) && (nearCross - ray.first).abs() > (crossPoint - ray.first).abs())
        {
            nearCross = std::move(crossPoint);
            obj = &object.second;
            edge = std::move(wall);
        }
    }
    // If something was hitted close enough - write it
    if (obj)
    {
        CollisionInformation newCollision;
        newCollision.distance = (nearCross - position()).abs();
        newCollision.edge = std::move(edge);
        newCollision.collisionPoint = nearCross;
        allCollisions.push_back(newCollision);
    }
}

// Check all directions for collisions and walls
void Camera::updateDistances(const World& world)
{
    v_distances.clear();
    allCollisions.clear();

    Point2D forward = { cos(d_direction), sin(d_direction) };
    Point2D left = { -forward.y, forward.x };
    double halfWidth = tan(d_fieldOfView / 2) * ((double)SCREEN_WIDTH / SCREEN_HEIGHT);

    int i;
    int segs = 2 * PI / d_fieldOfView * DISTANCES_SEGMENTS;

    // Visible for player segments
    for (i = 0; i < DISTANCES_SEGMENTS; i++) // Need top-down map? Set limit to segs. No need? DISTANCE_SEGMENTS.
    {
        pair<Point2D, Point2D> segment1;

        double offset = ((i * 2.0 / (DISTANCES_SEGMENTS - 1.0)) - 1.0) * halfWidth;
        segment1 = { {x(), y()}, {x() + d_depth * (forward.x + offset * left.x), y() + d_depth * (forward.y + offset * left.y)} };

        std::vector<RayCastStructure> v_rayCastStructure;

        objectsRayCrossed(segment1, v_rayCastStructure, getName());

        if (!v_rayCastStructure.empty())
            v_distances.push_back(v_rayCastStructure);
        else
            v_distances.push_back({ {d_depth, 0, nullptr, 1} });
    }

    // Invisible for player segments
    if (!b_collision)
        return;

    for (; i < segs; i++)
    {
        pair<Point2D, Point2D> segment1;
        
        double direction = d_direction + ((double)i / DISTANCES_SEGMENTS - 0.5) * d_fieldOfView;
        segment1 = { {x(), y()}, {x() + COLLISION_AREA * cos(direction), y() + COLLISION_AREA * sin(direction)} };

        hiddenObjectsRayCrossed(segment1, getName());
    }
}

// Draw camera on 2d map
void Camera::draw(sf::RenderTarget& window)
{
    if (v_distances.empty() || i_health <= 0)
        return;

    sf::CircleShape circle;
    circle.setRadius(COLLISION_DISTANCE * SCALE);
    circle.setOutlineColor(OUTLINE_CAMERA_COLOR);
    circle.setFillColor(FILL_CAMERA_COLOR);
    circle.setOutlineThickness(OUTLINE_CAMERA_THICKNESS);
    circle.setPosition((float)x() * SCALE - COLLISION_DISTANCE * SCALE, (float)y() * SCALE - COLLISION_DISTANCE * SCALE);

    double left = d_direction - d_fieldOfView / 2;
    double right = d_direction + d_fieldOfView / 2;

    sf::ConvexShape triangle;
    triangle.setPointCount(CONVEX_NUMBER + 2);
    triangle.setPoint(0, sf::Vector2f(0, 0));
    for (int i = 0; i <= CONVEX_NUMBER; i++)
    {
        int index = v_distances[(int)i * DISTANCES_SEGMENTS / CONVEX_NUMBER].size() - 1;
        triangle.setPoint(i + 1, sf::Vector2f(v_distances[(int)i * DISTANCES_SEGMENTS / CONVEX_NUMBER][index].distance * cos(left + (right - left) * i / CONVEX_NUMBER) * SCALE, v_distances[(int)i * DISTANCES_SEGMENTS / CONVEX_NUMBER][index].distance * sin(left + (right - left) * i / CONVEX_NUMBER) * SCALE));
    }
    triangle.setOutlineColor(OUTLINE_CAMERA_COLOR);
    triangle.setFillColor(FILED_OF_VEW_COLOR);
    triangle.setOutlineThickness(OUTLINE_CAMERA_THICKNESS);
    triangle.setPosition((float)x() * SCALE, (float)y() * SCALE);

    window.draw(triangle);
    window.draw(circle);
}

// Returns player if ray hits him
std::pair<Object2D*, double> Camera::cameraRayCheck(RayCastStructure& structure) {
    std::pair<Object2D*, double> result = { nullptr, 1 };
    // If hit player
    if (structure.object->type() == 1)
    {
        result.first = structure.object;
        result.second = structure.distance;
    }
    // If hit mirror
    else if (!structure.v_mirrorRayCast.empty())
    {
        return cameraRayCheck(structure.v_mirrorRayCast[structure.v_mirrorRayCast.size() - 1]);
    }
    return result;
}

// Fire at the player
void Camera::fire()
{
    pair<Point2D, Point2D> segment1 = { {x(), y()}, {x() + d_depth * cos(d_direction), y() + d_depth * sin(d_direction)} };
    std::vector<RayCastStructure> v_rayCastStructure;
    objectsRayCrossed(segment1, v_rayCastStructure, getName());

    // if something hitted
    if (!v_rayCastStructure.empty())
    {
        std::pair<Object2D*, double> hitted = cameraRayCheck(v_rayCastStructure[v_rayCastStructure.size() - 1]);
        if (hitted.first)
            client->shoot(hitted.first->getName(), v_weapons[i_selectedWeapon].damage(), hitted.second);
    }
}

// Read keyboard and mouse input
bool Camera::keyboardControl(double elapsedTime, sf::RenderWindow& window)
{
    // Prevents control from another window
    if (!window.hasFocus())
    {
        b_hadFocus = false;
        walkSound.pause();
        return true;
    }

    // Exit to menu
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
    {
        b_hadFocus = false;
        return false;
    }

    double dx = 0;
    double dy = 0;
    double d_sin = sin(d_direction);
    double d_cos = cos(d_direction);
    // Left and right
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
    {
        dx += d_sin;
        dy -= d_cos;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
    {
        dx -= d_sin;
        dy += d_cos;
    }
    // Forward and backward
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
    {
        dx += d_cos;
        dy += d_sin;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
    {
        dx -= d_cos;
        dy -= d_sin;
    }

    // Fire
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
        if (v_weapons[i_selectedWeapon].fire())
            fire();
    }
    // Mouse movement
    if (sf::Mouse::getPosition(window).x != localMousePosition.x) {
        int difference = sf::Mouse::getPosition(window).x - localMousePosition.x;
        sf::Mouse::setPosition({ (int)window.getSize().x / 2, (int)window.getSize().y / 2 }, window);
        localMousePosition = sf::Mouse::getPosition(window);
        // Ignoring first frame after window focus or game start
        if (b_hadFocus)
        {
            d_direction += d_viewSpeed * difference;
        }
    }

    // Start/stop walk sound
    if ((dx * dx + dy * dy) > d_walkSpeed * elapsedTime * d_walkSpeed * elapsedTime / 10)
    {
        if (walkSound.getStatus() != sf::Sound::Status::Playing)
            walkSound.play();
    }
    else
    {
        walkSound.pause();
    }

    // Move player
    shiftPrecise({ dx * d_walkSpeed * elapsedTime * ((double)i_health / 100), dy * d_walkSpeed * elapsedTime * ((double)i_health / 100) });

    // Remember that we had focus and weren't in menu
    b_hadFocus = true;
    return true;
}

// Returns position of wall on screen
pair<double, double> Camera::heightInPixels(double distance, double height)
{
    pair<double, double> h = { 0, 0 };
    double mod = tan(d_fieldOfView / 2) * distance;
    h.first = (1 - (height - d_eyesHeight) / mod) * SCREEN_HEIGHT / 2;
    h.second = (1 + d_eyesHeight / mod) * SCREEN_HEIGHT / 2;
    return h;
}

// Draw 1 pixel column of screen
void Camera::drawVerticalStrip(sf::RenderTarget& window, const RayCastStructure& obj, int shift, int f)
{
    sf::ConvexShape polygon;
    polygon.setPointCount(4);

    pair<double, double> height_now = heightInPixels(obj.distance * horizontalCos[shift], obj.height);

    int h1 = (int)height_now.first;
    int h2 = (int)height_now.second;

    polygon.setPoint(0, sf::Vector2f(0, (float)h1));
    polygon.setPoint(1, sf::Vector2f(0, (float)h2));
    polygon.setPoint(2, sf::Vector2f((float)SCREEN_WIDTH / DISTANCES_SEGMENTS, (float)h2));
    polygon.setPoint(3, sf::Vector2f((float)SCREEN_WIDTH / DISTANCES_SEGMENTS, (float)h1));

    int alpha = (int)(255 * (1 - obj.distance / d_depth));
    if (alpha > 255)
        alpha = 255;
    if (alpha < 0)
        alpha = 0;

    alpha = 255 - alpha;

    if (!b_textures)
        polygon.setFillColor({ 255, 174, 174, static_cast<sf::Uint8>(alpha) });
    else
        polygon.setFillColor({ 255, 255, 255, static_cast<sf::Uint8>(alpha) });

    //polygon.setOutlineThickness(0); // we can make non zero thickness for debug
    polygon.setPosition((float)(shift * SCREEN_WIDTH / DISTANCES_SEGMENTS), 0);

    double scaleFactor = (double)(h2 - h1) / SCREEN_HEIGHT;
    sf::Sprite sprite;
    if (obj.object && b_textures)
    {
        int left = (int)(obj.progress * SCREEN_WIDTH);
        int top = 0;
        if (obj.object->isMirror()) // In case of mirror
        {
            sprite.setTexture(W_world.skyTexture());
            left = (int)(d_direction / 10 * SCREEN_WIDTH);
            top = sprite.getTextureRect().height / 2 - SCREEN_HEIGHT / 2;
        }
        else
        {
            sprite.setTexture(obj.object->loadTexture());
        }
        sprite.setTextureRect(sf::IntRect(left, top, SCREEN_WIDTH / DISTANCES_SEGMENTS, SCREEN_HEIGHT));
        sprite.setPosition(sf::Vector2f((float)shift * SCREEN_WIDTH / DISTANCES_SEGMENTS, (float)h1)); // absolute position
        sprite.scale(1, (float)scaleFactor);
        window.draw(sprite);
    }

    if (abs(obj.distance - d_depth) > 0.001)
        window.draw(polygon);

    // Floor drawing

    int x = shift * SCREEN_WIDTH / DISTANCES_SEGMENTS;

    if (b_smooth || (f == 0) || (x % FLOOR_SEGMENT_SIZE != 0) || !b_textures) return;

    const int scale = 400;
    double baseOffset = 0.5 / horizontalCos[shift];
    double horMod = horizontalCos[shift] * directionCos - horizontalSin[shift] * directionSin;// cos (horizontalAngles[shift] + d_direction) = cos a * cos b - sin a * sin b
    double verMod = horizontalSin[shift] * directionCos + horizontalCos[shift] * directionSin;// sin (horizontalAngles[shift] + d_direction) = sin a * cos b + cos a * sin b

    for (int z = h2; z < SCREEN_HEIGHT; z += FLOOR_SEGMENT_SIZE)
    {
        double offset = baseOffset / verticalTan[z];
        int left = (int)(scale * (position().x + offset * horMod));
        int top = (int)(scale * (position().y + offset * verMod));
        int alpha2 = 255 * 2 * (z - SCREEN_HEIGHT / 2) / SCREEN_HEIGHT;

        sf::Sprite& floor = W_world.floor();

        floor.setTextureRect(sf::IntRect(left, top, FLOOR_SEGMENT_SIZE, FLOOR_SEGMENT_SIZE));
        floor.setPosition(sf::Vector2f((float)x, (float)z)); // absolute position
        floor.setColor({ 255, 255, 255, static_cast<sf::Uint8>(alpha2) });
        window.draw(floor);
    }
}

void Camera::recursiveDrawing(sf::RenderTarget& window, const std::vector<RayCastStructure>& v_RayCastStructure, int shift, int rec)
{
    int i = 0;
    for (const auto& k : v_RayCastStructure)
    {
        if (i + 1 != v_RayCastStructure.size() || (rec != 1))
            drawVerticalStrip(window, k, shift, 0);
        else
            drawVerticalStrip(window, k, shift, 1);
        i++;
        if (!k.v_mirrorRayCast.empty())
            recursiveDrawing(window, k.v_mirrorRayCast, shift, 2);
    }
}

void Camera::drawHealth(sf::RenderTarget& window, int xPos, int yPos, int width, int healthProgress)
{
    sf::ConvexShape polygon1;
    polygon1.setPointCount(4);
    sf::ConvexShape polygon2;
    polygon2.setPointCount(4);

    polygon1.setPoint(0, sf::Vector2f((float)xPos, (float)yPos));
    polygon1.setPoint(1, sf::Vector2f((float)xPos + width, (float)yPos));
    polygon1.setPoint(2, sf::Vector2f((float)xPos + width, (float)yPos + 20));
    polygon1.setPoint(3, sf::Vector2f((float)xPos, (float)yPos + 20));

    polygon2.setPoint(0, sf::Vector2f((float)xPos, (float)yPos));
    polygon2.setPoint(1, sf::Vector2f((float)xPos + width * healthProgress / 100, (float)yPos));
    polygon2.setPoint(2, sf::Vector2f((float)xPos + width * healthProgress / 100, (float)yPos + 20));
    polygon2.setPoint(3, sf::Vector2f((float)xPos, (float)yPos + 20));

    polygon1.setFillColor({ 255, 174, 174, 100 });
    polygon2.setFillColor({ static_cast<sf::Uint8>((100 - healthProgress) * 255), static_cast<sf::Uint8>(healthProgress * 255 / 100), 0, 100 });

    polygon1.setOutlineThickness(3); // we can make non zero thickness for debug
    window.draw(polygon1);
    window.draw(polygon2);
}

void Camera::drawCameraView(sf::RenderTarget& window)
{
    if (v_distances.empty())
        return;

    if (angleInited != d_fieldOfView)
    {
        for (int i = 0; i < DISTANCES_SEGMENTS; i++)
        {
            double halfWidth = tan(d_fieldOfView / 2) * ((double)SCREEN_WIDTH / SCREEN_HEIGHT);
            double offset = ((i * 2.0 / (DISTANCES_SEGMENTS - 1.0)) - 1.0) * halfWidth;
            Point2D dir = { 1, 1 * offset };
            dir = dir.normalize();
            double angle = atan2(dir.y, dir.x);
            horizontalCos[i] = cos(angle);
            horizontalSin[i] = sin(angle);
        }
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            double halfWidth = tan(d_fieldOfView / 2);
            double offset = ((i * 2.0 / (SCREEN_HEIGHT - 1.0)) - 1.0) * halfWidth;
            Point2D dir = { 1, 1 * offset };
            dir = dir.normalize();
            verticalTan[i] = tan(atan2(dir.y, dir.x));
        }
        angleInited = d_fieldOfView;
    }

    directionSin = sin(d_direction);
    directionCos = cos(d_direction);

    // Draw sky and rotate floor
    if (b_textures)
    {
        sf::Sprite sprite_sky;
        sprite_sky.setTexture(W_world.skyTexture());
        sprite_sky.setTextureRect(sf::IntRect((int)(d_direction * SCREEN_WIDTH / 2), sprite_sky.getTextureRect().height / 2 - SCREEN_HEIGHT / 2, SCREEN_WIDTH, 1080));
        sprite_sky.setPosition(sf::Vector2f(0, 0)); // absolute position
        window.draw(sprite_sky);
        W_world.floor().setRotation(-d_direction / PI * 180 - 90);

        if (b_smooth)
        {
            sf::Image floorPrerendered;
            floorPrerendered.create(SCREEN_WIDTH, SCREEN_HEIGHT, sf::Color(0, 0, 0, 0));
            sf::Image floorTexture = W_world.floorTexture().copyToImage();
            const int scale = 400;
            for (int i = 0; i < SCREEN_WIDTH; i++)
            {
                double baseOffset = 0.5 / horizontalCos[i];
                double horMod = horizontalCos[i] * directionCos - horizontalSin[i] * directionSin;
                double verMod = horizontalSin[i] * directionCos + horizontalCos[i] * directionSin;
                for (int j = SCREEN_HEIGHT / 2; j < SCREEN_HEIGHT; j++)
                {
                    double offset = baseOffset / verticalTan[j];
                    int left = (int)(scale * (position().x + offset * horMod)) % 960;
                    int top = (int)(scale * (position().y + offset * verMod)) % 960;
                    left += (left < 0) * 960;
                    top += (top < 0) * 960;
                    int alpha2 = 255 * (j * 2 - SCREEN_HEIGHT) / SCREEN_HEIGHT;
                    sf::Color col = floorTexture.getPixel(left, top);
                    col.a = alpha2;
                    floorPrerendered.setPixel(i, j, col);
                }
            }
            sf::Texture floorText;
            floorText.loadFromImage(floorPrerendered);
            sf::Sprite floorSprite(floorText);
            window.draw(floorSprite);
        }
    }

    for (int i = 0; i < DISTANCES_SEGMENTS; i++)
    {
        recursiveDrawing(window, v_distances[i], i);
    }

    //m_playersOnTheScreen
    for (const auto& player : m_playersOnTheScreen)
    {
        Point2D enemyDirection = (player.second.position() - position()).normalize();
        Point2D cameraLeftDirection = { cos(d_direction - d_fieldOfView / 2), sin(d_direction - d_fieldOfView / 2) };
        double angle = acos(enemyDirection * cameraLeftDirection);
        if ((cameraLeftDirection.x * enemyDirection.y - enemyDirection.x * cameraLeftDirection.y) > 0)
        {
            int xPos = (int)(SCREEN_WIDTH * angle / d_fieldOfView);
            int yPos = (int)(heightInPixels((player.second.position() - position()).abs(), 1).first);
            auto healthProgress = (double)player.second.health();
            drawHealth(window, xPos - 50, yPos, 100, (int)healthProgress);
        }
    }
    drawHealth(window, 50, SCREEN_HEIGHT - 50, 400, (int)health());
    v_weapons[i_selectedWeapon].draw(window);
}

double Camera::scalarWithNormal(Point2D edge, Point2D vector)
{
    Point2D normal = { edge.y, -edge.x };
    normal = normal.normalize();
    double scalar = vector.x * normal.x + vector.y * normal.y;
    return scalar;
}

void Camera::shiftPrecise(Point2D vector)
{
    if (!b_collision)
    {
        shift(vector);
        return;
    }

    for (auto c : allCollisions)
    {
        Point2D edgeVector = c.edge.second - c.edge.first;
        Point2D normal = { edgeVector.y, -edgeVector.x };
        normal = normal.normalize();
        double scalar = vector.x * normal.x + vector.y * normal.y;
        if (scalar < 0 && c.distance - abs(scalar) < COLLISION_DISTANCE)
        {
            vector.x -= normal.x * scalar;
            vector.y -= normal.y * scalar;
        }
    }

    shift(vector);
}

void Camera::previousWeapon()
{
    if (i_selectedWeapon > 0)
        i_selectedWeapon--;
    else
        i_selectedWeapon = v_weapons.size() - 1;
}

void Camera::nextWeapon()
{
    if (i_selectedWeapon < (int)v_weapons.size() - 1)
        i_selectedWeapon++;
    else
        i_selectedWeapon = 0;
}

bool Camera::reduceHealth(double damage)
{
    if (i_health > 100)
        i_health = 100;
    i_health -= damage;
    return i_health <= 0;
}
