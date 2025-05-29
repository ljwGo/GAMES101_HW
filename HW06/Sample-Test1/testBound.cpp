#include "pch.h"
#include "gtest/gtest.h"
#include "../HW06/HW06/Bounds3.hpp"

// uniform data
class Bounds3Test : public ::testing::Test {
protected:
	Bounds3Test() {
		;
	}
	void SetUp() override {
		ParallelNotIntersect = Ray(Vector3f(-2., -2, 0), Vector3f(1.,0, 0));
		ParallelIntersect = Ray(Vector3f(2., 0, 0), Vector3f(-1., 0, 0));
		RayO_Ne_D_Posi = Ray(Vector3f(-2., 0, 0), Vector3f(1., 0.5f, 0.5f));
		RayO_Posi_D_Ne = Ray(Vector3f(2., 0, 0), Vector3f(-1., -0.3f, -0.8f));
		RayO_Ne_D_Ne = Ray(Vector3f(-2., 0, 0), Vector3f(-1., -0.3f, -0.8f));
		RayO_Posi_D_Posi = Ray(Vector3f(2., 0, 0), Vector3f(1., 0.5f, 0.5f));
		RayO_Inside_D_Posi = Ray(Vector3f(0, 0, 0), Vector3f(1., 0.5f, 0.5f));
		RayO_Inside_D_Ne = Ray(Vector3f(0, 0, 0), Vector3f(-1., -0.3f, -0.8f));
	
		Box = Bounds3(Vector3f(-1, -1, -1), Vector3f(1., 1., 1.));
	}
public:
	Bounds3 Box;

	Ray ParallelNotIntersect;
	Ray ParallelIntersect;
	Ray RayO_Ne_D_Posi;  // origin: -2, 0, 0; direction: 1, 0.5, 0.5
	Ray RayO_Posi_D_Ne;  // origin: 2, 0, 0; direction: -1, -0.3, 0.8
	Ray RayO_Ne_D_Ne;
	Ray RayO_Posi_D_Posi;
	Ray RayO_Inside_D_Posi;
	Ray RayO_Inside_D_Ne;
};

TEST_F(Bounds3Test, RayIntersection) {
	EXPECT_LT(fmin(-1, 3), 0);

	std::array<float, 3> degIsNegative;

	degIsNegative = std::array<float, 3>{
		ParallelNotIntersect.direction.x,
		ParallelNotIntersect.direction.y,
		ParallelNotIntersect.direction.z,
	};
	EXPECT_FALSE(Box.IntersectP(ParallelNotIntersect, ParallelNotIntersect.direction_inv, degIsNegative)) << "平行光不应该和box相交";

	degIsNegative = std::array<float, 3>{
		ParallelIntersect.direction.x,
		ParallelIntersect.direction.y,
		ParallelIntersect.direction.z,
	};
	EXPECT_TRUE(Box.IntersectP(ParallelIntersect, ParallelIntersect.direction_inv, degIsNegative)) << "平行光应该和box相交";

	degIsNegative = std::array<float, 3>{
		RayO_Ne_D_Posi.direction.x,
		RayO_Ne_D_Posi.direction.y,
		RayO_Ne_D_Posi.direction.z,
	};
	EXPECT_TRUE(Box.IntersectP(RayO_Ne_D_Posi, RayO_Ne_D_Posi.direction_inv, degIsNegative)) << "从左到右的光不和box相交";

	degIsNegative = std::array<float, 3>{
		RayO_Posi_D_Ne.direction.x,
		RayO_Posi_D_Ne.direction.y,
		RayO_Posi_D_Ne.direction.z,
	};
	EXPECT_TRUE(Box.IntersectP(RayO_Posi_D_Ne, RayO_Posi_D_Ne.direction_inv, degIsNegative)) << "从右到左的光不和box相交";

	degIsNegative = std::array<float, 3>{
		RayO_Ne_D_Ne.direction.x,
		RayO_Ne_D_Ne.direction.y,
		RayO_Ne_D_Ne.direction.z,
	};
	EXPECT_FALSE(Box.IntersectP(RayO_Ne_D_Ne, RayO_Ne_D_Ne.direction_inv, degIsNegative)) << "从右到右和box相交";

	degIsNegative = std::array<float, 3>{
		RayO_Posi_D_Posi.direction.x,
		RayO_Posi_D_Posi.direction.y,
		RayO_Posi_D_Posi.direction.z,
	};
	EXPECT_FALSE(Box.IntersectP(RayO_Posi_D_Posi, RayO_Posi_D_Posi.direction_inv, degIsNegative)) << "从左到左和box相交";

	degIsNegative = std::array<float, 3>{
		RayO_Inside_D_Posi.direction.x,
		RayO_Inside_D_Posi.direction.y,
		RayO_Inside_D_Posi.direction.z,
	};
	EXPECT_TRUE(Box.IntersectP(RayO_Inside_D_Posi, RayO_Inside_D_Posi.direction_inv, degIsNegative)) << "从里到右的光不和box相交";

	degIsNegative = std::array<float, 3>{
		RayO_Inside_D_Ne.direction.x,
		RayO_Inside_D_Ne.direction.y,
		RayO_Inside_D_Ne.direction.z,
	};
	EXPECT_TRUE(Box.IntersectP(RayO_Inside_D_Ne, RayO_Inside_D_Ne.direction_inv, degIsNegative)) << "从里到左的光不和box相交";

}