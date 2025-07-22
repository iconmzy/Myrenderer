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

Model* model_mzy = NULL;
//�������S��ֵ
const int width = 800;
const int height = 800;
const int depth = 255;




//��Դ�Ƕ��������λ��
const Vec3f light_dir(0.2, 0.14, -1);
const Vec3f camera(0, 0, 3);



//����MVP�任����


//
Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);

	/*
   +--------------------------------+
   |    |x|     
   |    |y|     
   |    |z|     
   |    |w|    
	�ڸ��������ϳ���m����4d���������ϵӳ�����ά�ռ�
   +--------------------------------+
	*/
}

//4d ---> 3d w��������1��ʾ�� ����
// Matrix v2m(Vec3f v) {
// 	Matrix m(4, 1);
// 	m[0][0] = v.x;
// 	m[1][0] = v.y;
// 	m[2][0] = v.z;
// 	m[3][0] = 1.f;
// 	return m;
// }



//Matrix ViewPort, mapping ��׼������ռ�(-1,1)^3 into �����screen space  [0,width]x[0,height]x[0,depth]
Matrix viewport_mzy(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	//���������ƽ����
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;

	//���������������
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
	//[u,v,1]��[AB,AC,PA]��Ӧ��x��y��������ֱ�����Բ��
	Vec3f u = s[0] ^ s[1];
	//���㹲��ʱ���ᵼ��u[2]Ϊ0����ʱ����(-1,1,1)
	if (std::abs(u[2]) > 1e-2)
		//��1-u-v��u��vȫΪ����0��������ʾ�����������ڲ�
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1);

}




void triangle(int iface, Vec3f* tri, float* zbuffer, TGAImage& image, TGAColor color) {
	// �����Χ��
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			bboxmin[j] = std::max(0.f, std::min(bboxmin[j], tri[i][j]));
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], tri[i][j]));
		}
	}

	// ��ȡ���������UV���꣨��һ����
	Vec2f uvs[3];
	for (int i = 0; i < 3; i++) {
		int uv_idx = model_mzy->faces_[iface][i][1];  // ֱ�ӷ���faces_��Ա
		uvs[i] = model_mzy->uv_[uv_idx];  // ��ȡ��һ��UV����
	}

	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
			Vec3f bc_screen = barycentric_mzy(tri, P);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

			// ��ֵ���
			P.z = 0;
			for (int i = 0; i < 3; i++) P.z += tri[i][2] * bc_screen[i];

			// ��ֵUV���꣨��һ����
			Vec2f uv_pixel;
			for (int i = 0; i < 3; i++) {
				uv_pixel = uv_pixel + uvs[i] * bc_screen[i];
			}

			// ת��Ϊ������������
			Vec2i uv_pixel_coords(
				uv_pixel.x * model_mzy->diffusemap_.get_width(),
				uv_pixel.y * model_mzy->diffusemap_.get_height()
			);

			// ��Ȳ���
			int idx = int(P.x + P.y * image.get_width());
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;

				// ��ȡ������ɫ��Ӧ�ù���ǿ��
				TGAColor color = model_mzy->diffuse(uv_pixel_coords);


				image.set(P.x, P.y, color);
			}
		}
	}
}






int main_oldversion(int argc, char** argv) {



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


		/*��ʼ����Ŀ����ӳ�䵽���·�Χ
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
		//����face�������ڱ���һ��face��������������
		vector<int> face = model_mzy->face(i);
		// �洢������ 3 ���������Ļ����
		Vec3f screen_coods[3];
		//// �洢������ 3 ���������������
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model_mzy->vert(face[j]);
			
			//screen_coods[j] = world2screen(v);
			screen_coods[j] = m2v(ViewPort * Projection * v2m(v));
			world_coords[j] = v;

		}
		//���巨����n���������߲��һ����õ���ֱƽ���������Ȼ��λ��
		Vec3f n = cross((world_coords[2] - world_coords[0]), (world_coords[1] - world_coords[0]));
		n.normalize();
		//�򵥼���ĵ�˴���������
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

