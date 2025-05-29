//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"

inline std::normal_distribution<> stdNormal;

enum MaterialType { 
    DIFFUSE,
    DIFFUSE_NORMAL_DISTRIBUTION,
    TorranceSparrow
};

class Material{
private:

    // Compute reflection direction
    Vector3f reflect(const Vector3f &I, const Vector3f &N) const
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
    Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior) const
    {
        // ���ڻ����Ľ�����ֵ���෴��, ����������������⵽�ڻ������ڵ��������
        // ������ni��nt�ֱ���ʲôֵ, �������ָ��ƽ���
        float cosi = clamp(-1, 1, dotProduct(I, N));
        // etai����ǰ�����ܶ�, etat���������ܶ�
        float etai = 1, etat = ior;
        Vector3f n = N;
        // < 0 ����, > 0 ����
        if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
        float eta = etai / etat;
        // k������ķ����뷨�����нǵ�sinֵ
        float k = 1 - eta * eta * (1 - cosi * cosi);
        // k < 0ȫ����, ��ôͨ��sinֵ�����Ľ�?
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
    void fresnel(const Vector3f &I, const Vector3f &N, const float &ior, float &kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0) {  std::swap(etai, etat); }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1) {
            kr = 1;
        }
        else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            // ����ƫ�����ķ�����������ǵı仯���仯
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }

    // ����������ϵ��a������ת����ȫ��, TBN�仯, ���ߺ͸�����
    Vector3f toWorld(const Vector3f &a, const Vector3f &N){
        Vector3f B, C;
        if (std::fabs(N.x) > std::fabs(N.y)){
            float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
            C = Vector3f(N.z * invLen, 0.0f, -N.x *invLen);
        }
        else {
            float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
            C = Vector3f(0.0f, N.z * invLen, -N.y *invLen);
        }
        B = crossProduct(C, N);
        return a.x * B + a.y * C + a.z * N;
    }

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior;  // "�ܶ�"
    double alpha;
    Vector3f Kd, Ks;
    float specularExponent;
    //Texture tex;

    inline Material(MaterialType t=DIFFUSE, Vector3f e=Vector3f(0,0,0));
    inline MaterialType getType();
    //inline Vector3f getColor();
    inline Vector3f getColorAt(double u, double v);
    inline Vector3f getEmission();
    inline bool hasEmission();

    // sample a ray by Material properties
    inline Vector3f sample(const Vector3f &wi, const Vector3f &N);
    // given a ray, calculate the PdF of this ray
    inline float pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N);
    // given a ray, calculate the contribution of this ray
    inline Vector3f eval(const Vector3f &wi, const Vector3f &wo, const Vector3f &N);

};

Material::Material(MaterialType t, Vector3f e){
    m_type = t;
    //m_color = c;
    m_emission = e;
}

MaterialType Material::getType(){return m_type;}
///Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() {return m_emission;}
bool Material::hasEmission() {
    // EPSILON ��С�ĳ���
    if (m_emission.norm() > EPSILON) return true;
    else return false;
}

// not implement
Vector3f Material::getColorAt(double u, double v) {
    return Vector3f();
}


Vector3f Material::sample(const Vector3f &wi, const Vector3f &N){
    switch(m_type){
        case DIFFUSE:
        {
            // uniform sample on the hemisphere
            // ������䷽���sample���õ��Ƿ������ı�������
            // ��localRay x,yֵ����, �Ƶ���ԭ��
            // x_1 ����zֵ, z��-1��1֮��
            // r * r = 1 - z * z, ��֤x * x + y * y = r * r. ���ó���localRay�ǵ�λ����
            // phi��2 * M_PI��֤x, y�����һ�������-1��1�仯
            float x_1 = get_random_float(), x_2 = get_random_float();
            float z = std::fabs(1.0f - 2.0f * x_1);
            float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
            Vector3f localRay(r*std::cos(phi), r*std::sin(phi), z);
            return toWorld(localRay, N);
            
            break;
        }
    }
}

float Material::pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N){
    switch(m_type){
        case DIFFUSE:
        {
            // ���Ȳ���, pdf�Ͱ���������й�
            // uniform sample probability 1 / (2 * PI)
            if (dotProduct(wo, N) > 0.0f)
                return 0.5f / M_PI;
            else
                return 0.0f;
            break;
        }
        
    }
}

Vector3f Material::eval(const Vector3f &wi, const Vector3f &wo, const Vector3f &N){
    switch(m_type){
        case DIFFUSE:
        {
            // �����prdf�Ǽ���������������������, Kd�Ƿ�����
            // ������治��������, Ĭ��prdfΪ 1 / Pi
            // calculate the contribution of diffuse model
            // ���ﱣ֤�˳����ͷ�������ͬ��ʱ, radianceΪ0
            float cosalpha = dotProduct(N, wo);
            if (cosalpha > 0.0f) {
                Vector3f diffuse = Kd / M_PI;
                return diffuse;
            }
            else
                return Vector3f(0.0f);
            break;
        }
        case DIFFUSE_NORMAL_DISTRIBUTION: 
        {
            Vector3f h_normal = normalize(-wi + wo);  // �������
            float kr, kt;  // �����ʺ�������
            fresnel(-wo, h_normal, ior, kr);  //�ӵ�1, wo�Ǵ�ײ���㵽��Դ; ����������Ӧ��ʹ��-wo, �ӹ�Դ��ײ����
            // ��̬�ֲ���Ϊ΢����ķ��߷ֲ�

            kt = 1 - kr;
            /*
                ��ͨ����̬�ֲ��Ƕ�ά�ģ��������������λ�ġ�
                �ҵ���������theta���Ǻ�phi��λ�Ǳ�ʾ���������Ծ���Ӱ��
                thetaȡ��������̫�ֲ���ȡֵ��Χ0-pi����phi���Ͼ��ȷֲ���ȡֵ��Χ0-2pi��
                �������ÿ������ȡ�Ŀ�����Ϊ f(x) / 2pi. f(x)Ϊ��̬�ֲ�
            */
            // https://zhuanlan.zhihu.com/p/434964126΢����
            double theta, pdf;
            theta = std::acos(dotProduct(N, h_normal));
            // map theta to x which belong to normol distribution
            const float normal_distribution_x_len = 1.;
            double x = theta / (0.5 * M_PI) * normal_distribution_x_len;
            //pdf = standardNormalDistribution(x) / (2 * M_PI); ���Բ��ó���2PI��,��λ����ֻ�ö�ά����̫�ֲ��ͺ���
            pdf = standardNormalDistribution(x);
            
            return 2.5 * kr * pdf / (4 * dotProduct(N, -wi) * dotProduct(N, wo)) * Ks + 
                kt * Kd / M_PI;
        }
        case TorranceSparrow:
            float cosalpha = dotProduct(N, wo);
            if (cosalpha > 0.0f) {
                Vector3f h_normal = normalize(-wi + wo);
                float kr, kt;
                fresnel(-wo, h_normal, ior, kr);
                kt = 1 - kr;

                double ndf = NDF(alpha, h_normal, N);
                double g = G(alpha, N, wi, wo);

                Vector3f res = kr * ndf * g / (4 * dotProduct(N, -wi) * dotProduct(N, wo)) * Ks +
                    kt * Kd / M_PI;
                return res;
            }
            else
                return Vector3f(0.0f);
            break;
    }
}

#endif //RAYTRACING_MATERIAL_H
