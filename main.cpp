#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "my_gl.h"


//�궨��߽�����
#define CLAMP(t) ((t>1.f)?1.f:((t<0.f)?0.f:t))



const int width = 800;
const int height = 800;
//const int depth = 255;

//һЩ����ϵ����
float  ambient = 18.;
float  shadowlimit_ambient = 3.0;
float shadowfactor = 0.8;
float spec_coefficient = 0.6;

Model* model = NULL;
float* shadowbuffer = NULL;
float* zbuffer = NULL;


//camera location
Vec3f light_dir(1, 1, 1);
Vec3f       camera(1, 1, 9);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);




struct FlatShader: public IShader{
	//NXN�����¼ת������ĵ�
	mat<3, 3, float> varing_tri;  
	/*
	ÿ�д���һ������ (��3������)
	ÿ�д���һ���������(x, y, z)

	  varying_tri = [ x0, x1, x2 ]  �� ��0�У����ж����x����
					[ y0, y1, y2 ]  �� ��1�У����ж����y����
					[ z0, z1, z2 ]  �� ��2�У����ж����z����
	*/
	virtual Vec4f vertex(int iface, int nthvert) {
		////��ȡ�� iface ����/�����εĵ�nthvert������ĳ��������ת��Ϊ�������ϵ
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));
		gl_vertex = Projection * ModelView * gl_vertex;
		//����ͶӰ�仯�Ķ�������/z������͸�ӳ��������xzy������ӵ�varing_tri�Ĳ�ͬ��
		varing_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));
		gl_vertex = ViewPort * gl_vertex;
		return gl_vertex;


	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		//���� ���������ߵĲ��
		Vec3f n = cross(varing_tri.col(1) - varing_tri.col(0), varing_tri.col(2) - varing_tri.col(0)).normalize();
		//����жϹ����Ƿ�ƽ��������
		float intensity = CLAMP(n * light_dir);
		color = TGAColor(255, 255, 255) * intensity;
		return false;
	}
};



struct GouraudShader :public IShader {
	Vec3f varing_intensity;

	virtual Vec4f vertex(int iface, int nthvert) {
		////��ȡ�� iface ����/�����εĵ�nthvert������ĳ��������ת��Ϊ�������ϵ
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));


		//Gouraud Shader ֻ����㶥���������Ļ���꣬����Ҫ�� Flat Shader �����洢ͶӰ���3D���ꡣ
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
		//ӳ��uv����
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
	mat<4, 4, float> uniform_MIT; //Projection*ModelView ��������
	virtual Vec4f vertex(int iface, int nthvert) {
		//ӳ��uv����
		varing_uv.set_col(nthvert, model->uv(iface, nthvert));
		//varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);//�������ŷ��߱仯,���ﲻ�ټ���

		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));

		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;

		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {
		
		Vec2f uv = varing_uv * bar;

		
		//�ӷ�����ͼ��ȡ���߿ռ䷨�ߣ��任����ͼ�ռ䣨ʹ����ת�þ��󱣳ִ�ֱ�ԣ�����������µĹ�һ����ʾ
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();

		//// ������ռ���շ���任����ͼ�ռ�õ� ��������µ�light�Ĺ�һ��
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();


		//����Lambert���Ҷ��ɼ���������ǿ��
		//n��l  ====�� | n || l | cos�ȣ��ѹ�һ��===�� | n |= | l |= 1����
		float intensity = std::max(0.f,n*l);

		color = model->diffuse(uv) * intensity;
		return false;
	}

};


struct PhongShader :public IShader {

	mat<2, 3, float> varing_uv;
	mat<4, 4, float> uniform_M;//Projection*ModelView
	mat<4, 4, float> uniform_MIT; //Projection*ModelView ��������
	virtual Vec4f vertex(int iface, int nthvert) {
		//ӳ��uv����
		varing_uv.set_col(nthvert, model->uv(iface, nthvert));
		//varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);//�������ŷ��߱仯,���ﲻ�ټ���

		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));

		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;

		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {

		Vec2f uv = varing_uv * bar;

		//�ӷ�����ͼ��ȡ���߿ռ䷨�ߣ��任����ͼ�ռ䣨ʹ����ת�þ��󱣳ִ�ֱ�ԣ�����������µĹ�һ����ʾ
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();

		//// ������ռ���շ���任����ͼ�ռ�õ� ��������µ�light�Ĺ�һ��
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();


		//����Ҫ���ǵ����������
		/*����������Ƶ�
		*  https://straywriter.github.io/2021/03/18/%E8%AE%A1%E7%AE%97%E6%9C%BA%E5%9B%BE%E5%BD%A2%E5%AD%A6/Phong%E5%85%89%E7%85%A7%E6%A8%A1%E5%9E%8B/
		*/
		Vec3f r = (n * (n * l * 2.f) - l).normalize();

		
		//r.z������ʾ����������ߵļн����� ���� < 0 ====������ⷽ�������߷����෴���޸߹⣩���β�2�ǴӸ߹���ͼ�ж�ȡ��ֵ��0~255��
		//��Ӧgames101�У�ͨ����������Կ��Ƹ߹�ķ�Χ
		float spec = pow(std::max(r.z, 0.f), model->specular(uv));
		//����Lambert���Ҷ��ɼ���������ǿ��
		//n��l  ====�� | n || l | cos�ȣ��ѹ�һ��===�� | n |= | l |= 1����
		float diff = std::max(0.f, n * l);
		TGAColor current_color = model->diffuse(uv);
		color = current_color;
		for (int i = 0; i < 3 ; ++i){
			//��һ������ʾ�����⣨Ambient��
			//�ڶ���������ǿ��*��ʾ ��ǰ��texture uv����ϵ�ж�ȡ������ɫ
			//������߹�*ϵ��
			color[i] = std::min<float>(ambient + current_color[i] * (diff + spec_coefficient * spec), 255);
		}
		return false;
	}

};




struct PhongShaderWithShadow :public IShader {

	mat<3, 3, float> vari_tri; // triangle coordinates before Viewport transform, written by VS, read by FS
	mat<2, 3, float> vari_uv;
	mat<4, 4, float> uniform_M;//Projection*ModelView
	mat<4, 4, float> uniform_MIT; //Projection*ModelView ��������

	mat<4, 4, float> uniform_Mshadow; // transform framebuffer screen coordinates to shadowbuffer screen coordinates

	
	PhongShaderWithShadow() = default;


	//ֱ��ͨ�����캯����ʼ��
	PhongShaderWithShadow(Matrix M, Matrix MIT, Matrix MS) : uniform_M(M), uniform_MIT(MIT), uniform_Mshadow(MS), vari_uv(), vari_tri() {}


	virtual Vec4f vertex(int iface, int nthvert) {
		//ӳ��uv����
		vari_uv.set_col(nthvert, model->uv(iface, nthvert));
		//varing_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir);//�������ŷ��߱仯,���ﲻ�ټ���

		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert));

		gl_vertex = ViewPort * Projection * ModelView * gl_vertex;

		return gl_vertex;
	}
	virtual bool fragment(Vec3f bar, TGAColor& color) {

		//����ǰƬԪ����Ļ�ռ�����ת������Դ�ӽǵ���Ӱ��ͼ�ռ�����
		Vec4f sb_p = uniform_Mshadow * embed<4>(vari_tri * bar);
		//�������ϵ====>�ռ�����
		sb_p = sb_p / sb_p[3];


		//index in shadow buffer array
		int idx = int(sb_p[0]) + int(sb_p[1]) * width;


		//ϵ����
		//float shadow = ambient*0.2 + shadowfactor * (shadowbuffer[idx] < sb_p[2]); // magic coeff to avoid z-fighting
		// �Ľ�����Ӱ���㲿�ֿ��Ը�Ϊ��
		float shadow = 1.0;
		float bias = 0.005; // �ʵ���ƫ��ֵ

		//��Χ 3��3 ���� �ڣ������� 9 ���㣨x �� y �� -1 �� 1��
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				float pcfDepth = shadowbuffer[idx + x + y * width];
				shadow += (sb_p[2] - bias > pcfDepth) ? shadowfactor : 0.0;
			}
		}
		shadow /= 9.0; // 3x3 PCF�������ٳ���9
		


		Vec2f uv = vari_uv * bar;

		//�ӷ�����ͼ��ȡ���߿ռ䷨�ߣ��任����ͼ�ռ䣨ʹ����ת�þ��󱣳ִ�ֱ�ԣ�����������µĹ�һ����ʾ
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();

		//// ������ռ���շ���任����ͼ�ռ�õ� ��������µ�light�Ĺ�һ��
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();


		//

		//����Ҫ���ǵ����������
		/*����������Ƶ�
		*  https://straywriter.github.io/2021/03/18/%E8%AE%A1%E7%AE%97%E6%9C%BA%E5%9B%BE%E5%BD%A2%E5%AD%A6/Phong%E5%85%89%E7%85%A7%E6%A8%A1%E5%9E%8B/
		*/
		Vec3f r = (n * (n * l * 2.f) - l).normalize();


		//r.z������ʾ����������ߵļн����� ���� < 0 ====������ⷽ�������߷����෴���޸߹⣩���β�2�ǴӸ߹���ͼ�ж�ȡ��ֵ��0~255��
		//��Ӧgames101�У�ͨ����������Կ��Ƹ߹�ķ�Χ
		float spec = pow(std::max(r.z, 0.f), model->specular(uv));
		//����Lambert���Ҷ��ɼ���������ǿ��
		//n��l  ====�� | n || l | cos�ȣ��ѹ�һ��===�� | n |= | l |= 1����
		float diff = std::max(0.f, n * l);
		TGAColor current_color = model->diffuse(uv);
		color = current_color;
		for (int i = 0; i < 3; ++i) {
			//��һ������ʾ�����⣨Ambient��
			//�ڶ���������ǿ��*��ʾ ��ǰ��texture uv����ϵ�ж�ȡ������ɫ
			//������߹�*ϵ��
			color[i] = std::min<float>(ambient + current_color[i] *  shadow *  (diff + spec_coefficient * spec), 255);
		}
		return false;
	}

};





struct DepthShader : public IShader {
	mat<3, 3, float> vari_tri;
	DepthShader(): vari_tri(){}


	//��Ȼ����Ĺ�ʽһ����������viewtrans��ʱ��ѡ�����㲻ͬ
	virtual Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert));
		gl_Vertex = ViewPort * Projection * ModelView * gl_Vertex;          // transform it to screen coordinates
		vari_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color) {
		Vec3f p = vari_tri* bar;
		color = TGAColor(255, 255, 255) * (p.z / depth);
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
	//�Ѵ���������쵽nά��fill with 1
	return gl_vertex = ViewPort * Projection * ModelView * gl_vertex;
	
}


int main(int argc, char** argv) {
	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
		
		//model = new Model("obj/diablo3_pose.obj");
		
	}


	//try to add eye balls  with multi obj file
// 	std::vector<Model*> models;
// 	models.push_back(new Model("obj/african_head.obj"));
// 	models.push_back(new Model("obj/african_head_eye_inner.obj"));



	zbuffer = new float[width * height];
	shadowbuffer = new float[width * height];
	//�������ȳ�ʼ����Сֵ
	for (int i = width * height; --i; ) {
		zbuffer[i] = shadowbuffer[i] = -std::numeric_limits<float>::max();
	}

	light_dir.normalize();

	
	


	{
		//�Ȱ�����Ӱ��ϵ��һ��������
		TGAImage shadow_depth(width, height, TGAImage::RGB);
		//�������Թ�ԴΪ��������ӽǱ仯�õ�Զ����ϵ
		viewtrans(light_dir, center, up);
		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

		//// ����͸��ͶӰ����0�����ȼ�������ͶӰ����ƽ�й⣩
		projection(0);
		DepthShader shadowshader;
		Vec4f screen_coords[3];
		for (int i = 0; i < model->nfaces(); i++) {
			for (int j = 0; j < 3; j++) {
				screen_coords[j] = shadowshader.vertex(i, j);
			}
			triangle(screen_coords, shadowshader, shadow_depth, shadowbuffer);
		}
		shadow_depth.flip_vertically(); // to place the origin in the bottom left corner of the image
		shadow_depth.write_tga_file("Nigger_shadow_depth_ADDeye.tga");

	}
	
	//����shadows��mvp����
	Matrix MVP_shadow = ViewPort * Projection * ModelView;



	{ 
		//�ڶ�����Ⱦ����Ӱ�Ļ�������ɫ,�������Ϊ�ӽǱ仯ԭ��
		TGAImage frame(width, height, TGAImage::RGB);
		viewtrans(camera, center, up);
		viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		projection(-1.f / (camera - center).norm());

		PhongShaderWithShadow shader(ModelView, (Projection * ModelView).invert_transpose(), MVP_shadow * (ViewPort * Projection * ModelView).invert());
		
		//�ϲ���ģ����ͼͶӰ����MVP�������ڶ���任
		//shader.uniform_M = Projection * ModelView;
		//MVP����ת�þ���������ȷ�任����
		//shader.uniform_MIT = shader.uniform_M.invert_transpose();
		//GouraudShader shader;


		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			
			Vec4f screen_coords[3];
			
			for (int j = 0; j < 3; j++) {
				
				screen_coords[j] = shader.vertex(i,j);
			}
			
			//triangle_line(screen_coords[0], screen_coords[1], screen_coords[2], intensity[0], intensity[1], intensity[2], image,zbuffer);
			triangle(screen_coords,shader, frame,zbuffer);
			
		}
		frame.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		frame.write_tga_file("ADDeye_Nigger_withShadw.tga");

	}



	delete model;
	
	return 0;
}