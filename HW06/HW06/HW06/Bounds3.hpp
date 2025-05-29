//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_BOUNDS3_H
#define RAYTRACING_BOUNDS3_H
#include "Ray.hpp"
#include "Vector.hpp"
#include <limits>
#include <array>
#include <cmath>

class Bounds3
{
  public:
    Vector3f pMin, pMax; // two points to specify the bounding box
    Bounds3()
    {
        constexpr double minNum = std::numeric_limits<double>::lowest();
        constexpr double maxNum = std::numeric_limits<double>::max();
        pMax = Vector3f(minNum, minNum, minNum);
        pMin = Vector3f(maxNum, maxNum, maxNum);
    }
    Bounds3(const Vector3f p) : pMin(p), pMax(p) {}
    Bounds3(const Vector3f p1, const Vector3f p2)
    {
        pMin = Vector3f(fmin(p1.x, p2.x), fmin(p1.y, p2.y), fmin(p1.z, p2.z));
        pMax = Vector3f(fmax(p1.x, p2.x), fmax(p1.y, p2.y), fmax(p1.z, p2.z));
    }

    Vector3f Diagonal() const { return pMax - pMin; }
    int maxExtent() const
    {
        Vector3f d = Diagonal();
        if (d.x > d.y && d.x > d.z)
            return 0;
        else if (d.y > d.z)
            return 1;
        else
            return 2;
    }

    double SurfaceArea() const
    {
        Vector3f d = Diagonal();
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }

    Vector3f Centroid() { return 0.5 * pMin + 0.5 * pMax; }
    Bounds3 Intersect(const Bounds3& b)
    {
        return Bounds3(Vector3f(fmax(pMin.x, b.pMin.x), fmax(pMin.y, b.pMin.y),
                                fmax(pMin.z, b.pMin.z)),
                       Vector3f(fmin(pMax.x, b.pMax.x), fmin(pMax.y, b.pMax.y),
                                fmin(pMax.z, b.pMax.z)));
    }

    Vector3f Offset(const Vector3f& p) const
    {
        Vector3f o = p - pMin;
        if (pMax.x > pMin.x)
            o.x /= pMax.x - pMin.x;
        if (pMax.y > pMin.y)
            o.y /= pMax.y - pMin.y;
        if (pMax.z > pMin.z)
            o.z /= pMax.z - pMin.z;
        return o;
    }

    bool Overlaps(const Bounds3& b1, const Bounds3& b2)
    {
        bool x = (b1.pMax.x >= b2.pMin.x) && (b1.pMin.x <= b2.pMax.x);
        bool y = (b1.pMax.y >= b2.pMin.y) && (b1.pMin.y <= b2.pMax.y);
        bool z = (b1.pMax.z >= b2.pMin.z) && (b1.pMin.z <= b2.pMax.z);
        return (x && y && z);
    }

    bool Inside(const Vector3f& p, const Bounds3& b)
    {
        return (p.x >= b.pMin.x && p.x <= b.pMax.x && p.y >= b.pMin.y &&
                p.y <= b.pMax.y && p.z >= b.pMin.z && p.z <= b.pMax.z);
    }
    inline const Vector3f& operator[](int i) const
    {
        return (i == 0) ? pMin : pMax;
    }

    inline bool IntersectP(const Ray& ray, const Vector3f& invDir,
                           const std::array<float, 3>& dirisNeg) const;
    inline bool IntersectPDiscard(const Ray& ray, const Vector3f& invDir, const std::array<int, 3>& dirIsNeg) const;

    inline bool IntersectArea(const Ray& ray, Vector3f normal_area, Vector3f point_area, float& t) const;
};

inline void swap_n(float& a, float& b) {
    float t = a;
    a = b;
    b = t;
}

inline bool Bounds3::IntersectP(const Ray& ray, const Vector3f& invDir,
    const std::array<float, 3>& dirIsNeg) const {

    float x_out, y_out, z_out, x_i, y_i, z_i;
    x_out = (pMax.x - ray.origin.x)* invDir.x;
    x_i = (pMin.x - ray.origin.x)* invDir.x;

    if (dirIsNeg[0] < 0) swap_n(x_out, x_i);
    else if(dirIsNeg[0] == 0){
        if (pMin.x < ray.origin.x && ray.origin.x < pMax.x) {
            x_out = std::numeric_limits<float>::max();
        }
        else {
            x_out = std::numeric_limits<float>::lowest();
        }
    }

    y_out = (pMax.y - ray.origin.y) * invDir.y;
    y_i = (pMin.y - ray.origin.y) * invDir.y;
    if (dirIsNeg[1] < 0) swap_n(y_out, y_i);
    else if (dirIsNeg[1] == 0){
        if (pMin.y < ray.origin.y && ray.origin.y < pMax.y) {
            y_out = std::numeric_limits<float>::max();
        }
        else {
            y_out = std::numeric_limits<float>::lowest();
        }
    }

    z_out = (pMax.z - ray.origin.z) * invDir.z;
    z_i = (pMin.z - ray.origin.z) * invDir.z;

    if (dirIsNeg[2] < 0) swap_n(z_out, z_i);
	else if (dirIsNeg[2] == 0) {
        if (pMin.z < ray.origin.z && ray.origin.z < pMax.z) {
            z_out = std::numeric_limits<float>::max();
        }
        else {
            z_out = std::numeric_limits<float>::lowest();
        }
    }

    float ti = fmax(fmax(x_i, y_i), z_i);
    float to = fmin(fmin(x_out, y_out), z_out);
    return ti < to && to > 0;
}

inline bool Bounds3::IntersectArea(const Ray& ray, Vector3f normal_area, Vector3f point_area, float& t) const{
    // 使用一个法向量和一个点来定义一个平面
    // (o + td - p) dot n = 0 则在平面上

    if (dotProduct(ray.direction, normal_area) == 0){
        // 光线方向和法向量平行
        return false;
    }

    t = (dotProduct(point_area, normal_area) - dotProduct(ray.origin, normal_area)) / dotProduct(ray.direction, normal_area);
    return true;
}

inline bool Bounds3::IntersectPDiscard(const Ray& ray, const Vector3f& invDir,
    const std::array<int, 3>& dirIsNeg) const
{
    // invDir: ray direction(x,y,z), invDir=(1.0/x,1.0/y,1.0/z), use this because Multiply is faster that Division
    // dirIsNeg: ray direction(x,y,z), dirIsNeg=[int(x>0),int(y>0),int(z>0)], use this to simplify your logic
    // TODO test if ray bound intersects
    
    Vector3f n_zOy(1, 0, 0), n_xOy(0, 0, 1), n_xOz(0, 1, 0);

    // intersect 6 pairs area
    float t01_zOy, t02_zOy, t01_xOy, t02_xOy, t01_xOz, t02_xOz;
    // 区分入射和出射t
    float ti_zOy=0, to_zOy=0, ti_xOy=0, to_xOy=0, ti_xOz=0, to_xOz=0;

    // 如果它与平面平行， 要做特殊处理
    if (!IntersectArea(ray, n_zOy, pMin, t01_zOy)){
        if (pMin.x < ray.origin.x && ray.origin.x < pMax.x){
            // 初始光线就在空间内, 离开时间为无限(不离开) 
            ti_zOy = std::numeric_limits<float>::lowest();
            to_zOy = std::numeric_limits<float>::max();
        }
        else{
            // 光线永远和空间不相交 
            to_zOy = std::numeric_limits<float>::lowest();
            ti_zOy = std::numeric_limits<float>::lowest();
        }
    };
    IntersectArea(ray, n_zOy, pMax, t02_zOy);

    if(!IntersectArea(ray, n_xOy, pMin, t01_xOy)){
        if (pMin.z < ray.origin.z && ray.origin.z < pMax.z){
            // 初始光线就在空间内, 离开时间为无限(不离开) 
            ti_xOy = std::numeric_limits<float>::lowest();
            to_xOy = std::numeric_limits<float>::max();
        }
        else{
            // 光线永远和空间不相交 
            to_xOy = std::numeric_limits<float>::lowest();
            ti_xOy = std::numeric_limits<float>::lowest();
        }
    };
    IntersectArea(ray, n_xOy, pMax, t02_xOy);

    if (!IntersectArea(ray, n_xOz, pMin, t01_xOz)){
        if (pMin.y < ray.origin.y && ray.origin.y < pMax.y){
            // 初始光线就在空间内, 离开时间为无限(不离开) 
            ti_xOz = std::numeric_limits<float>::lowest();
            to_xOz = std::numeric_limits<float>::max();
        }
        else{
            // 光线永远和空间不相交 
            to_xOz = std::numeric_limits<float>::lowest();
            ti_xOz = std::numeric_limits<float>::lowest();
        }
    };
    IntersectArea(ray, n_xOz, pMax, t02_xOz);

    // 上面的t01和t02只是两个交点, 谁是入射和出射还要算(比较大的为出射)
    if(t01_xOy > t02_xOy){
        to_xOy = t01_xOy;
        ti_xOy = t02_xOy;
    }
    if(t01_xOz > t02_xOz){
        to_xOz = t01_xOz;
        ti_xOz = t02_xOz;
    }
    if(t01_zOy > t02_zOy){
        to_zOy = t01_zOy;
        ti_zOy = t02_zOy;
    }

    float ti = fmax(ti_xOy, fmax(ti_xOz, ti_zOy));
    float to = fmin(to_xOy, fmax(to_xOz, to_zOy));

    // 出射点小于0表示, 光线不可能打入到包围盒中
    if (to <= 0){
        return false;
    }
    return true;
}

inline Bounds3 Union(const Bounds3& b1, const Bounds3& b2)
{
    Bounds3 ret;
    ret.pMin = Vector3f::Min(b1.pMin, b2.pMin);
    ret.pMax = Vector3f::Max(b1.pMax, b2.pMax);
    return ret;
}

inline Bounds3 Union(const Bounds3& b, const Vector3f& p)
{
    Bounds3 ret;
    ret.pMin = Vector3f::Min(b.pMin, p);
    ret.pMax = Vector3f::Max(b.pMax, p);
    return ret;
}

#endif // RAYTRACING_BOUNDS3_H
