#ifndef MATERIAL_H
#define MATERIAL_H

#include "general.h"
#include "hitinfo.h"
#include "texture.h"

class hit_record;

class material {

    public:

        virtual color emitted(double u, double v, const point3& p) const;

        virtual bool scatter(const ray& r_in, const HitInfo& rec, color& attenuation, ray& scattered) const = 0;
};

class lambertian : public material {

    public:

        lambertian(const color& a);
        lambertian(shared_ptr<texture> a);

        virtual bool scatter(const ray& r_in, const HitInfo& rec, color& attenuation, ray& scattered) const override;

    private:

        shared_ptr<texture> albedo;
};

class hemispheric : public material {

    public:

        hemispheric(const color& a);

        virtual bool scatter(const ray& r_in, const HitInfo& rec, color& attenuation, ray& scattered) const override;

    public:

        color albedo;
};

class metal : public material {

    public:

        metal(const color& a, double f);

        virtual bool scatter(const ray& r_in, const HitInfo& rec, color& attenuation, ray& scattered) const override;

    public:

        color albedo;
        double fuzz;
};

class dielectric : public material {

    public:

        dielectric(double index_of_refraction);

        virtual bool scatter(const ray& r_in, const HitInfo& rec, color& attenuation, ray& scattered) const override;
        

    public:

        double ir; // Index of Refraction

    private:
    
        // Christophe Schlick's approximation (probability of reflectance)
        // https://en.wikipedia.org/wiki/Schlick%27s_approximation
        static double reflectance(double cosine, double ref_idx);
};

#endif
