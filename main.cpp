#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const int width = 600;
const int height = 600;
const int depth = 255;

Model* model = NULL;

Vec3f light_dir = Vec3f(0, -1, -1).normalize();
//cameraλ��
Vec3f camera(-1, -1, 2);

//ԭ��/���㣿
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);




Matrix ModelView;
Matrix ViewPort;
Matrix Projection;







Matrix viewtrans(Vec3f camera, Vec3f center, Vec3f up) {
	//�����������ͷ����ת����y������Ȼ����z��ı�׼����ϵ��Ȼ�����������任
	//����������͵õ�Ҫת��z������lookatdirection == camera-->center����λ��
	Vec3f z = (camera - center).normalize();
	//���ַ������ϵķ����lookatdirection�õ�x�᷽��
	Vec3f x = cross(up,z).normalize();
	//ͬ�������y������GitHub�İ����õ���z^x,���������ᾭ��ˮƽ��ת
	Vec3f y = cross(z,x).normalize();
	//����ϵ�غ���Ҫ������ת��ƽ�ƣ��Ѹ��Եķ����ӵ���ξ�����
	//mat�����matrixĬ����4ά������һ����λ����
	Matrix viewTrans = Matrix::identity();
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
	Matrix m = Matrix::identity();
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
	Vec3f x = cross(up, z).normalize();
	Vec3f y = cross(z , x).normalize();
	Matrix res = Matrix::identity();
	for (int i = 0; i < 3; i++) {
		res[0][i] = x[i];
		res[1][i] = y[i];
		res[2][i] = z[i];
		res[i][3] = -center[i];
	}
	return res;
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

void triangle_box(Vec4f* tri,  TGAImage& image, TGAImage & zbuffer, TGAColor color) {
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

// 	// ��ȡ���������UV���꣨��һ����
// 	Vec2f uvs[3];
// 	for (int i = 0; i < 3; i++) {
// 		int uv_idx = model->faces_[iface][i][1];  // ֱ�ӷ���faces_��Ա
// 		uvs[i] = model->uv_[uv_idx];  // ��ȡ��һ��UV����
// 	}

	Vec2i P;
	for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x+=1.0f) {
		for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y+=1.0f) {


			//�����proj��һ����ά������ģ��
			Vec3f bc_screen = barycentric(proj<2>(tri[0] / tri[0][3]), proj<2>(tri[1] / tri[1][3]),
				proj<2>(tri[2] / tri[2][3]), P);
			
			//��z��w�ĵ�����ֵ��Ϊ��͸��У����
			float z = tri[0][2] * bc_screen.x + tri[1][2] * bc_screen.y + tri[2][2] * bc_screen.z;
			float w = tri[0][3] * bc_screen.x + tri[1][3] * bc_screen.y + tri[2][3] * bc_screen.z;
			

			// ͸��У�� + ������ ��ͨ���Ҷ�0~255 
			int frag_depth = std::max(0, std::min(255, int(z / w + 0.5)));
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 || zbuffer.get(P.x, P.y)[0] > frag_depth) continue;


			
			image.set(P.x, P.y, color);
			//���������Ⱦzbuffer������
			zbuffer.set(P.x, P.y, TGAColor(frag_depth));

				
				

			// ��ֵUV���꣨��һ����
// 			Vec2f uv_pixel;
// 			for (int i = 0; i < 3; i++) {
// 				uv_pixel = uv_pixel + uvs[i] * bc_screen[i];
// 			}
// 
// 			// ת��Ϊ������������
// 			Vec2i uv_pixel_coords(
// 				uv_pixel.x * model->diffusemap_.get_width(),
// 				uv_pixel.y * model->diffusemap_.get_height()
// 			);
		}
	}
}



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
	}
	TGAImage image(width, height, TGAImage::RGB);
	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
	//zbuffer = new float[width * height];
	//for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());


	{ // draw the model
		ModelView = viewtrans(camera, center, up);
		
		ViewPort = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
		Projection = Matrix::identity();
		Projection[3][2] = -1.f / 3;

// 		std::cerr << ModelView << std::endl;
// 		std::cerr << Projection << std::endl;
// 		std::cerr << ViewPort << std::endl;
// 		Matrix z = (ViewPort * Projection * ModelView);
// 		std::cerr << z << std::endl;

		
		for (int i = 0; i < model->nfaces(); i++) {
			std::vector<int> face = model->face(i);
			//�ĳ��������
			Vec4f screen_coords[3];
			Vec3f world_coords[3];
			
			for (int j = 0; j < 3; j++) {
				
				world_coords[j] = model->vert(face[j]);;
				screen_coords[j] = world2screen(world_coords[j]);
			}
			//����㷨��
			Vec3f norm = cross(world_coords[2] - world_coords[0], world_coords[1] - world_coords[0]);
			norm.normalize();
			float intensity = light_dir * norm;
			//triangle_line(screen_coords[0], screen_coords[1], screen_coords[2], intensity[0], intensity[1], intensity[2], image,zbuffer);
			if (intensity > 0){
				triangle_box(screen_coords, image,zbuffer, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
			}
			
		}
		image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
		image.write_tga_file("drawMVPAficaFace_withoutTexture.tga");
		zbuffer.flip_vertically();
		zbuffer.write_tga_file("zbuffer.tga");
	}



	delete model;
	
	return 0;
}