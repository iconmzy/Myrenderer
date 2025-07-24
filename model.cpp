#include <iostream>
#include <fstream>
#include <sstream>

#include "model.h"

Model::Model(const char* filename) : verts_(), faces_(), norms_(), uv_()
{
	std::ifstream in(filename, std::ifstream::in);
	if (in.is_open()) {
		std::string line;
		while (std::getline(in, line))
		{
			char trash;
			std::istringstream iss(line);
			if (line.substr(0, 2) == "v ") {
				iss >> trash;
				Vec3f v;
				for (int i = 0; i < 3; i++) iss >> v[i];
				verts_.push_back(v);
			}
			else if (line.substr(0, 2) == "vn") {
				iss >> trash >> trash;
				Vec3f n;
				for (int i = 0; i < 3; i++) iss >> n[i];
				norms_.push_back(n);
			}
			else if (line.substr(0, 2) == "vt") {
				iss >> trash >> trash;
				Vec2f uv;
				for (int i = 0; i < 2; i++) iss >> uv[i];
				uv_.push_back(uv);
			}
			else if (line.substr(0, 2) == "f ") {
				std::vector<Vec3i> f;
				Vec3i tmp;
				iss >> trash;
				while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
					for (int i = 0; i < 3; i++) tmp[i]--; // in wavefront obj all indices start at 1, not zero
					f.push_back(tmp);
				}
				faces_.push_back(f);
			}
		}
		in.close();
	}

	std::cerr << "# v#" << verts_.size() << " f# " << faces_.size() << " vt# " <<
		uv_.size() << " vn# " << norms_.size() << std::endl;
	//在构造函数中就把texture加载了
	std::cout << "loading texture" << std::endl;
	load_texture(filename, DIFFUSE);
	load_texture(filename, NORMAL);
	//帶刀疤的高光貼圖texture 
	load_texture(filename, SPECULAR);
}

Model::~Model() {}

int Model::nverts() {
	return (int)verts_.size();
}

int Model::nfaces() {
	return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
	std::vector<int> face;
	for (int i = 0; i < (int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
	return face;
}

void Model::load_texture(std::string filename, TextureType type) {
	std::string textfile(filename);
	std::string suffix;
	size_t dot = textfile.find_last_of(".");
	if (dot != std::string::npos) {
		switch (type) {
		case DIFFUSE:    suffix = "_diffuse.tga"; break;
		case NORMAL:     suffix = "_nm.tga";      break;
		case SPECULAR:   suffix = "_spec.tga";    break;

		}
		textfile = textfile.substr(0, dot) + (suffix);
		std::cout << "textfile file" << textfile << "loading " <<
			(textures_[type].read_tga_file(textfile.c_str()) ? "ok" : "failed") << std::endl;
		textures_[type].flip_vertically();
	}
}

TGAColor Model::diffuse(Vec2f uvf) {
	Vec2i uv(uvf[0] * textures_[DIFFUSE].get_width(), uvf[1] * textures_[DIFFUSE].get_height());
	return textures_[DIFFUSE].get(uv[0], uv[1]);
}

Vec3f Model::vert(int iface, int nthvert) {
	return verts_[faces_[iface][nthvert][0]];
}

Vec2f Model::uv(int iface, int nthvert) {
	return uv_[faces_[iface][nthvert][1]];
}

Vec3f Model::normal(int iface, int nthvert) {
	int idx = faces_[iface][nthvert][2];
	return norms_[idx].normalize();
}

Vec3f Model::normal(Vec2f uvf) {
	Vec2i uv(uvf[0] * textures_[NORMAL].get_width(), uvf[1] * textures_[NORMAL].get_height());
	TGAColor c = textures_[NORMAL].get(uv[0], uv[1]);
	Vec3f res;
	// notice TGAColor is bgra, and in byte
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f * 2.f - 1.f;
	return res;
}

float Model::specular(Vec2f uvf) {

	//將紋理坐標（0-1範圍）轉換為紋理圖像的具體像素坐標（整數）
	Vec2i uv(uvf[0] * textures_[SPECULAR].get_width(), uvf[1] * textures_[SPECULAR].get_height());


	//get返回一个rgb值，任选一个通道，然后转换成float
	return textures_[SPECULAR].get(uv[0], uv[1])[0] / 1.f;
}