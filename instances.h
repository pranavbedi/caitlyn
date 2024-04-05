#include "ray.h"
#include "primitive.h"

class instance_object : public Geometry {
  public:
  // Takes in the geom object p, the transform matrix pointer, length for the length of transform to access elements
    instance(shared_ptr<Geometry> p, float *transform, int length)
      : object(p), transform{transform}, length{length}

  private:
    shared_ptr<Geometry> object;
    float *transform;
    int length;
};