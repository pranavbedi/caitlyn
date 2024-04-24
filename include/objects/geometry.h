#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "visual.h"
#include "material.h"
#include "hitinfo.h"
#include <embree4/rtcore.h>

/**
 * @class Geometry
 * @brief Abstract base class for physical objects with materials.
*/
class Geometry : public Visual {
    public:
    Geometry(vec3 position) : Visual(position) {}

    /** @brief given a geomID (referring to ID given by scene attachment), find the material pointer. Usually called by renderer. */
    virtual shared_ptr<material> materialById(unsigned int geomID) const = 0;

    /**
     * @brief Computes and returns HitInfo object for hit information.
     * @return HitInfo A structure containing details about the intersection (e.g., hit point, normal at the hit, material properties).
     */
    virtual HitInfo getHitInfo(const ray& r, const vec3& p, const float t, unsigned int geomID) const = 0;
};

#endif