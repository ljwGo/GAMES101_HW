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
        // Ͱ�Ӽ���, ÿ��Ͱ�ӷ������ε�Object
        std::vector<std::vector<Object *>>* buckets = new std::vector<std::vector<Object*>>(bucketSize, std::vector<Object *>());
        // ��ӦͰ�ӵ�Bounds
        std::vector<Bounds3>* bucketBounds = new std::vector<Bounds3>(bucketSize, Bounds3());
        std::vector<Object*>* leftObjects = new std::vector<Object*>();
        std::vector<Object*>* rightObjects = new std::vector<Object*>();

        switch (axis)
        {
        case 0:
            boundLength = nodeBound.pMax.x - nodeBound.pMin.x;
            bucketLength = boundLength / bucketSize;
            // ��������
            for (auto object : objects) {
                objectLength = object->getBounds().Centroid().x - nodeBound.pMin.x;
                bucketIndex = clamp(0, bucketSize-1, floor((objectLength / bucketLength)));

                // ����Ͱ��
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
                // ���﷢���˸��㾫�����µĴ���, ����Χ�����ļ��俿������Χ�б�Ե
                // ���׵�����������buckets����, ��������ĵ�ģ��, ��Χ��y�߶ȼ���Ϊ0
                // ע��: �����clamp���ܻ�����һ�������Ĵ���
                objectLength = object->getBounds().Centroid().z - nodeBound.pMin.z;
                bucketIndex = clamp(0, bucketSize-1, floor((objectLength / bucketLength)));

                (*buckets)[bucketIndex].emplace_back(object);
                (*bucketBounds)[bucketIndex] = Union((*bucketBounds)[bucketIndex], object->getBounds());
            }
            break;
        }

        int betterSplitIndex = -1;
        // ����bucketSize-1�ַָ���Ļ�������
        for (int splitIndex = 1; splitIndex < bucketSize; splitIndex++) {

            Bounds3 leftBound, rightBound;
            int countLeft = 0, countRight = 0;

            // �ϲ����Χ��
            for (int i = 0; i < splitIndex; i++) {
                leftBound = Union(leftBound, (*bucketBounds)[i]);
                countLeft += (*buckets)[i].size();
            }
            // �ϲ��Ұ�Χ��
            for (int j = splitIndex; j < bucketSize; j++) {
                rightBound = Union(rightBound, (*bucketBounds)[j]);
                countRight += (*buckets)[j].size();
            }

            // SAH��ʽ
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
        // ��������
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
    // ������ӽڵ�
    if(node->left == nullptr || node->right == nullptr){
        // object������meshtriangle��, Ҳ������triangle
        node->object->Sample(pos, pdf);
        // pdfʹ�û��нڵ����  / ��(��)�������ʾ
        pdf *= node->area;
        return;
    }
    // ӳ��p��ĳ���ӽڵ�
    if(p < node->left->area) getSample(node->left, p, pos, pdf);
    else getSample(node->right, p - node->left->area, pos, pdf);
}

// ֱ��ΪʲôbvhҪ�в�����, ��Ϊ����Ĺ�ԴҲ��ʹ�õ�MeshTriangle, �Թ�Դ�Ĳ������������, ���͹����й�
void BVHAccel::Sample(Intersection &pos, float &pdf){
    // ���ȡС�ڸ������һ�����ֵp
    float p = std::sqrt(get_random_float()) * root->area;
    getSample(root, p, pos, pdf);
    // pdfʹ�û��нڵ����  / ��(��)�������ʾ, ˵���Ǿ��Ȳ���
    pdf /= root->area;
}