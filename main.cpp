#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const int width = 800;
const int height = 800;
const int depth = 255;

Model* model = NULL;
float* zbuffer = NULL;
Vec3f light_dir = Vec3f(1, -1, 1).normalize();
//cameraλ��
Vec3f camera(1, 1, 3);

//ԭ��/���㣿
Vec3f center(0, 0, 0);


Matrix viewtrans(Vec3f camera, Vec3f center, Vec3f up) {
	//�����������ͷ����ת����y������Ȼ����z��ı�׼����ϵ��Ȼ�����������任
	//����������͵õ�Ҫת��z������lookatdirection == camera-->center����λ��
	Vec3f z = (camera - center).normalize();
	//���ַ������ϵķ����lookatdirection�õ�x�᷽��
	Vec3f x = (up ^ z).normalize();
	//ͬ�������y������GitHub�İ����õ���z^x,���������ᾭ��ˮƽ��ת
	Vec3f y = (z ^ x).normalize();
	//����ϵ�غ���Ҫ������ת��ƽ�ƣ��Ѹ��Եķ����ӵ���ξ�����
	Matrix viewTrans = Matrix::identity(4);
	for (int i = 0;i < 3; ++i){
		viewTrans[0][i] = x[i];
		viewTrans[1][i] = y[i];
		viewTrans[2][i] = z[i];
		//��������λ��
		viewTrans[i][3] = -center[i];
	}
	return viewTrans;
}

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

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
	Vec3f z = (eye - center).normalize();
	Vec3f x = (up ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	Matrix res = Matrix::identity(4);
	for (int i = 0; i < 3; i++) {
		res[0][i] = x[i];
		res[1][i] = y[i];
		res[2][i] = z[i];
		res[i][3] = -center[i];
	}
	return res;
}

Vec3f barycentric_old(Vec3f* tri, Vec3f P) {
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

Vec3f barycentric(Vec3f* tri, Vec3f P) {
	Vec3f v0 = tri[1] - tri[0];
	Vec3f v1 = tri[2] - tri[0];
	Vec3f v2 = P - tri[0];

	float d00 = v0 * v0;
	float d01 = v0 * v1;
	float d11 = v1 * v1;
	float d20 = v2 * v0;
	float d21 = v2 * v1;
	float denom = d00 * d11 - d01 * d01;

	if (std::abs(denom) < 1e-5f) return Vec3f(-1, 1, 1); // ��С����ֵ

	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	return Vec3f(u, v, w);
}



void triangle_line(Vec3i t0, Vec3i t1, Vec3i t2, float ity0, float ity1, float ity2, TGAImage& image, int* zbuffer) {
	if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
	if (t0.y > t1.y) { std::swap(t0, t1); std::swap(ity0, ity1); }
	if (t0.y > t2.y) { std::swap(t0, t2); std::swap(ity0, ity2); }
	if (t1.y > t2.y) { std::swap(t1, t2); std::swap(ity1, ity2); }

	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
		Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
		Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
		float ityA = ity0 + (ity2 - ity0) * alpha;
		float ityB = second_half ? ity1 + (ity2 - ity1) * beta : ity0 + (ity1 - ity0) * beta;
		if (A.x > B.x) { std::swap(A, B); std::swap(ityA, ityB); }
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (B.x - A.x);
			Vec3i    P = Vec3f(A) + Vec3f(B - A) * phi;
			float ityP = ityA + (ityB - ityA) * phi;
			int idx = P.x + P.y * width;
			if (P.x >= width || P.y >= height || P.x < 0 || P.y < 0) continue;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;
				image.set(P.x, P.y, TGAColor(255, 255, 255) * ityP);
			}
		}
	}
}

void triangle_box(int iface, Vec3f* tri,  TGAImage& image, float intensity[3], float* zbuffer) {
	// �����Χ��
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

	for (int i = 0; i < 3; i++) {
		bboxmin.x = std::max(0.f, std::min(bboxmin.x, tri[i].x));
		bboxmin.y = std::max(0.f, std::min(bboxmin.y, tri[i].y));
		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, tri[i].x));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, tri[i].y));
	}
	// ��չ�������ȷ�����Ǳ�Ե
	bboxmin.x = std::floor(bboxmin.x - 0.5f);
	bboxmin.y = std::floor(bboxmin.y - 0.5f);
	bboxmax.x = std::ceil(bboxmax.x + 0.5f);
	bboxmax.y = std::ceil(bboxmax.y + 0.5f);
	// ��ȡ���������UV���꣨��һ����
	Vec2f uvs[3];
	for (int i = 0; i < 3; i++) {
		int uv_idx = model->faces_[iface][i][1];  // ֱ�ӷ���faces_��Ա
		uvs[i] = model->uv_[uv_idx];  // ��ȡ��һ��UV����
	}

	Vec3f P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x+=1.0f) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y+=1.0f) {
			Vec3f bc_screen = barycentric(tri, P);
			//if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;

			 // ʹ�ø����ɵı߽�����
			if (bc_screen.x < -0.001f || bc_screen.y < -0.001f || bc_screen.z < -0.001f) continue;
			
			 // ��ֵ��Ⱥ͹���
			P.z = tri[0].z * bc_screen.x + tri[1].z * bc_screen.y + tri[2].z * bc_screen.z;

			//�ѹ����Ƶ����Ǻ�����
			float ity = intensity[0] * bc_screen.x + intensity[1] * bc_screen.y + intensity[2] * bc_screen.z;
				
				

			// ��ֵUV���꣨��һ����
			Vec2f uv_pixel;
			for (int i = 0; i < 3; i++) {
				uv_pixel = uv_pixel + uvs[i] * bc_screen[i];
			}

			// ת��Ϊ������������
			Vec2i uv_pixel_coords(
				uv_pixel.x * model->diffusemap_.get_width(),
				uv_pixel.y * model->diffusemap_.get_height()
			);

			// ��Ȳ���
			int idx = int(P.x + P.y * image.get_width());
			if (idx < 0 || idx >= width * height) continue;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;

				// ��ȡ������ɫ��Ӧ�ù���ǿ��
				//TGAColor color = model->diffuse(uv_pixel_coords);


				image.set(P.x, P.y, TGAColor(255, 255, 255) * ity);
			}
		}
	}
}



int main(int argc, char** argv) {
	if (2 == argc) {
		model = new Model(argv[1]);
	}
	else {
		model = new Model("obj/african_head.obj");
	}

	zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());


	{ // draw the model
		Matrix ModelView = viewtrans(camera, center, Vec3f(0, 1, 0));
		
		Matrix ViewPort = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		Matrix Projection = Matrix::identity(4);
		Projection[3][2] = -1.f / (camera - center).norm();

// 		std::cerr << ModelView << std::endl;
// 		std::cerr << Projection << std::endl;
// 		std::cerr << ViewPort << std::endl;
// 		Matrix z = (ViewPort * Projection * ModelView);
// 		std::cerr << z << std::endl;

		TGAImage image(width, height, TGAImage::RGB);
		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			Vec3f screen_coords[3];
			Vec3f world_coords[3];
			float intensity[3];
			for (int j = 0; j < 3; j++) {
				Vec3f v = model->vert(face[j]);
				screen_coords[j] = Vec3f(ViewPort * Projection * ModelView * Matrix(v));
				world_coords[j] = v;
				intensity[j] = model->norm(i, j) * light_dir;
			}
			
			//triangle_line(screen_coords[0], screen_coords[1], screen_coords[2], intensity[0], intensity[1], intensity[2], image,zbuffer);
			triangle_box(i, screen_coords,  image, intensity,zbuffer);
		}
		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("drawMVPAficaFace_withTexture.tga");
	}

// 	{ // dump z-buffer (debugging purposes only)
// 		TGAImage zbimage(width, height, TGAImage::GRAYSCALE);
// 		for (int i = 0; i < width; i++) {
// 			for (int j = 0; j < height; j++) {
// 				zbimage.set(i, j, TGAColor(zbuffer[i + j * width]));
// 			}
// 		}
// 		zbimage.flip_vertically(); // i want to have the origin at the left bottom corner of the image
// 		zbimage.write_tga_file("zbuffer.tga");
// 	}

	delete model;
	delete[] zbuffer;
	return 0;
}