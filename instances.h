#include "ray.h"
#include "primitive.h"

class instance_object : public Geometry {
  public:

  instance_object(std::shared_ptr<Geometry> p, float* transform)
    : Geometry(vec3(p->position)), object(p), transform(transform) {}

    shared_ptr<Geometry> object;
    float *transform;
};