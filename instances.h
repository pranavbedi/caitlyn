#include "ray.h"
#include "primitive.h"

class translate : public Primitive {
  public:
    translate(shared_ptr<Primitive> p, const vec3& displacement)
      : object(p), offset(displacement)
    {   
        auto pos = object->position + offset;
        p->position = pos;
    }

    bool hit(const ray& r, interval ray_t, HitInfo& rec) {
        // Move the ray backwards by the offset
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        // Determine where (if any) an intersection occurs along the offset ray
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // Move the intersection point forwards by the offset
        rec.p += offset;

        return true;
    }


  private:
    shared_ptr<Primitive> object;
    vec3 offset;

};