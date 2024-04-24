#ifndef CAMERA_H
#define CAMERA_H

#include "ray.h"
#include "general.h"
#include "base.h"

/**
 * @class Camera
 * 
 * @param[in]       lookfrom Camera position
 * @param[in]       lookat point towards coordinate
 * @param[in]       vup the vector representing the normal to the camera plane. Can be specified to rotate the camera.
 * @param[in]       vfov Field of view
 * @param[in]       aspect_ratio float of width/height
 * @param[in]       focus_dist Distance from lookfrom in direction towards lookat to focus on. e.g 10.0 means the distance of highest sharpness is 10 units in direction of pointing.
 * @param[in]       _time0 DEPRECATED, shutter open
 * @param[in]       _time1 DEPRECATED, shutter close
 * 
 * @note Shutter open and shutter close are defaulted to 0 and 1, and have no effect on anything.
 */
class Camera : Base {
    public:
        Camera(
            point3 lookfrom,
            point3 lookat,
            vec3   vup,
            double vfov, 
            double aspect_ratio,
            double aperture,
            double focus_dist,
            double _time0 = 0,
            double _time1 = 1) : Base(lookfrom) {
            
            // Convert vertical field of view from degrees to radians
            auto theta = degrees_to_radians(vfov);
            // Calculate the height of the viewport
            auto h = tan(theta/2);
            auto viewport_height = 2.0 * h;
            // Calculate the width of the viewport
            auto viewport_width = aspect_ratio * viewport_height;
            
            // Calculate the camera's orthonormal basis vectors for orientation
            w = (position - lookat).unit_vector();  // Viewing direction vector (reverse)
            u = (cross(vup, w)).unit_vector();      // Right-side vector of the camera
            v = cross(w, u);                        // Actual "up" vector for the camera

            // Calculate the vectors representing the viewport dimensions
            horizontal = focus_dist * viewport_width * u;  // Horizontal vector scaled by focus distance
            vertical = focus_dist * viewport_height * v;   // Vertical vector scaled by focus distance
            // Calculate the lower-left corner of the viewport
            lower_left_corner = position - horizontal/2 - vertical/2 - focus_dist * w;

            // Calculate the radius of the lens for depth of field effects
            lens_radius = aperture / 2;
            // Set the motion blur time interval
            time0 = _time0;
            time1 = _time1;
        }

        ray get_ray(double s, double t) const {
            vec3 rd = lens_radius * random_in_unit_disk();
            vec3 offset = u * rd.x() + v * rd.y();


            return ray(position + offset, 
                lower_left_corner + s*horizontal + t*vertical - position - offset,
                random_double(time0,time1));
        }

    private:
        point3 lower_left_corner;
        vec3 horizontal;
        vec3 vertical;
        vec3 u, v, w;
        double lens_radius;
        double time0, time1; // shutter open -> shutter close
};

#endif