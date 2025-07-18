
#include "tgaimage.h"
#include <iostream>;
#include "drawLine.h"
#include "model.h"

using namespace std;


int drawpbk_main() {



    //�������β�����ΪTGAImage::RGBA��֧��Alphaͨ����͸������
    //����ΪTGAImage::RGB���������ز�͸��
    //TGAImage image(200, 200, TGAImage::RGBA);
    //drawLineBresenham(X0, Y0, X1, Y1, image, pink);

//�����п��Ʒ�ʽ�ʹ��뷽ʽ����model
    //����ģ��(obj�ļ�·��)

    //����tga(���ߣ�ָ����ɫ�ռ�)


    const TGAColor white = TGAColor(255, 255, 255, 255);
    const TGAColor red = TGAColor(255, 0, 0, 255);
    const TGAColor pink = TGAColor(255, 192, 203, 255);
    //������ɫ

    Model* model1 = NULL;
    //�����ȸ߶�
    const int width = 800;
    const int height = 800;




    TGAImage image(width, height, TGAImage::RGBA);
    //ģ�͵�����Ϊѭ�����Ʊ���
    for (int i = 0; i < model1->nfaces(); i++) {
        //����face�������ڱ���һ��face��������������
        std::vector<int> face = model1->face(i);
        for (int j = 0; j < 3; j++) {
            //����v0
            Vec3f v0 = model1->vert(face[j]);
            //����v1
            Vec3f v1 = model1->vert(face[(j + 1) % 3]);
            //���ݶ���v0��v1����
            //��Ҫ����ģ�����굽��Ļ�����ת��
            //(-1,-1)��Ӧ(0,0)   (1,1)��Ӧ(width,height)
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;
            //����
            drawLineBresenham(x0, y0, x1, y1, image, white);
        }
    }

    //tgaĬ��ԭ�������Ͻǣ�����Ҫָ��Ϊ���½ǣ����Խ�����ֱ��ת
    image.flip_vertically();
    image.write_tga_file("drawface.tga");
    delete model1;
    return 0;
}