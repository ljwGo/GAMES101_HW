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
        // 由于互补的角余弦值是相反数, 利用这个区分是由外到内还是由内到外的折射
        // 以区别ni和nt分别是什么值, 入射光是指向平面的
        float cosi = clamp(-1, 1, dotProduct(I, N));
        // etai折射前介质密度, etat折射后介质密度
        float etai = 1, etat = ior;
        Vector3f n = N;
        // < 0 入射, > 0 出射
        if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
        float eta = etai / etat;
        // k是折射的方向与法向量夹角的sin值
        float k = 1 - eta * eta * (1 - cosi * cosi);
        // k < 0全反射, 怎么通过sin值求它的角?
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
            // 两个偏振方向光的反射率随入射角的变化而变化
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }

    // 将本地坐标系下a的坐标转换到全局, TBN变化, 切线和副切线
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
    float ior;  // "密度"
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
    // EPSILON 很小的常数
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
            // 随机反射方向的sample利用的是法向量的本地坐标
            // 从localRay x,y值出发, 推导出原理
            // x_1 决定z值, z在-1到1之间
            // r * r = 1 - z * z, 保证x * x + y * y = r * r. 最后得出的localRay是单位向量
            // phi中2 * M_PI保证x, y的正弦或余弦在-1到1变化
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
            // 均匀采样, pdf和半球立体角有关
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
            // 这里的prdf是假设的最完美的漫反射情况, Kd是反照率
            // 如果表面不吸收能量, 默认prdf为 1 / Pi
            // calculate the contribution of diffuse model
            // 这里保证了出射光和法向量不同向时, radiance为0
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
            Vector3f h_normal = normalize(-wi + wo);  // 半程向量
            float kr, kt;  // 反射率和折射率
            fresnel(-wo, h_normal, ior, kr);  //坑点1, wo是从撞击点到光源; 所以求反射率应该使用-wo, 从光源到撞击点
            // 正态分布作为微表面的法线分布

            kt = 1 - kr;
            /*
                普通的正态分布是二维的，但半程向量是三位的。
                我的做法是用theta仰角和phi方位角表示向量，忽略距离影响
                theta取法符合正太分布（取值范围0-pi），phi符合均匀分布（取值范围0-2pi）
                半程向量每个方向取的可能性为 f(x) / 2pi. f(x)为正态分布
            */
            // https://zhuanlan.zhihu.com/p/434964126微表面
            double theta, pdf;
            theta = std::acos(dotProduct(N, h_normal));
            // map theta to x which belong to normol distribution
            const float normal_distribution_x_len = 1.;
            double x = theta / (0.5 * M_PI) * normal_distribution_x_len;
            //pdf = standardNormalDistribution(x) / (2 * M_PI); 可以不用除以2PI的,三位方向只用二维的正太分布就好了
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
