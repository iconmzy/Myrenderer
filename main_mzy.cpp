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

Model* model_mzy = NULL;
//定义三維數值
const int width = 800;
const int height = 800;
const int depth = 255;




//光源角度与摄像机位置
const Vec3f light_dir(0.2, 0.14, -1);
const Vec3f camera(0, 0, 3);



//定义MVP变换矩阵


//
Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);

	/*
   +--------------------------------+
   |    |x|     
   |    |y|     
   |    |z|     
   |    |w|    
	在各个分量上除以m，从4d的齐次坐标系映射回三维空间
   +--------------------------------+
	*/
}

//4d ---> 3d w分量设置1表示点 坐标
// Matrix v2m(Vec3f v) {
// 	Matrix m(4, 1);
// 	m[0][0] = v.x;
// 	m[1][0] = v.y;
// 	m[2][0] = v.z;
// 	m[3][0] = 1.f;
// 	return m;
// }



//Matrix ViewPort, mapping 标准立方体空间(-1,1)^3 into 定义的screen space  [0,width]x[0,height]x[0,depth]
Matrix viewport_mzy(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	//各个方向的平移量
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;

	//各个方向的缩放量
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


Vec3f world2screen_MVP(Vec3f v) {
	
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}




Vec3f barycentric_mzy(Vec3f* tri, Vec3f P) {
	Vec3f s[2];


	for (int i = 2; i--; ) {
		s[i][0] = tri[2][i] - tri[0][i];
		s[i][1] = tri[1][i] - tri[0][i];
		s[i][2] = tri[0][i] - P[i];
	}
	//[u,v,1]和[AB,AC,PA]对应的x和y向量都垂直，所以叉乘
	Vec3f u = s[0] ^ s[1];
	//三点共线时，会导致u[2]为0，此时返回(-1,1,1)
	if (std::abs(u[2]) > 1e-2)
		//若1-u-v，u，v全为大于0的数，表示点在三角形内部
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1);

}




void triangle(int iface, Vec3f* tri, float* zbuffer, TGAImage& image, TGAColor color) {
	// 计算包围盒
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], tri[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], tri[i][j]));
		}
	}

	// 获取三个顶点的UV坐标（归一化）
	Vec2f uvs[3];
	for (int i = 0; i < 3; i++) {
		int uv_idx = model_mzy->faces_[iface][i][1];  // 直接访问faces_成员
		uvs[i] = model_mzy->uv_[uv_idx];  // 获取归一化UV坐标
	}

	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric_mzy(tri, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

			// 插值深度
			P.z = 0;
			for (int i = 0; i < 3; i++) P.z += tri[i][2] * bc_screen[i];

			// 插值UV坐标（归一化）
			Vec2f uv_pixel;
			for (int i = 0; i < 3; i++) {
				uv_pixel = uv_pixel + uvs[i] * bc_screen[i];
			}

			// 转换为纹理像素坐标
			Vec2i uv_pixel_coords(
				uv_pixel.x * model_mzy->diffusemap_.get_width(),
				uv_pixel.y * model_mzy->diffusemap_.get_height()
			);

			// 深度测试
			int idx = int(P.x + P.y * image.get_width());
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;

				// 获取纹理颜色并应用光照强度
				TGAColor color = model_mzy->diffuse(uv_pixel_coords);


				image.set(P.x, P.y, color);
			}
		}
	}
}






int main_oldversion(int argc, char** argv) {



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
		model_mzy = new Model(argv[1]);
	}
	else {
		model_mzy = new Model("obj/african_head.obj");
	}
	float* zbuffer = new float[height * width];
	//init zbuffer
	for (int i = width * height; i--; zbuffer[i] = -numeric_limits<float>::max());

	TGAImage image(width, height, TGAImage::RGB);
	
	Matrix Projection = Matrix::identity(4);
	Matrix ViewPort = viewport_mzy(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	Projection[3][2] = -1.f / camera.z;


		/*初始化的目的是映射到如下范围
	(0, height)                      (width, height)
   +--------------------------------+
   |         Screen Space           |
   |    +---------------------+     |
   |    |    Sub-Viewport     |     |
   |    |  (x=width/8,        |     |
   |    |   y=height/8)       |     |
   |    |   w=3width/4,       |     |
   |    |   h=3height/4       |     |
   |    +---------------------+     |
   |                                |
   +--------------------------------+
(0, 0)                        (width, 0)
	*/


	for (int i = 0; i < model_mzy->nfaces(); i++) {
		//创建face数组用于保存一个face的三个顶点坐标
		vector<int> face = model_mzy->face(i);
		// 存储三角形 3 个顶点的屏幕坐标
		Vec3f screen_coods[3];
		//// 存储三角形 3 个顶点的世界坐标
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model_mzy->vert(face[j]);
			
			//screen_coods[j] = world2screen(v);
			screen_coods[j] = m2v(ViewPort * Projection * v2m(v));
			world_coords[j] = v;

		}
		//定义法向量n，任意两边叉乘一定会得到垂直平面的向量，然后单位化
		Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
		n.normalize();
		//简单计算的点乘代表漫反射
		float intensity = n * light_dir;
		if (intensity > 0) {
			triangle(i,screen_coods, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
			//triangle(screen_coods, zbuffer,image, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255));
		}
	}

	image.flip_vertically();
	image.write_tga_file("drawMVPAficaFace_withTexture.tga");
	return 0;
}

