#pragma once
#include <vector>

struct TestForVector
{
public:
	std::vector<float> v;
};

class Object {
public:
	float a = 0;
};

struct TestForVector02
{
public:
	std::vector<Object *> v;
};

