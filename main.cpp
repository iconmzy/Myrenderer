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
//������ɫ

Model* model = NULL;
//�������S��ֵ
const int width = 500;
const int height = 500;
const int depth = 300;





//����MVP�任����
//Matrix ModelView[];

//Matrix ViewPort, mapping ��׼������ռ�(-1,1)^3 into �����screen space  [0,width]x[0,height]x[0,depth]
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
	//+0.5ʵ����������,�@��ԭʼ�汾�ѽ����O�ژ˜�׃�����
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}



int v0716_drawFullFace_main(int argc, char** argv) {



	//�������β�����ΪTGAImage::RGBA��֧��Alphaͨ����͸������
	//����ΪTGAImage::RGB���������ز�͸��
	//TGAImage image(200, 200, TGAImage::RGBA);



	/*
	//�����п��Ʒ�ʽ�ʹ��뷽ʽ����model
	//����ģ��(obj�ļ�·��)
	����ģ�͵�ÿ���������棨model->nfaces()��

		��ȡ�������ݲ�ת������Ļ�ռ�

		�������ǿ�ȣ�����������շ���ĵ����

		����泯��Դ��intensity > 0�������� triangle() �������Ƹ�������

	*/



	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}
	//����tga(���ߣ�ָ����ɫ�ռ�)
	constexpr int width = 900;
	constexpr int height = 900;

	TGAImage image(width, height, TGAImage::RGB);
	//�����ķ�������-z
	Vec3f light_dir(0, 0, -1);

	for (int i = 0; i < model->nfaces(); i++) {
		//����face�������ڱ���һ��face��������������
		vector<int> face = model->face(i);
		// �洢������ 3 ���������Ļ����
		Vec2i screen_coods[3];
		//// �洢������ 3 ���������������
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			//����⣬���vert������װ��prejection��viewport�任
			screen_coods[j] = Vec2i((v.x + 1.) * width / 2., (v.y + 1.) * height / 2.);
			world_coords[j] = v;
		}
		//���巨����n���������߲��һ����õ���ֱƽ���������Ȼ��λ��
		Vec3f n = (world_coords[2] - world_coords[0])^(world_coords[1] - world_coords[0]);
		n.normalize();
		//�򵥼���ĵ�˴����������ǲ����й�
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



	//�������β�����ΪTGAImage::RGBA��֧��Alphaͨ����͸������
	//����ΪTGAImage::RGB���������ز�͸��
	//TGAImage image(200, 200, TGAImage::RGBA);



	/*
	//�����п��Ʒ�ʽ�ʹ��뷽ʽ����model
	//����ģ��(obj�ļ�·��)
	����ģ�͵�ÿ���������棨model->nfaces()��

		��ȡ�������ݲ�ת������Ļ�ռ�

		�������ǿ�ȣ�����������շ���ĵ����

		����泯��Դ��intensity > 0�������� triangle() �������Ƹ�������

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
	//�����ķ�������-z
	Vec3f light_dir(0, 0, -1);
	




	for (int i = 0; i < model->nfaces(); i++) {
		//����face�������ڱ���һ��face��������������
		vector<int> face = model->face(i);
		// �洢������ 3 ���������Ļ����
		Vec3f screen_coods[3];
		//// �洢������ 3 ���������������
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			
			screen_coods[j] = world2screen(v);
			world_coords[j] = v;
		}
		//���巨����n���������߲��һ����õ���ֱƽ���������Ȼ��λ��
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		//�򵥼���ĵ�˴���������
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

