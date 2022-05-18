#ifndef CH_8_HEADDER

#include <CL/cl2.hpp>
#include<Windows.h>
#include<iostream>
#include<sstream>
#include<fstream>

using namespace std;
using namespace cl;
void ch8_main(void);
Image2D createImage2D(const Context& context, const string image_path, BITMAPFILEHEADER& hf,
	BITMAPINFOHEADER& h_info, RGBQUAD* hRGB) noexcept(false);
BYTE* read_bitmap(const string file_name, BITMAPFILEHEADER* hf, BITMAPINFOHEADER* h_info, RGBQUAD* hRGB) noexcept(false);
BYTE* cvtBGR2BGRA(const BYTE* image, int width, int height);
BYTE* cvtBGRA2BGR(const BYTE* image, int width, int height);
#endif