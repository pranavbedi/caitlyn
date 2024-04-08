#ifndef INSTANCES_H
#define INSTANCES_H

#include "ray.h"
#include "primitive.h"

class instance_object : public Geometry {
  public:

  instance_object(std::shared_ptr<Geometry> p, float* transform)
    : Geometry(vec3(p->position)), object(p), transform(transform) {}

    shared_ptr<Geometry> object;
    float *transform;
};


class Instance {
  public:
  float* transform; // assumes float[12]
  RTCScene instance_scene;

  Instance(float* transform) : transform{transform} {}
};

class PrimitiveInstance : public Instance {
  public:
  std::shared_ptr<Primitive> pptr;

  PrimitiveInstance(float* transform) : Instance(transform) {}
};

class SpherePrimitiveInstance : public PrimitiveInstance {
  public:

  SpherePrimitiveInstance(std::shared_ptr<SpherePrimitive> sprim, float* transform, RTCDevice device)
    : PrimitiveInstance(transform) {
    instance_scene = rtcNewScene(device);
    unsigned int geomID = rtcAttachGeometry(instance_scene, sprim->geom);
    rtcReleaseGeometry(sprim->geom);
    rtcCommitScene(instance_scene);

    vec3 translate = vec3(transform[3], transform[7], transform[11]);
    pptr = make_shared<SpherePrimitive>(sprim->position + translate, sprim->mat_ptr, 1, device);
  }
};

#endif