#pragma execution_character_set("gbk") 
#include <algorithm>
#include <cassert>
#include "BVH.hpp"

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod, int bucketSize)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod), bucketSize(bucketSize),
      primitives(std::move(p))
{
    time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;

    if (splitMethod == SplitMethod::SAH) {
        root = recursiveBuildSAH(primitives);
    }
    else {
        root = recursiveBuild(primitives);
    }

    time(&stop);
    double diff = difftime(stop, start);
    int hrs = (int)diff / 3600;
    int mins = ((int)diff / 60) - (hrs * 60);
    int secs = (int)diff - (hrs * 3600) - (mins * 60);

    printf(
        "\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
        hrs, mins, secs);
}

BVHBuildNode* BVHAccel::recursiveBuildSAH(std::vector<Object*> objects) {
    BVHBuildNode* node = new BVHBuildNode();
    
    if (objects.size() == 1) {
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        node->area = objects[0]->getArea();
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuildSAH(std::vector<Object *>{objects[0]});
        node->right = recursiveBuildSAH(std::vector<Object*>{objects[1]});
        node->bounds = Union(node->left->bounds, node->right->bounds);
        node->area = node->left->area + node->right->area;
        return node;
    }

    std::vector<Object*> splitLeftObjects;
    std::vector<Object*> splitRightObjects;
    float minCost = std::numeric_limits<float>::max();
    float boundLength, bucketLength, objectLength, bucketIndex;

    Bounds3 nodeBound;

    for (int i = 0; i < objects.size(); i++) {
        nodeBound = Union(nodeBound, objects[i]->getBounds());
    }

    // 0->x axis; 1->y axis; 2->z axis. 
    for (int axis = 0; axis < 3; axis++) {
        // vector<>(count, initialValue);
        // 桶子集合, 每个桶子放三角形等Object
        std::vector<std::vector<Object *>>* buckets = new std::vector<std::vector<Object*>>(bucketSize, std::vector<Object *>());
        // 对应桶子的Bounds
        std::vector<Bounds3>* bucketBounds = new std::vector<Bounds3>(bucketSize, Bounds3());
        std::vector<Object*>* leftObjects = new std::vector<Object*>();
        std::vector<Object*>* rightObjects = new std::vector<Object*>();

        switch (axis)
        {
        case 0:
            boundLength = nodeBound.pMax.x - nodeBound.pMin.x;
            bucketLength = boundLength / bucketSize;
            // 基数排序
            for (auto object : objects) {
                objectLength = object->getBounds().Centroid().x - nodeBound.pMin.x;
                bucketIndex = clamp(0, bucketSize-1, floor((objectLength / bucketLength)));

                // 进入桶子
                (*buckets)[bucketIndex].emplace_back(object);
                (*bucketBounds)[bucketIndex] = Union((*bucketBounds)[bucketIndex], object->getBounds());
            }
            break;
        case 1:
            boundLength = nodeBound.pMax.y - nodeBound.pMin.y;
            bucketLength = boundLength / bucketSize;
            for (auto object : objects) {
                objectLength = object->getBounds().Centroid().y - nodeBound.pMin.y;
                bucketIndex = clamp(0, bucketSize-1, floor((objectLength / bucketLength)));

                (*buckets)[bucketIndex].emplace_back(object);
                (*bucketBounds)[bucketIndex] = Union((*bucketBounds)[bucketIndex], object->getBounds());
            }
            break;
        case 2:
            boundLength = nodeBound.pMax.z - nodeBound.pMin.z;
            bucketLength = boundLength / bucketSize;
            for (auto object : objects) {
                // 这里发生了浮点精度误差导致的错误, 当包围盒中心及其靠近父包围盒边缘
                // 容易导致索引超出buckets容量, 比如这里的灯模型, 包围盒y高度几乎为0
                // 注意: 这里的clamp可能会屏蔽一个致命的错误
                objectLength = object->getBounds().Centroid().z - nodeBound.pMin.z;
                bucketIndex = clamp(0, bucketSize-1, floor((objectLength / bucketLength)));

                (*buckets)[bucketIndex].emplace_back(object);
                (*bucketBounds)[bucketIndex] = Union((*bucketBounds)[bucketIndex], object->getBounds());
            }
            break;
        }

        int betterSplitIndex = -1;
        // 计算bucketSize-1种分割方法的击中期望
        for (int splitIndex = 1; splitIndex < bucketSize; splitIndex++) {

            Bounds3 leftBound, rightBound;
            int countLeft = 0, countRight = 0;

            // 合并左包围盒
            for (int i = 0; i < splitIndex; i++) {
                leftBound = Union(leftBound, (*bucketBounds)[i]);
                countLeft += (*buckets)[i].size();
            }
            // 合并右包围盒
            for (int j = splitIndex; j < bucketSize; j++) {
                rightBound = Union(rightBound, (*bucketBounds)[j]);
                countRight += (*buckets)[j].size();
            }

            // SAH公式
            float cost = leftBound.SurfaceArea() / nodeBound.SurfaceArea() * countLeft +
                rightBound.SurfaceArea() / nodeBound.SurfaceArea() * countRight;

            if (cost < minCost) {
                minCost = cost;
                betterSplitIndex = splitIndex;
            }
        }

        if (betterSplitIndex != -1) {
            for (int i = 0; i < betterSplitIndex; i++) {
                leftObjects->insert(leftObjects->end(), (*buckets)[i].begin(), (*buckets)[i].end());
            }

            for (int j = betterSplitIndex; j < bucketSize; j++) {
                rightObjects->insert(rightObjects->end(), (*buckets)[j].begin(), (*buckets)[j].end());
            }

            assert(objects.size() == (leftObjects->size() + rightObjects->size()));

            splitLeftObjects = *leftObjects;
            splitRightObjects = *rightObjects;
        }

        delete buckets;
        delete bucketBounds;
        delete leftObjects;
        delete rightObjects;
    }

    node->left = recursiveBuildSAH(splitLeftObjects);
    node->right = recursiveBuildSAH(splitRightObjects);
    node->bounds = Union(node->left->bounds, node->right->bounds);
    node->area = node->left->area + node->right->area;

    return node;
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        // 添加了面积
        node->area = objects[0]->getArea();
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector{objects[0]});
        node->right = recursiveBuild(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        node->area = node->left->area + node->right->area;
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        auto beginning = objects.begin();
        auto middling = objects.begin() + (objects.size() / 2);
        auto ending = objects.end();

        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        node->left = recursiveBuild(leftshapes);
        node->right = recursiveBuild(rightshapes);

        node->bounds = Union(node->left->bounds, node->right->bounds);
        node->area = node->left->area + node->right->area;
    }

    return node;
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    // TODO Traverse the BVH to find intersection
    Intersection inter;

    if (!(node->bounds.IntersectP(ray, ray.direction_inv))) return inter;

    Intersection l_inter;
    Intersection r_inter;

    if (node->right != nullptr) {
        r_inter = getIntersection(node->right, ray);
    }

    if (node->left != nullptr) {
        l_inter = getIntersection(node->left, ray);
    }

    if (node->left == nullptr && node->right == nullptr) {
        return node->object->getIntersection(ray);
    }

    if (r_inter.distance < l_inter.distance) {
        return r_inter;
    }
    else {
        return l_inter;
    }

    return inter;
}

void BVHAccel::getSample(BVHBuildNode* node, float p, Intersection &pos, float &pdf){
    // 如果是子节点
    if(node->left == nullptr || node->right == nullptr){
        // object可以是meshtriangle等, 也可以是triangle
        node->object->Sample(pos, pdf);
        // pdf使用击中节点面积  / 根(总)面积来表示
        pdf *= node->area;
        return;
    }
    // 映射p到某个子节点
    if(p < node->left->area) getSample(node->left, p, pos, pdf);
    else getSample(node->right, p - node->left->area, pos, pdf);
}

// 直到为什么bvh要有采样了, 因为这里的光源也是使用的MeshTriangle, 对光源的采样是随机采样, 不和光线有关
void BVHAccel::Sample(Intersection &pos, float &pdf){
    // 随机取小于根面积的一个面积值p
    float p = std::sqrt(get_random_float()) * root->area;
    getSample(root, p, pos, pdf);
    // pdf使用击中节点面积  / 根(总)面积来表示, 说明是均匀采样
    pdf /= root->area;
}