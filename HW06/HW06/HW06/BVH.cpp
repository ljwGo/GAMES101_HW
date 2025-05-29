#include <algorithm>
#include <cassert>
#include "BVH.hpp"
#include <iostream>

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod, int bucketSize)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod), bucketSize(bucketSize),
      primitives(std::move(p))
{
	time_t start, stop;
    time(&start);
    if (primitives.empty())
        return;

    switch (splitMethod)
    {
    case BVHAccel::SplitMethod::NAIVE:
        root = recursiveBuild(primitives);
        break;
    case BVHAccel::SplitMethod::SAH:
        //root = buildSAH(primitives);
        root = recursiveBuildSAH(primitives);
        break;
    default:
        root = recursiveBuild(primitives);
        break;
    }

    time(&stop);
    double diff = difftime(stop, start);
    int hrs = (int)diff / 3600;
    int mins = ((int)diff / 60) - (hrs * 60);
    int secs = (int)diff - (hrs * 3600) - (mins * 60);

    switch (splitMethod)
    {
    case BVHAccel::SplitMethod::NAIVE:
        printf(
            "\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
            hrs, mins, secs);
        break;
    case BVHAccel::SplitMethod::SAH:
        printf(
            "\rSAH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
            hrs, mins, secs);
        break;
    }
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    // 这个objects是meshtriangle而不是triangle
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector<Object*>{objects[0]});
        node->right = recursiveBuild(std::vector<Object*>{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        // 因为只是为了区分最长轴是哪个，所以使用了质心
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds = Union(centroidBounds, objects[i]->getBounds().Centroid());
        // 按最长轴排序
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

        // 按指定轴的中位三角形, 划分为两个bounding box
        auto beginning = objects.begin();
        auto middling = objects.begin() + (objects.size() / 2);
        auto ending = objects.end();

        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        // 递归创建子包围盒
        node->left = recursiveBuild(leftshapes);
        node->right = recursiveBuild(rightshapes);

        node->bounds = Union(node->left->bounds, node->right->bounds);
    }

    return node;
}

BVHBuildNode* BVHAccel::buildSAH(std::vector<Object*> objects) {
    // 构建初始bound box和bucket以适应递归条件
    Bounds3 rootBounds;
    for (auto object : objects) {
        rootBounds = Union(rootBounds, object->getBounds());
    }

    Bucket rootBuck(rootBounds, objects);

    return recursiveBuildSAHDeprecated(rootBuck);
}

BVHBuildNode* BVHAccel::recursiveBuildSAHDeprecated(Bucket &bucket) {

    BVHBuildNode* node = new BVHBuildNode();
    if (bucket.primitives.size() == 1) {
        node->bounds = bucket.primitives[0]->getBounds();
        node->object = bucket.primitives[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (bucket.primitives.size() == 2) {
        Bucket leftBucket(bucket.primitives[0]->getBounds(), std::vector<Object*>{bucket.primitives[0]});
        Bucket rightBucket(bucket.primitives[1]->getBounds(), std::vector<Object*>{bucket.primitives[1]});
        node->left = recursiveBuildSAHDeprecated(leftBucket);
        node->right = recursiveBuildSAHDeprecated(rightBucket);
        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }

    float min_cos = std::numeric_limits<float>::max();
    Bucket min_left_bucket;
    Bucket min_right_bucket;
    float bound_axis_len;
    float per_bucket_len;
    float object_axis_len;
    int bucket_index;

    // 0x轴， 1y轴， 2z轴
    for (int axis = 0; axis < 3; axis++) {
        // 准备对应的buckets
        Bucket init_b;
        // 之前不在堆上分配内存，导致栈溢出了。在递归逻辑正确的情况下
        std::vector<Bucket>* buckets = new std::vector<Bucket>(bucketSize, init_b);

        switch (axis)
        {
        case 0:
            bound_axis_len = bucket.bounds.pMax.x - bucket.bounds.pMin.x;
            per_bucket_len = bound_axis_len / bucketSize;
            // 遍历三角形等，将它们加入到对应的bucket
            for (auto object : bucket.primitives) {
                // 计算落在第几个bucket中, 用包围盒中点代替三角形中心是对的
                object_axis_len = object->getBounds().Centroid().x - bucket.bounds.pMin.x;
                bucket_index = floor((object_axis_len / per_bucket_len));

                (*buckets)[bucket_index].primitives.emplace_back(object);
                (*buckets)[bucket_index].bounds = Union((*buckets)[bucket_index].bounds, object->getBounds());
            }
            break;
        case 1:
            bound_axis_len = bucket.bounds.pMax.y - bucket.bounds.pMin.y;
            per_bucket_len = bound_axis_len / bucketSize;
            // 遍历三角形等，将它们加入到对应的bucket
            for (auto object : bucket.primitives) {
                // 计算落在第几个bucket中, 这里可能会出错
                object_axis_len = object->getBounds().Centroid().y - bucket.bounds.pMin.y;
                bucket_index = floor((object_axis_len / per_bucket_len));

                (*buckets)[bucket_index].primitives.emplace_back(object);
                (*buckets)[bucket_index].bounds = Union((*buckets)[bucket_index].bounds, object->getBounds());
            }
            break;
        case 2:
            bound_axis_len = bucket.bounds.pMax.z - bucket.bounds.pMin.z;
            per_bucket_len = bound_axis_len / bucketSize;
            // 遍历三角形等，将它们加入到对应的bucket
            for (auto object : bucket.primitives) {
                // 计算落在第几个bucket中, 这里可能会出错
                object_axis_len = object->getBounds().Centroid().z - bucket.bounds.pMin.z;
                bucket_index = floor((object_axis_len / per_bucket_len));

                (*buckets)[bucket_index].primitives.emplace_back(object);
                (*buckets)[bucket_index].bounds = Union((*buckets)[bucket_index].bounds, object->getBounds());
            }
            break;
        }

        Bucket left_bucket, right_bucket;
        int better_split_index = -1;

        // 计算bucketSize - 1种分割方法的击中期望
        for (int split_index = 1; split_index < bucketSize; split_index++) {

            Bounds3 left_bound, right_bound;
            int left_p_n = 0, right_p_n = 0;

            // 合并左包围盒
            for (int left_index = 0; left_index < split_index; left_index++) {
                left_bound = Union(left_bound, (*buckets)[left_index].bounds);
                left_p_n += (*buckets)[left_index].primitives.size();
            }
            // 合并右包围盒
            for (int right_index = split_index; right_index < bucketSize; right_index++) {
                right_bound = Union(right_bound, (*buckets)[right_index].bounds);
                right_p_n += (*buckets)[right_index].primitives.size();
            }

            // SAH公式
            float cos = left_bound.SurfaceArea() / bucket.bounds.SurfaceArea() * left_p_n +
                right_bound.SurfaceArea() / bucket.bounds.SurfaceArea() * right_p_n;

            if (cos < min_cos) {
                min_cos = cos;
                better_split_index = split_index;
            }
        }

        if (better_split_index != -1) {
            for (int left_index = 0; left_index < better_split_index; left_index++) {
                left_bucket.bounds = Union(left_bucket.bounds, (*buckets)[left_index].bounds);
                left_bucket.primitives.insert(left_bucket.primitives.end(), (*buckets)[left_index].primitives.begin(), (*buckets)[left_index].primitives.end());
            }

            for (int right_index = better_split_index; right_index < bucketSize; right_index++) {
                right_bucket.bounds = Union(right_bucket.bounds, (*buckets)[right_index].bounds);
                right_bucket.primitives.insert(right_bucket.primitives.end(), (*buckets)[right_index].primitives.begin(), (*buckets)[right_index].primitives.end());
            }
            
            assert(bucket.primitives.size() == (left_bucket.primitives.size() + right_bucket.primitives.size()));

            min_left_bucket = left_bucket;
            min_right_bucket = right_bucket;
        }

        delete buckets;
    }

    node->left = recursiveBuildSAHDeprecated(min_left_bucket);
    node->right = recursiveBuildSAHDeprecated(min_right_bucket);
    node->bounds = Union(node->left->bounds, node->right->bounds);

    return node;
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    long recursiveCount = 0;
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    Intersection inter;

    // TODO Traverse the BVH to find intersection
    // 我们这里没有使用x, y, z的分量方法来计算光线是否打击到包围盒. 用分量速率更快
    // 如果没有交点

    std::array<float, 3> degIsNegative{
        ray.direction.x,
        ray.direction.y,
        ray.direction.z,
    };

    if (!(node->bounds.IntersectP(ray, ray.direction_inv, degIsNegative))) return inter;

    Intersection l_inter;
    Intersection r_inter;

    if (node->right != nullptr) {
        r_inter = getIntersection(node->right, ray);
    }

    if (node->left != nullptr){
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

BVHBuildNode* BVHAccel::recursiveBuildSAH(std::vector<Object*> objects) {
    BVHBuildNode* node = new BVHBuildNode();

    if (objects.size() == 1) {
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuildSAH(std::vector<Object*>{objects[0]});
        node->right = recursiveBuildSAH(std::vector<Object*>{objects[1]});
        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }

    std::vector<Object*> splitLeftObjects;
    std::vector<Object*> splitRightObjects;
    float minCost = std::numeric_limits<float>::max();
    float boundLength, bucketLength, objectLength, bucketIndex;

    // 0->x axis; 1->y axis; 2->z axis. 
    for (int axis = 0; axis < 3; axis++) {
        // vector<>(count, initialValue);
        // 桶子集合, 每个桶子放三角形等Object
        std::vector<std::vector<Object*>>* buckets = new std::vector<std::vector<Object*>>(bucketSize, std::vector<Object*>());
        // 对应桶子的Bounds
        std::vector<Bounds3>* bucketBounds = new std::vector<Bounds3>(bucketSize, Bounds3());
        std::vector<Object*>* leftObjects = new std::vector<Object*>();
        std::vector<Object*>* rightObjects = new std::vector<Object*>();

        Bounds3 nodeBound;

        for (int i = 0; i < objects.size(); i++) {
            nodeBound = Union(nodeBound, objects[i]->getBounds());
        }

        switch (axis)
        {
        case 0:
            boundLength = nodeBound.pMax.x - nodeBound.pMin.x;
            bucketLength = boundLength / bucketSize;
            // 基数排序
            for (auto object : objects) {
                objectLength = object->getBounds().Centroid().x - nodeBound.pMin.x;
                bucketIndex = floor((objectLength / bucketLength));

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
                bucketIndex = floor((objectLength / bucketLength));

                (*buckets)[bucketIndex].emplace_back(object);
                (*bucketBounds)[bucketIndex] = Union((*bucketBounds)[bucketIndex], object->getBounds());
            }
            break;
        case 2:
            boundLength = nodeBound.pMax.z - nodeBound.pMin.z;
            bucketLength = boundLength / bucketSize;
            for (auto object : objects) {
                objectLength = object->getBounds().Centroid().z - nodeBound.pMin.z;
                bucketIndex = floor((objectLength / bucketLength));

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

    return node;
}