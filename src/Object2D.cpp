//
// Created by ivan- on 18.01.2020.
//

#include "Object2D.h"
#include "settings.h"

int sign(double number) { return number >= 0 ? 1 : -1; }

double cross_product(Point2D p1, Point2D p2)
{
    return p1.x*p2.y - p1.y*p2.x;
}

bool Object2D::segments_crossing(std::pair<Point2D, Point2D> segment1, std::pair<Point2D, Point2D> segment2, Point2D& point)
{
    // {v11 = segment1.first, v12 = segment1.second}, {v21 = segment2.first, v22 = segment2.second}
    Point2D cut1 = segment1.second - segment1.first;
    Point2D cut2 = segment2.second - segment2.first;
    double prod1;
    double prod2;

    prod1 = cross_product(cut1, (segment2.first - segment1.first));
    prod2 = cross_product(cut1, (segment2.second - segment1.first));

    if (sign(prod1) == sign(prod2) || (prod1 == 0) || (prod2 == 0)) // Отсекаем также и пограничные случаи
        return false;

    prod1 = cross_product(cut2, (segment1.first - segment2.first));
    prod2 = cross_product(cut2, (segment1.second - segment2.first));

    if (sign(prod1) == sign(prod2) || (prod1 == 0) || (prod2 == 0)) // Отсекаем также и пограничные случаи
        return false;

    point.x = segment1.first.x + cut1.x * std::abs(prod1) / std::abs(prod2 - prod1);
    point.y = segment1.first.y + cut1.y * std::abs(prod1) / std::abs(prod2 - prod1);

    return true;
}

bool Object2D::cross(const std::pair<Point2D, Point2D>& ray, std::pair<Point2D, Point2D>& wall, Point2D& point, double& uv)
{
    Point2D crossPoint = { 0, 0 };
    std::pair<Point2D, Point2D> segment2 = { p_position + v_points2D.back(), p_position + v_points2D.front() };
    bool success = false;
    for (size_t k = 0; k < v_points2D.size() - 1; k++)
    {
        if (segments_crossing(ray, segment2, crossPoint) && (point - ray.first).abs() > (crossPoint - ray.first).abs())
        {
            success = true;
            point = crossPoint;
            wall = std::move(segment2);
        }
        segment2 = { p_position + v_points2D[k], p_position + v_points2D[k + 1] };
    }
    if (segments_crossing(ray, segment2, crossPoint) && (point - ray.first).abs() > (crossPoint - ray.first).abs())
    {
        success = true;
        point = crossPoint;
        wall = std::move(segment2);
    }
    if (success)
    {
        uv = (wall.second - point).abs();
    }
    return success;
}

void Object2D::draw(sf::RenderTarget& window) {
    sf::ConvexShape polygon;
    polygon.setPointCount(nodes().size());
    int i = 0;
    for (auto p : nodes())
        polygon.setPoint(i++, sf::Vector2f((float)p.x * SCALE, (float)p.y * SCALE));
    polygon.setOutlineColor(OUTLINE_COLOR);
    polygon.setFillColor(FILL_COLOR);
    polygon.setOutlineThickness(OUTLINE_THICKNESS);
    polygon.setPosition((float)x() * SCALE, (float)y() * SCALE);
    window.draw(polygon);
}
