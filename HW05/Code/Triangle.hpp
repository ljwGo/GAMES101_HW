#pragma once

#include "Object.hpp"

#include <cstring>

enum class Side{
    Left = 0,
    Right = 1,
    Inline = 2,
};

// 2D的已经不好用了, 会出各种问题. 注意当前方法不能保证点在三角形平面内, 只能保证点在以三角形为底的柱体内
static Side _whichSide3D(Vector3f point, const Vector3f &tailP, const Vector3f &headP, const Vector3f normal){

    Vector3f a = headP - tailP;
    Vector3f b = point - tailP;

    Vector3f a_cross_b(
        a.y * b.z - b.y * a.z,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );

    if (dotProduct(a_cross_b, normal) < 0){
        return Side::Right;
    }
    else if (dotProduct(a_cross_b, normal) > 0){
        return Side::Left;
    }
    else{
        return Side::Inline;
    }
}

static bool insideTriangle(Vector3f vector, const Vector3f* _v, const Vector3f normal)
{   
    Side side01 = _whichSide3D(vector, _v[0], _v[1], normal);
    Side side02 = _whichSide3D(vector, _v[1], _v[2], normal);
    Side side03 = _whichSide3D(vector, _v[2], _v[0], normal);

    if ((int)side01 != (int)side02 || (int)side01 != (int)side03){
        return false;
    }
    else if(side01 == Side::Inline || side02 == Side::Inline || side03 == Side::Inline){
        return false;
    }
    else{
        return true;
    }
}

// 求三位空间中三角形内点的插值, 2D方法就不好用了. 注意下面方法不能识别出不再三角形内的点
static std::tuple<float, float, float> computeBarycentric3D(Vector3f point, const Vector3f* v)
{
    // 这里用了叉乘的方式, 计算3维空间内三角形面积, 特别说明的是, 它没法识别出p点是否在三角形内
    // a cross b = |a||b|sin0

    Vector3f e00 = v[0] - v[1];
    Vector3f e01 = v[1] - v[2];
    Vector3f e02 = v[2] - v[0];

    Vector3f p00 = point - v[0];
    Vector3f p01 = point - v[1];
    Vector3f p02 = point - v[2];

    // 以下代码加入了点是否在三角形内的逻辑, 但是它同样无法判断点是否在三角形的平面内(这里假设它在平面内)
    // normal为三角形面的法向量
    // normal = crossProduct(v[0] - v[1], v[2] - v[1]);
    // Aarea = dotProduct(crossProduct(p01, e01), normal);
    // Barea = dotProduct(crossProduct(p02, e02), normal);
    // Carea = dotProduct(crossProduct(p00, e00), normal);
    // if (Aarea < 0 || Barea < 0 || Carea < 0) 点在三角形外

    float Aarea = norm(crossProduct(e01, p01));
    float Barea = norm(crossProduct(e02, p02));
    float Carea = norm(crossProduct(e00, p00));
    float area = norm(crossProduct(e00, e01));

    return {Aarea / area, Barea / area, Carea / area};
}

bool rayTriangleIntersect(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector3f& orig,
                          const Vector3f& dir, float& tnear, float& u, float& v)
{
    // TODO: Implement this function that tests whether the triangle
    // that's specified bt v0, v1 and v2 intersects with the ray (whose
    // origin is *orig* and direction is *dir*)
    // Also don't forget to update tnear, u and v.

    Vector3f e01 = v0 - v1;
    Vector3f e02 = v2 - v1;
    Vector3f normal = crossProduct(e01, e02);
    normal = normalize(normal);

    // (v1 - intersectPoint) 垂直与normal法向量
    // (o + td - v1) dot normal = 0 --> o dot n + t(d dot n) - v1 dot n = 0 
    // --> t = (v1 dot n - o dot n) / (d dot n)

    // dotProduct(dir, normal) == 0 是一种特殊情况, 光线沿平面平行射入
    if (dotProduct(dir, normal) == 0){
        return false;
    }

    // 获取光线和三角形所在平面的交点
    float t = (dotProduct(v1, normal) - dotProduct(orig, normal)) / dotProduct(dir, normal);
    Vector3f intersectPoint = orig + t * dir;

    // 判断交点是否在三角形内
    const Vector3f* vertexs = new Vector3f[3]{
        v0, v1, v2
    };

    if (insideTriangle(intersectPoint, vertexs, normal) && t > 0){
        tnear = t;

        auto[alpha, beta, gamma] = computeBarycentric3D(intersectPoint, vertexs);
        // u和v不是uv坐标中的u,v; 这里的uv指的是MT算法, 也就是利用重心坐标计算光线是否与三角形相交的算法, u指b1, v指b2
        // 使用的是比较好理解的第一种方法, MT: o + td = (1-u-v)A + uB + vC;
        u = beta;
        v = gamma;

        return true;
    }

    return false;
}

class MeshTriangle : public Object
{
public:
    MeshTriangle(const Vector3f* verts, const uint32_t* vertsIndex, const uint32_t& numTris, const Vector2f* st)
    {
        uint32_t maxIndex = 0;
        for (uint32_t i = 0; i < numTris * 3; ++i)
            if (vertsIndex[i] > maxIndex)
                maxIndex = vertsIndex[i];
        maxIndex += 1;
        // 生成最大索引数的顶点数组
        vertices = std::unique_ptr<Vector3f[]>(new Vector3f[maxIndex]);
        // 顶点数组复制, 不要对c++指针使用sizeof()
        memcpy(vertices.get(), verts, sizeof(Vector3f) * maxIndex);
        vertexIndex = std::unique_ptr<uint32_t[]>(new uint32_t[numTris * 3]);
        memcpy(vertexIndex.get(), vertsIndex, sizeof(uint32_t) * numTris * 3);
        numTriangles = numTris;
        stCoordinates = std::unique_ptr<Vector2f[]>(new Vector2f[maxIndex]);
        memcpy(stCoordinates.get(), st, sizeof(Vector2f) * maxIndex);
    }

    bool intersect(const Vector3f& orig, const Vector3f& dir, float& tnear, uint32_t& index,
                   Vector2f& uv) const override
    {
        bool intersect = false;

        // 光线与所有三角形求交点
        for (uint32_t k = 0; k < numTriangles; ++k)
        {
            const Vector3f& v0 = vertices[vertexIndex[k * 3]];
            const Vector3f& v1 = vertices[vertexIndex[k * 3 + 1]];
            const Vector3f& v2 = vertices[vertexIndex[k * 3 + 2]];
            float t, u, v;
            if (rayTriangleIntersect(v0, v1, v2, orig, dir, t, u, v) && t < tnear)
            {
                tnear = t;
                uv.x = u;
                uv.y = v;
                index = k;
                intersect = true;
            }
        }

        return intersect;
    }

    void getSurfaceProperties(const Vector3f&, const Vector3f&, const uint32_t& index, const Vector2f& uv, Vector3f& N,
                              Vector2f& st) const override
    {
        const Vector3f& v0 = vertices[vertexIndex[index * 3]];
        const Vector3f& v1 = vertices[vertexIndex[index * 3 + 1]];
        const Vector3f& v2 = vertices[vertexIndex[index * 3 + 2]];
        Vector3f e0 = normalize(v1 - v0);
        Vector3f e1 = normalize(v2 - v1);
        N = normalize(crossProduct(e0, e1));
        const Vector2f& st0 = stCoordinates[vertexIndex[index * 3]];
        const Vector2f& st1 = stCoordinates[vertexIndex[index * 3 + 1]];
        const Vector2f& st2 = stCoordinates[vertexIndex[index * 3 + 2]];
        // 看了文章依旧清楚st具体指什么,但他指standardCoorinate
        st = st0 * (1 - uv.x - uv.y) + st1 * uv.x + st2 * uv.y;
    }

    Vector3f evalDiffuseColor(const Vector2f& st) const override
    {
        float scale = 5;
        // fmodf计算浮点余数
        float pattern = (fmodf(st.x * scale, 1) > 0.5) ^ (fmodf(st.y * scale, 1) > 0.5);
        return lerp(Vector3f(0.815, 0.235, 0.031), Vector3f(0.937, 0.937, 0.231), pattern);
    }

    std::unique_ptr<Vector3f[]> vertices;
    uint32_t numTriangles;
    std::unique_ptr<uint32_t[]> vertexIndex;
    std::unique_ptr<Vector2f[]> stCoordinates;
};
