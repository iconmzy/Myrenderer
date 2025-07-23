#include <cmath>
#include <limits>
#include <cstdlib>
#include "my_gl.h"

Matrix ModelView;
Matrix ViewPort;
Matrix Projection;

IShader::~IShader() {}



void projection(float coeff) {
	Projection = Matrix::identity();
	Projection[3][2] = coeff;
}

void viewtrans(Vec3f camera, Vec3f center, Vec3f up) {
	//�����������ͷ����ת����y������Ȼ����z��ı�׼����ϵ��Ȼ�����������任
	//����������͵õ�Ҫת��z������lookatdirection == camera-->center����λ��
	Vec3f z = (camera - center).normalize();
	//���ַ������ϵķ����lookatdirection�õ�x�᷽��
	Vec3f x = cross(up, z).normalize();
	//ͬ�������y������GitHub�İ����õ���z^x,���������ᾭ��ˮƽ��ת
	Vec3f y = cross(z, x).normalize();
	//����ϵ�غ���Ҫ������ת��ƽ�ƣ��Ѹ��Եķ����ӵ���ξ�����
	//mat�����matrixĬ����4ά������һ����λ����
	Matrix viewTrans = Matrix::identity();
	for (int i = 0; i < 3; ++i) {
		viewTrans[0][i] = x[i];
		viewTrans[1][i] = y[i];
		viewTrans[2][i] = z[i];
		//��������λ��
		viewTrans[i][3] = -center[i];
	}
	ModelView = viewTrans;
	return ;
}

void viewport(int x, int y, int w, int h) {
	ViewPort = Matrix::identity();
	ViewPort[0][3] = x + w / 2.f;
	ViewPort[1][3] = y + h / 2.f;
	ViewPort[2][3] = 255 / 2.f;

	ViewPort[0][0] = w / 2.f;
	ViewPort[1][1] = h / 2.f;
	ViewPort[2][2] = 255 / 2.f;
	return;
}


Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
	Vec3f s[2];
	for (int i = 2; i--; ) {
		s[i][0] = C[i] - A[i];
		s[i][1] = B[i] - A[i];
		s[i][2] = A[i] - P[i];
	}
	//[u,v,1]��[AB,AC,PA]��Ӧ��x��y��������ֱ�����Բ��
	Vec3f u = cross(s[0], s[1]);
	if (std::abs(u[2]) > 1e-2)
		//��1-u-v��u��vȫΪ����0��������ʾ�����������ڲ�
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	return Vec3f(-1, 1, 1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

//Vec4f* tri, TGAImage& image, TGAImage& zbuffer, TGAColor color
void triangle_box(Vec4f* tri, IShader& shader, TGAImage& image, TGAImage& zbuffer) {
	// �����Χ��
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			// x/w y/w
			bboxmin[j] = std::min(bboxmin[j], tri[i][j] / tri[i][3]);
			bboxmax[j] = std::max(bboxmax[j], tri[i][j] / tri[i][3]);
		}
	}


	Vec2i P;
	TGAColor color;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x += 1.0f) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y += 1.0f) {


			//�����proj��һ����ά������ģ��
			Vec3f bc_screen = barycentric(proj<2>(tri[0] / tri[0][3]), proj<2>(tri[1] / tri[1][3]),
				proj<2>(tri[2] / tri[2][3]), P);

			//��z��w�ĵ�����ֵ��Ϊ��͸��У����
			float z = tri[0][2] * bc_screen.x + tri[1][2] * bc_screen.y + tri[2][2] * bc_screen.z;
			float w = tri[0][3] * bc_screen.x + tri[1][3] * bc_screen.y + tri[2][3] * bc_screen.z;


			// ͸��У�� + ������ ��ͨ���Ҷ�0~255 
			int frag_depth = std::max(0, std::min(255, int(z / w + 0.5)));
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 || zbuffer.get(P.x, P.y)[0] > frag_depth) continue;

			bool discard = shader.fragment(bc_screen, color);
			if (!discard){
				//����color������&color��ÿ�����ʵʱ�仯
				image.set(P.x, P.y, color);
				//���������Ⱦzbuffer�����ȣ���ͨ���Ҷȼ���
				zbuffer.set(P.x, P.y, TGAColor(frag_depth));
			}



		}
	}
}