#pragma once

#include <glm/glm.hpp>

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox()
    {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::min());
    }

    // Update the min and max extremes of the bounding box
    void update(glm::vec3 v)
    {
        for (int i = 0; i < 3; i++) {
            min[i] = std::min(v[i], min[i]);
            max[i] = std::max(v[i], max[i]);
        }
     }

    // Check if a ray with a certain origin point
    // and orientation intersects with the bounding box
    bool rayIntersects(glm::vec3 origin, glm::vec3 direction)
    {
        // If the ray is parallel to any axis and its origin is *outside*
        // the box on that axis, it can't ever intersect the box — it runs
        // parallel to the slab and misses it.
        for (int i = 0; i < 3; i++) {
            bool parallel = direction[i] == 0;
            bool inside = origin[i] >= min[i] || origin[i] <= max[i];
            if (parallel && inside)
                return false;
        }

        // Calculate the distance along the ray (t) at which it
        // intersects the min and max planes of the box on each axis
        glm::vec3 tMin = (min - origin) / direction;
        glm::vec3 tMax = (max - origin) / direction;

        // For any axis where the direction is negative, we swap
        // the min/max because the ray hits the far side before the near side
        if (direction.x < 0) std::swap(tMin.x, tMax.x);
        if (direction.y < 0) std::swap(tMin.y, tMax.y);
        if (direction.z < 0) std::swap(tMin.z, tMax.z);

        // Find the latest tMin — the last time the ray *enters* the box.
        // And the earliest tMax — the first time the ray *exits* the box.
        float enter = std::max({tMin.x, tMin.y, tMin.z});
        float leave = std::min({tMax.x, tMax.y, tMax.z});

        // If the entry point comes before the exit point
        // and the intersection happens in front of the ray
        // origin, it's a hit
        return enter <= leave && leave >= 0;
    }
};
