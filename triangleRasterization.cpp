#include <iostream>
#include "geometry.h"
#include "drawLine.h"
#include "model.h"
#include "tgaimage.h"
#include <vector>


using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

void line(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color) {
	bool steep = false;
	if (std::abs(p0.x - p1.x) < std::abs(p0.y - p1.y)) {
		std::swap(p0.x, p0.y);
		std::swap(p1.x, p1.y);
		steep = true;
	}
	if (p0.x > p1.x) {
		std::swap(p0, p1);
	}

	for (int x = p0.x; x <= p1.x; x++) {
		float t = (x - p0.x) / (float)(p1.x - p0.x);
		int y = p0.y * (1. - t) + p1.y * t;
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
	}
}



void triangle_with_horizontal_scan(Vec2i a, Vec2i b, Vec2i c, TGAImage& image, TGAColor color) {

	//退化成线段不用处理
	if (a.y == b.y && a.y == c.y) return;
	//old school style
	if (a.y > b.y) {
		swap(a, b);
	}
	if (a.y > c.y) {
		swap(a, c);
	}
	if (b.y > c.y) {
		swap(b, c);
	}
	//最高点和最低点的处置高度差
	int total_height = c.y - a.y;

	for (int i = 0; i < total_height; ++i) {
		//如果ab在一个水平线上那就只需要画上半部分
		bool top_half = i > b.y - a.y || b.y == a.y;
		int	 half_height = top_half ? c.y - b.y : b.y - a.y;
		int	bottom_half_height = b.y - a.y;
		//这两个比例计算的是当前水平高度占总高度和当前部分的比例
		float alpha = (float)i / total_height;
		//这里也很好理解，如果是上半部分就要先减去下半部分的水平高度再算比例
		float beta = (float)(i - (top_half ? bottom_half_height : 0)) / half_height;

		//我们知道点和点之间的减法运算会得到差向量a- b 得到b->a
		//这样就得到了处于同一水平线的两个点
		Vec2i A = a + (c - a) * alpha;
		Vec2i B = top_half ? b + (c - b) * beta : a + (b - a) * beta;

		if (A.x > B.x) {
			swap(A, B);
		}
		//右端点也需要画
		for (int j = A.x; j <= B.x; ++j)
		{
			image.set(j, a.y + i, color);
		}
	}

	/*
	* 画水平线逐个点填充计算量比调用这个画线的函数快
	drawLineBresenham(a.u, a.v,b.u,b.v, image, color);
	drawLineBresenham(b.u, b.v, c.u, c.v, image, green);
	drawLineBresenham(c.u, c.v, a.u, a.v, image, red);
	*/

}




/*
definition
重心坐标描述了点 P 在三角形 ABC 内的位置，可以理解为 P 对三个顶点的“权重”。
计算方式基于面积比：

α = 面积(PBC) / 面积(ABC)

β = 面积(APC) / 面积(ABC)

γ = 面积(ABP) / 面积(ABC)

在之前的叉积计算中：

u.z = ACₓ·ABᵧ - ABₓ·ACᵧ = 2 × 面积(ABC)
（因为叉积的模等于平行四边形面积）

u.y = APₓ·ACᵧ - ACₓ·APᵧ = 2 × 面积(APC)

u.x = ABₓ·APᵧ - APₓ·ABᵧ = 2 × 面积(ABP)

因此：

β = 面积(APC) / 面积(ABC) = u.y / u.z

γ = 面积(ABP) / 面积(ABC) = u.x / u.z

α = 1 - β - γ = 1 - (u.x + u.y)/u.z
*/



/*
一个点如果在三角形内部那么它与三个点同一时针方向的叉乘得到的是同方向的
如果有一项叉乘结果为0，在一条边上，两项叉乘结果为0在顶点上
对于任意的在（A,B,C）三角形内的点P，重心坐标满足
P = α·A + β·B + γ·C  α + β + γ = 1
return 三个浮点数 (α, β, γ)，即点P相对于三角形ABC的重心坐标
判断条件
在三角形内：当 α, β, γ ∈ [0,1] 且 α+β+γ=1

在边上：某一个坐标为0，其他两个在[0,1]之间

在顶点：某一个坐标为1，其他两个为0

在三角形外：任一坐标为负或大于1
*/



/*
Vec3f barycentric_2d(Vec2i* pts, Vec2i P) {




	 `pts` and `P` has integer value as coordinates
	   so `abs(u[2])` < 1 means `u[2]` is 0, that means
	   triangle is degenerate, in this case return something with negative coordinates

		special case ABC共线
		u.z = (B.x - A.x)(C.y - A.y) - (B.y - A.y)(C.x - A.x)= 2 * Area(ABC)
		这里每个点采用的是int表示坐标，在单位向量计算下，每个向量最小面积就是0.5
		那么u.z < 1即可以判断三角形面积为0 三点共线





			Vec3f u = Vec3f(
		pts[2].x - pts[0].x,
		pts[1].x - pts[0].x,
		pts[0].x - P.x
	) ^ Vec3f(
		pts[2].y - pts[0].y,
		pts[1].y - pts[0].y,
		pts[0].y - P.y
	);
	if (abs(u.z) < 1) return Vec3f(-1, 1, 1);


	return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);


	 



	Vec3f temp ;
	return temp;

	}

	*/


//由上面的二维拓展到三维
Vec3f barycentric_3d(Vec3f* tri, Vec3f P) {
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

void triangle_old(Model* model,Vec3f *tri, float* zbuffer,TGAImage& image, TGAColor color) {
	
	//Vec2f bboxmin(image.get_width() - 1, image.get_height() - 1);
	//Vec2f bboxmax(0, 0);
	//Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	Vec2f bboxmin(numeric_limits<float>::max(), numeric_limits<float>::max());
	Vec2f bboxmax(-numeric_limits<float>::max(), numeric_limits<float>::max());
	//類似openGL一类的库直接提供了clamp函数
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	//这里找到屏幕空间上包裹住该三角形的最小矩形
	/*
	* 	for (int i = 0; i < 3; i++) {
		bboxmin.x = max(0.f, min(bboxmin.x, tri[i].x));
		bboxmin.y = max(0.f, min(bboxmin.y, tri[i].y));
		bboxmax.x = min(clamp.x, max(bboxmax.x, tri[i].x));
		bboxmax.y = min(clamp.y, max(bboxmax.y, tri[i].y));
		等价于下面的简洁形式
	}
	*/
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = max(0.f, min(bboxmin[j], tri[i][j]));
			bboxmax[j] = min(clamp[j], max(bboxmax[j], tri[i][j]));
		}
	}


	//对上面找到的矩形空间内的点，在三角形内的就上色
	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; ++P.x) {
		for (P.y=bboxmin.y;P.y<=bboxmax.y;++P.y){
			Vec3f bc_location = barycentric_3d(tri, P);
			if (bc_location.x < 0|| bc_location.y < 0	|| bc_location.z <0){
				continue;
			}
			//判断完平面内是否在三角形中现在来考虑z深度,zbuffer算法存储当前屏幕上的点距离camera最近的值，如果有更近的（覆盖）才渲染
			P.z = 0;
			for (int i = 0;i < 3;++i){
				//P.z=u⋅tri[0].z+v⋅tri[1].z+w⋅tri[2].z
				//通过三角形三个顶点的深度值（tri[0].z, tri[1].z, tri[2].z）和当前像素的质心坐标（u, v, w）进行 加权平均，就能得到该像素的深度值 P.z
				P.z += tri[i][2] * bc_location[i];
			}


			//将 2D 屏幕坐标 (P.x, P.y) 转换为一维数组索引，前面定义的zbuffer 是按行存储的而不是二维数组）
			if (zbuffer[int(P.x + image.get_width()* P.y)] < P.z ) {
				//更新目前最大深度
				zbuffer[int(P.x + image.get_width() * P.y)] = P.z;

				
				
				image.set(P.x, P.y, color);
			}

			

		}
	}


	/*
	* 画水平线逐个点填充计算量比调用这个画线的函数快
	drawLineBresenham(a.u, a.v,b.u,b.v, image, color);
	drawLineBresenham(b.u, b.v, c.u, c.v, image, green);
	drawLineBresenham(c.u, c.v, a.u, a.v, image, red);
	*/


}


/*
void triangle_without_zbuffer(Vec2i* tri, TGAImage& image, TGAColor color) {

	Vec2i bboxmin(image.get_width() - 1, image.get_height() - 1);
	Vec2i bboxmax(0, 0);
	Vec2i clamp(image.get_width() - 1, image.get_height() - 1);

	//这里找到屏幕空间上包裹住该三角形的最小矩形
	for (int i = 0; i < 3; i++) {
		bboxmin.x = max(0, min(bboxmin.x, tri[i].x));
		bboxmin.y = max(0, min(bboxmin.y, tri[i].y));
		bboxmax.x = min(clamp.x, max(bboxmax.x, tri[i].x));
		bboxmax.y = min(clamp.y, max(bboxmax.y, tri[i].y));
	}
	//对上面找到的矩形空间内的点，在三角形内的就上色
	*画水平线逐个点填充计算量比调用这个画线的函数快
		drawLineBresenham(a.u, a.v, b.u, b.v, image, color);
	drawLineBresenham(b.u, b.v, c.u, c.v, image, green);
	drawLineBresenham(c.u, c.v, a.u, a.v, image, red);
	Vec2i P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; ++P.x) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; ++P.y) {
			Vec3f bc_location = barycentric_2d(tri, P);
			if (bc_location.x < 0 || bc_location.y < 0 || bc_location.z < 0) {
				continue;
			}
			image.set(P.x, P.y, color);

		}
	}



}

*/





int drawFullTriangle_main() {

	//构造tga(宽，高，指定颜色空间)
	constexpr int width = 800;
	constexpr int height = 800;

	TGAImage image(width, height, TGAImage::RGB);
	Vec2i tri[3] = { Vec2i(200,80),Vec2i(743,733) ,Vec2i(598,399) };
	//triangle(tri, image, blue);

	image.flip_vertically();
	image.write_tga_file("drawstrianglefinal.tga");
	return 0;

}



void triangle(Model* model, int iface, Vec3f* tri, float* zbuffer, TGAImage& image, TGAColor color) {
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
		int uv_idx = model->faces_[iface][i][1];  // 直接访问faces_成员
		uvs[i] = model->uv_[uv_idx];  // 获取归一化UV坐标
	}

	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric_3d(tri, P);
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
				uv_pixel.x * model->diffusemap_.get_width(),
				uv_pixel.y * model->diffusemap_.get_height()
			);

			// 深度测试
			int idx = int(P.x + P.y * image.get_width());
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;

				// 获取纹理颜色并应用光照强度
				TGAColor color = model->diffuse(uv_pixel_coords);
				

				image.set(P.x, P.y, color);
			}
		}
	}
}

