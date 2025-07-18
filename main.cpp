#include "tgaimage.h"
#include <iostream>
#include "drawLine.h"
#include "model.h"
#include "geometry.h"
#include "triangleRasterization.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include <vector>








using namespace std;
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor pink = TGAColor(255, 192, 203, 255);
//定义颜色

Model* model = NULL;
//定义三維數值
const int width = 500;
const int height = 500;
const int depth = 300;





//定义MVP变换矩阵
//Matrix ModelView[];

//Matrix ViewPort, mapping 标准立方体空间(-1,1)^3 into 定义的screen space  [0,width]x[0,height]x[0,depth]
Matrix viewport(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;

	m[0][0] = w / 2.f;
	m[1][1] = h / 2.f;
	m[2][2] = depth / 2.f;
	return m;
}
//Matrix Prejection = Matrix::identity(4);






Vec3f world2screen(Vec3f v) {
	//+0.5实现四舍五入,這個原始版本已經假設在標準變化完成
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}



int v0716_drawFullFace_main(int argc, char** argv) {



	//第三个形参声明为TGAImage::RGBA会支持Alpha通道，透明画布
	//声明为TGAImage::RGB则所有像素不透明
	//TGAImage image(200, 200, TGAImage::RGBA);



	/*
	//命令行控制方式和代码方式构造model
	//构造模型(obj文件路径)
	遍历模型的每个三角形面（model->nfaces()）

		获取顶点数据并转换到屏幕空间

		计算光照强度（法向量与光照方向的点积）

		如果面朝光源（intensity > 0），则用 triangle() 函数绘制该三角形

	*/



	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}
	//构造tga(宽，高，指定颜色空间)
	constexpr int width = 900;
	constexpr int height = 900;

	TGAImage image(width, height, TGAImage::RGB);
	//定义光的方向沿着-z
	Vec3f light_dir(0, 0, -1);

	for (int i = 0; i < model->nfaces(); i++) {
		//创建face数组用于保存一个face的三个顶点坐标
		vector<int> face = model->face(i);
		// 存储三角形 3 个顶点的屏幕坐标
		Vec2i screen_coods[3];
		//// 存储三角形 3 个顶点的世界坐标
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			//简单理解，这个vert函数封装了prejection和viewport变换
			screen_coods[j] = Vec2i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2.);
			world_coords[j] = v;
		}
		//定义法向量n，任意两边叉乘一定会得到垂直平面的向量，然后单位化
		Vec3f n = (world_coords[2] - world_coords[0])^(world_coords[1] - world_coords[0]);
		n.normalize();
		//简单计算的点乘代表漫反射是不是有光
		float intensity = n * light_dir;
		if (intensity > 0) {
			//triangle(screen_coods, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
			//triangle_without_zbuffer(screen_coods, image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
		}
	}

	image.flip_vertically();
	image.write_tga_file("drawFullFace_withText.tga");
	return 0;
}



int main(int argc, char** argv) {



	//第三个形参声明为TGAImage::RGBA会支持Alpha通道，透明画布
	//声明为TGAImage::RGB则所有像素不透明
	//TGAImage image(200, 200, TGAImage::RGBA);



	/*
	//命令行控制方式和代码方式构造model
	//构造模型(obj文件路径)
	遍历模型的每个三角形面（model->nfaces()）

		获取顶点数据并转换到屏幕空间

		计算光照强度（法向量与光照方向的点积）

		如果面朝光源（intensity > 0），则用 triangle() 函数绘制该三角形

	*/



	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}
	float* zbuffer = new float[height * width];
	//init zbuffer
	for (int i = width * height; i--; zbuffer[i] = -numeric_limits<float>::max());

	TGAImage image(width, height, TGAImage::RGB);
	//定义光的方向沿着-z
	Vec3f light_dir(0, 0, -1);
	




	for (int i = 0; i < model->nfaces(); i++) {
		//创建face数组用于保存一个face的三个顶点坐标
		vector<int> face = model->face(i);
		// 存储三角形 3 个顶点的屏幕坐标
		Vec3f screen_coods[3];
		//// 存储三角形 3 个顶点的世界坐标
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			
			screen_coods[j] = world2screen(v);
			world_coords[j] = v;
		}
		//定义法向量n，任意两边叉乘一定会得到垂直平面的向量，然后单位化
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		//简单计算的点乘代表漫反射
		float intensity = n * light_dir;
		if (intensity > 0) {
			triangle(model,i,screen_coods, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
			//triangle(screen_coods, zbuffer,image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
		}
	}

	image.flip_vertically();
	image.write_tga_file("drawAficaFace_withTexture.tga");
	return 0;
}

