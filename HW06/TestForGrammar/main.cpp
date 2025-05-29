#include "TestForVector.h"
#include <iostream>
#include <thread>

void test01(TestForVector &v1) {
	v1.v = std::vector<float>{ 1,2,3 };
	std::cout << "inside fn: " << v1.v[2] << std::endl;
}

void test02(TestForVector& v1) {
	TestForVector v2;
	v2.v = std::vector<float>{ 1,2,3 };
	v1 = v2;
}

void test03(TestForVector02 &v1) {
	Object obj01, obj02, obj03;
	obj01.a = 1, obj02.a = 2, obj03.a = 3;
	std::vector<Object*> v{ &obj01, &obj02, &obj03 };
	v1.v = v;
	std::cout << "inside fn: " << v1.v[2]->a << std::endl;
}

int threadTest(int k) {
	std::cout << "thread " << k << " start" << std::endl;
	
	int j = 0;
	for (int i = 0; i < 1000000000 * k; i++) {
		j++;
	}
	std::cout << "thread " << k <<" end" << std::endl;

	return j;
}

int main(void) {

	//TestForVector v1;
	// 没有报错，说明vector=赋值至少是浅拷贝，而非地址赋值
	//test01(v1);
	// 默认拷贝函数也没问题
	//test02(v1);
	//std::cout << "outside fn: " << v1.v[2] << std::endl;

	// vector赋值是浅拷贝
	//TestForVector02 v1;
	//test03(v1);
	//std::cout << "outside fn: " << v1.v[2]->a << std::endl;

	// vector的元素初始化至少是浅拷贝
	//Object b;
	//b.a = 2;

	//std::vector<Object> v(3, b);
	//std::cout << "Not Change other index value: " << v[2].a << std::endl;
	//v[1].a = 1;
	//std::cout << "Change other index value: " << v[2].a << std::endl;
	//std::cout << "Change index value: " << v[1].a << std::endl;
	
	// 测试join方法一调用就会阻塞主线程
	std::vector<std::thread> threads;
	for (int i = 0; i < 10; i++) {
		threads.emplace_back(std::thread(threadTest, i));
	}
	for (int i = 0; i < threads.size(); i++) {
		// 答案是的没错, 一但join就会阻塞
		threads[i].join();

		std::cout << "thread " << i << " had joined!" << std::endl;
	}

	return 0;
}