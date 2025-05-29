//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"
#include <cassert>

inline std::normal_distribution<> stdNormal;

enum MaterialType {
    DIFFUSE,
    Microface
};

class Material {
private:

    // Compute reflection direction
    Vector3f reflect(const Vector3f& I, const Vector3f& N) const
    {
        return I - 2 * dotProduct(I, N) * N;
    }

    // Compute refraction direction using Snell's law
    //
    // We need to handle with care the two possible situations:
    //
    //    - When the ray is inside the object
    //
    //    - When the ray is outside.
    //
    // If the ray is outside, you need to make cosi positive cosi = -N.I
    //
    // If the ray is inside, you need to invert the refractive indices and negate the normal N
    Vector3f refract(const Vector3f& I, const Vector3f& N, const float& ior) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        Vector3f n = N;
        // < 0 ÈëÉä, > 0 ³öÉä
        if (cosi < 0) { cosi = -cosi; }
        else { std::swap(etai, etat); n = -N; }
        float eta = etai / etat;
        float k = 1 - eta * eta * (1 - cosi * cosi);
        return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
    }

    // Compute Fresnel equation
    //
    // \param I is the incident view direction
    //
    // \param N is the normal at the intersection point
    //
    // \param ior is the material refractive index
    //
    // \param[out] kr is the amount of light reflected
    void fresnel(const Vector3f& I, const Vector3f& N, const float& ior, float& kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0) { std::swap(etai, etat); }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1) {
            kr = 1;
        }
        else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }

    Vector3f toWorld(const Vector3f& a, const Vector3f& N) {
        Vector3f B, C;
        if (std::fabs(N.x) > std::fabs(N.y)) {
            float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
            C = Vector3f(N.z * invLen, 0.0f, -N.x * invLen);
        }
        else {
            float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
            C = Vector3f(0.0f, N.z * invLen, -N.y * invLen);
        }
        B = crossProduct(C, N);
        return a.x * B + a.y * C + a.z * N;
    }

    float __NdfDistributionGGX(Vector3f h, Vector3f n) {
        float alpha = roughness * roughness;
        // I like to expose potential problem so I don't use max function to make hDotn positive.
        float hDotn = dotProduct(h, n);
        assert(hDotn >= 0);
        // n and h is normalize vector
        assert(fabs(n.norm() - 1.0) < 1e-4);
        assert(fabs(h.norm() - 1.0) < 1e-4);

        float alpha2 = alpha * alpha;
        float nom = alpha2;
        
        float denom = M_PI * pow((alpha2 - 1) * hDotn * hDotn + 1, 2);
        // prevent divide by zero when roughness is zero and hDotn is one.
        return nom / std::max(denom, EPSILON);
    }

    float __ShelterCoefficientGGX(Vector3f n, Vector3f v) {
        assert(fabs(n.norm() - 1.0) < 1e-4);
        assert(fabs(v.norm() - 1.0) < 1e-4);
        
        float nDotv = dotProduct(n, v);
        assert(nDotv >= 0.0f);
        float alpha = pow((roughness + 1), 2);
        float k = alpha * 0.125f;

        return nDotv / (nDotv * (1 - k) + k);
    }
    
    float __GCoefficientGGX(Vector3f n, Vector3f l, Vector3f i) {
        return __ShelterCoefficientGGX(n, l) * __ShelterCoefficientGGX(n, i);
    }

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior;
    float roughness;
    Vector3f Kd, Ks;
    float specularExponent;
    //Texture tex;

    inline Material(MaterialType t = DIFFUSE, Vector3f e = Vector3f(0, 0, 0));
    inline MaterialType getType();
    //inline Vector3f getColor();
    inline Vector3f getColorAt(double u, double v);
    inline Vector3f getEmission();
    inline bool hasEmission();

    // sample a ray by Material properties
    inline Vector3f sample(const Vector3f& wi, const Vector3f& N);
    // given a ray, calculate the PdF of this ray
    inline float pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& N);
    // given a ray, calculate the contribution of this ray
    inline Vector3f eval(const Vector3f& wi, const Vector3f& wo, const Vector3f& N);

};

Material::Material(MaterialType t, Vector3f e) {
    m_type = t;
    //m_color = c;
    m_emission = e;
}

MaterialType Material::getType() { return m_type; }
///Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() { return m_emission; }
bool Material::hasEmission() {
    if (m_emission.norm() > EPSILON) return true;
    else return false;
}

// not implement
Vector3f Material::getColorAt(double u, double v) {
    return Vector3f();
}


Vector3f Material::sample(const Vector3f& wi, const Vector3f& N) {
    switch (m_type) {
    case DIFFUSE:
    {
        float x_1 = get_random_float(), x_2 = get_random_float();
        float z = std::fabs(1.0f - 2.0f * x_1);
        float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
        Vector3f localRay(r * std::cos(phi), r * std::sin(phi), z);
        return toWorld(localRay, N);

        break;
    }
    case Microface:
    {
        float x_1 = get_random_float(), x_2 = get_random_float();
        float z = std::fabs(1.0f - 2.0f * x_1);
        float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
        Vector3f localRay(r * std::cos(phi), r * std::sin(phi), z);
        return toWorld(localRay, N);

        break;
    }

    }
}

float Material::pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& N) {
    switch (m_type) {
    case DIFFUSE:
    {
        if (dotProduct(wo, N) > 0.0f)
            return 0.5f / M_PI;
        else
            return 0.0f;
        break;
    }
    case Microface:
    {
        if (dotProduct(wo, N) > 0.0f)
            return 0.5f / M_PI;
        else
            return 0.0f;
        break;
    }
    }
}

Vector3f Material::eval(const Vector3f& wi, const Vector3f& wo, const Vector3f& N) {
    switch (m_type) {
    case DIFFUSE:
    {
        float cosalpha = dotProduct(N, wo);
        if (cosalpha > 0.0f) {
            Vector3f diffuse = Kd / M_PI;
            return diffuse;
        }
        else
            return Vector3f(0.0f);
        break;
    }
    case Microface:

        float lDotn = dotProduct(wo, N);
        if (lDotn > 0.) {
            Vector3f h = normalize(-wi+wo);

            float iDotn = dotProduct(-wi, N);
            assert(iDotn >= 0.0);

            float kr, kt;
            fresnel(wi, N, ior, kr);
            kt = 1 - kr;
            float D = __NdfDistributionGGX(h, N);
            float G = __GCoefficientGGX(N, wo, -wi);

            float brdf = kr * D * G / (4 * iDotn * lDotn);
            
            float diffuse = 1. / M_PI;

            //return brdf;
            return Ks * brdf + Kd * kt * diffuse;
        }
        else
            return Vector3f(0.0f);
        break;
    }
}

#endif //RAYTRACING_MATERIAL_H
