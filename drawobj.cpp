
#include "tgaimage.h"
#include <iostream>;
#include "drawLine.h"
#include "model.h"

using namespace std;


int drawpbk_main() {



    //第三个形参声明为TGAImage::RGBA会支持Alpha通道，透明画布
    //声明为TGAImage::RGB则所有像素不透明
    //TGAImage image(200, 200, TGAImage::RGBA);
    //drawLineBresenham(X0, Y0, X1, Y1, image, pink);

//命令行控制方式和代码方式构造model
    //构造模型(obj文件路径)

    //构造tga(宽，高，指定颜色空间)


    const TGAColor white = TGAColor(255, 255, 255, 255);
    const TGAColor red = TGAColor(255, 0, 0, 255);
    const TGAColor pink = TGAColor(255, 192, 203, 255);
    //定义颜色

    Model* model1 = NULL;
    //定义宽度高度
    const int width = 800;
    const int height = 800;




    TGAImage image(width, height, TGAImage::RGBA);
    //模型的面作为循环控制变量
    for (int i = 0; i < model1->nfaces(); i++) {
        //创建face数组用于保存一个face的三个顶点坐标
        std::vector<int> face = model1->face(i);
        for (int j = 0; j < 3; j++) {
            //顶点v0
            Vec3f v0 = model1->vert(face[j]);
            //顶点v1
            Vec3f v1 = model1->vert(face[(j + 1) % 3]);
            //根据顶点v0和v1画线
            //先要进行模型坐标到屏幕坐标的转换
            //(-1,-1)对应(0,0)   (1,1)对应(width,height)
            int x0 = (v0.x + 1.) * width / 2.;
            int y0 = (v0.y + 1.) * height / 2.;
            int x1 = (v1.x + 1.) * width / 2.;
            int y1 = (v1.y + 1.) * height / 2.;
            //画线
            drawLineBresenham(x0, y0, x1, y1, image, white);
        }
    }

    //tga默认原点在左上角，现需要指定为左下角，所以进行竖直翻转
    image.flip_vertically();
    image.write_tga_file("drawface.tga");
    delete model1;
    return 0;
}