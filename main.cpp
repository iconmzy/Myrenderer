#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "my_gl.h"


//宏定义边界限制
#define CLAMP(t) ((t>1.f)?1.f:((t<0.f)?0.f:t))



const int width = 600;
const int height = 600;
const int depth = 255;
float  ambient = 8.;
float spec_coefficient = 0.6;

Model* model = NULL;
//camera location
Vec3f light_dir(1, 1, 1);
Vec3f       camera(0, -1, 3);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);




struct FlatShader: public IShader{
	//NXN矩阵记录转换坐标的点
	mat<3, 3, float> varing_tri;  
	/*
	每列代表一个顶点 (共3个顶点)
	每行代表一个坐标分量(x, y, z)

	  varying_tri = [ x0, x1, x2 ]  ← 第0行：所有顶点的x分量
					[ y0, y1, y2 ]  ← 第1行：所有顶点的y分量
					[ z0, z1, z2 ]  ← 第2行：所有顶点的z分量
	*/
	virtual Vec4f vertex(int iface, int nthvert) {
		////读取第 iface 个面/三角形的第nthvert个顶点改成齐次坐标转化为齐次坐标系
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
		gl_vertex = Projection * ModelView * gl_vertex;
		//经过投影变化的顶点坐标/z分量（透视除法）后的xzy分量添加到varing_tri的不同列
		varing_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));
		gl_vertex = ViewPort * gl_vertex;
		return gl_vertex;


	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		//法线 是任意两边的叉乘
		Vec3f n = cross(varing_tri.col(1) - varing_tri.col(0), varing_tri.col(2) - varing_tri.col(0)).normalize();
		//点乘判断光照是否平行三角形
		float intensity = CLAMP(n * light_dir);
		color = TGAColor(255, 255, 255) * intensity;
		return false;
	}
};



struct GouraudShader :public IShader {
	Vec3f varing_intensity;

	virtual Vec4f vertex(int iface, int nthvert) {
		////读取第 iface 个面/三角形的第nthvert个顶点改成齐次坐标转化为齐次坐标系
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));


		//Gouraud Shader 只需计算顶点的最终屏幕坐标，不需要像 Flat Shader 那样存储投影后的3D坐标。
		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;
		varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);
		return gl_vertex;
	}
 	virtual bool fragment(Vec3f bar, TGAColor& color) {
 		float intensity = varing_intensity * bar;
		color = TGAColor(255, 255, 255) * intensity;
 		return false;
 	}
// 	virtual bool fragment(Vec3f bar, TGAColor& color) {
// 		float intensity = varing_intensity * bar; //interpolate intensity for current Pixel
// 		if (intensity > .85)  intensity = 1;
// 		else if (intensity > .60) intensity = .80;
// 		else if (intensity > .45) intensity = .60;
// 		else if (intensity > .30) intensity = .45;
// 		else if (intensity > .15) intensity = .30;
// 		else intensity = 0;
// 		color = TGAColor(255, 255, 255) * intensity;
// 		return false; // do not discard pixel
// 	}
};


struct GouraudShader_Texture :public IShader {
	Vec3f varing_intensity;
	mat<2, 3, float> varing_uv;
	virtual Vec4f vertex(int iface, int nthvert) {
		//映射uv坐标
		varing_uv.set_col(nthvert, model->uv(iface, nthvert));
		varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);

		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));

		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;
		
		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		float intensity = varing_intensity * bar;
		Vec2f uv = varing_uv * bar;
		color = model->diffuse(uv) * intensity;
		return false;
	}

};


struct GouraudShader_Texture_DiffuseReflection :public IShader {
	
	mat<2, 3, float> varing_uv;
	mat<4, 4, float> uniform_M;//Projection*ModelView
	mat<4, 4, float> uniform_MIT; //Projection*ModelView 的逆运算
	virtual Vec4f vertex(int iface, int nthvert) {
		//映射uv坐标
		varing_uv.set_col(nthvert, model->uv(iface, nthvert));
		//varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);//光照随着法线变化,这里不再计算

		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));

		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;

		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		
		Vec2f uv = varing_uv * bar;

		
		//从法线贴图读取切线空间法线，变换到视图空间（使用逆转置矩阵保持垂直性）在齐次坐标下的归一化表示
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();

		//// 将世界空间光照方向变换到视图空间得到 齐次坐标下的light的归一化
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();


		//基于Lambert余弦定律计算漫反射强度
		//n·l  ====》 | n || l | cosθ（已归一化===》 | n |= | l |= 1）。
		float intensity = std::max(0.f,n*l);

		color = model->diffuse(uv) * intensity;
		return false;
	}

};


struct PhongShader :public IShader {

	mat<2, 3, float> varing_uv;
	mat<4, 4, float> uniform_M;//Projection*ModelView
	mat<4, 4, float> uniform_MIT; //Projection*ModelView 的逆运算
	virtual Vec4f vertex(int iface, int nthvert) {
		//映射uv坐标
		varing_uv.set_col(nthvert, model->uv(iface, nthvert));
		//varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);//光照随着法线变化,这里不再计算

		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));

		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;

		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {

		Vec2f uv = varing_uv * bar;

		//从法线贴图读取切线空间法线，变换到视图空间（使用逆转置矩阵保持垂直性）在齐次坐标下的归一化表示
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();

		//// 将世界空间光照方向变换到视图空间得到 齐次坐标下的light的归一化
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();


		//还需要考虑到反射光向量
		/*反射光向量推导
		*  https://straywriter.github.io/2021/03/18/%E8%AE%A1%E7%AE%97%E6%9C%BA%E5%9B%BE%E5%BD%A2%E5%AD%A6/Phong%E5%85%89%E7%85%A7%E6%A8%A1%E5%9E%8B/
		*/
		Vec3f r = (n * (n * l * 2.f) - l).normalize();

		
		//r.z分量表示反射光与视线的夹角余弦 ，若 < 0 ====》反射光方向与视线方向相反（无高光），形参2是从高光贴图中读取的值（0~255）
		//对应games101中，通过幂运算可以控制高光的范围
		float spec = pow(std::max(r.z, 0.f), model->specular(uv));
		//基于Lambert余弦定律计算漫反射强度
		//n·l  ====》 | n || l | cosθ（已归一化===》 | n |= | l |= 1）。
		float diff = std::max(0.f, n * l);
		TGAColor current_color = model->diffuse(uv);
		color = current_color;
		for (int i = 0; i < 3 ; ++i){
			//第一项常数项表示环境光（Ambient）
			//第二项漫反射强度*表示 当前从texture uv坐标系中读取到的颜色
			//第三项高光*系数
			color[i] = std::min<float>(ambient + current_color[i] * (diff + spec_coefficient * spec), 255);
		}
		return false;
	}

};





// void triangle_line(Vec3i t0, Vec3i t1, Vec3i t2, float ity0, float ity1, float ity2, TGAImage& image, int* zbuffer) {
// 	if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
// 	if (t0.y > t1.y) { std::swap(t0, t1); std::swap(ity0, ity1); }
// 	if (t0.y > t2.y) { std::swap(t0, t2); std::swap(ity0, ity2); }
// 	if (t1.y > t2.y) { std::swap(t1, t2); std::swap(ity1, ity2); }
// 
// 	int total_height = t2.y - t0.y;
// 	for (int i = 0; i < total_height; i++) {
// 		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
// 		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
// 		float alpha = (float)i / total_height;
// 		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
// 		Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
// 		Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
// 		float ityA = ity0 + (ity2 - ity0) * alpha;
// 		float ityB = second_half ? ity1 + (ity2 - ity1) * beta : ity0 + (ity1 - ity0) * beta;
// 		if (A.x > B.x) { std::swap(A, B); std::swap(ityA, ityB); }
// 		for (int j = A.x; j <= B.x; j++) {
// 			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (B.x - A.x);
// 			Vec3i    P = Vec3f(A) + Vec3f(B - A) * phi;
// 			float ityP = ityA + (ityB - ityA) * phi;
// 			int idx = P.x + P.y * width;
// 			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;
// 			if (zbuffer[idx] < P.z) {
// 				zbuffer[idx] = P.z;
// 				image.set(P.x, P.y, TGAColor(255, 255, 255) * ityP);
// 			}
// 		}
// 	}
// }





Vec4f world2screen(Vec3f v) {

	Vec4f gl_vertex = embed<4>(v);
	//把传入对象拉伸到n维，fill with 1
	return gl_vertex = ViewPort * Projection * ModelView * gl_vertex;
	
}


int main(int argc, char** argv) {
	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	viewtrans(camera, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(-1.f / (camera - center).norm());
	light_dir.normalize();
	PhongShader shader;

	//合并的模型视图投影矩阵（MVP），用于顶点变换
	shader.uniform_M = Projection * ModelView;
	//MVP的逆转置矩阵，用于正确变换法线
	shader.uniform_MIT = shader.uniform_M.invert_transpose();
	//GouraudShader shader;
	{ 


		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			
			Vec4f screen_coords[3];
			
			for (int j = 0; j < 3; j++) {
				
				screen_coords[j] = shader.vertex(i,j);
			}
			
			

			//triangle_line(screen_coords[0], screen_coords[1], screen_coords[2], intensity[0], intensity[1], intensity[2], image,zbuffer);
			
			triangle_box(screen_coords,shader, image,zbuffer);
			
			
		}
		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("PhongShader_AficaFace_withTexture.tga");
		zbuffer.flip_vertically();
		zbuffer.write_tga_file("zbuffer_PhongShader_AficaFace_withTexture.tga");
	}



	delete model;
	
	return 0;
}