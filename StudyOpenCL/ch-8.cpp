#include "ch-8.h"
#include "ch-2.h"
#include <sstream>

using namespace cl;

void ch8_main(void) {
	BITMAPINFOHEADER h_info;
	BITMAPFILEHEADER hf;
	RGBQUAD hRGB[256];

	cl_int err_num;
	std::vector<Platform> platforms;
	Device device;
	Context context;
	Program program;
	Kernel kernel;

	err_num = Platform::get(&platforms);
	checkErr((err_num != CL_SUCCESS) ? err_num : (platforms.size() <= 0 ? -1 : CL_SUCCESS), "Platform::get");

	for (Platform platform : platforms) {
		std::vector<Device> devices;
		err_num = platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
		if (err_num != CL_SUCCESS && err_num == CL_DEVICE_NOT_FOUND) {
			checkErr(err_num, "platform.getDevices");
		}
		else if (devices.size() > 0) {
			device = devices[0];
			break;
		}
	}

	if (err_num != CL_SUCCESS || err_num == CL_DEVICE_NOT_FOUND) {
		cerr << "No CPU Device found" << endl;
		exit(EXIT_FAILURE);
	}

	context = Context(device, NULL, NULL, &err_num);
	CommandQueue queue(context,device,NULL,&err_num);
	checkErr(err_num, "CommanQueue()");
	Image2D test;

	try {
		test = createImage2D(context, "test.bmp", hf,h_info,hRGB);
	}
	catch(string error){
		cerr << error << endl;
		exit(EXIT_FAILURE);
	}
	ImageFormat format;
	format.image_channel_data_type = CL_UNORM_INT8;
	format.image_channel_order = CL_BGRA;
	Image2D out(context,CL_MEM_WRITE_ONLY, format, h_info.biWidth, h_info.biHeight,0,NULL, &err_num);
	if (err_num != CL_SUCCESS) {
		cerr << "Error creating Imaeg2D object" << endl;
		exit(EXIT_FAILURE);
	}

	Sampler sampler(context, CL_FALSE,CL_ADDRESS_CLAMP_TO_EDGE,CL_FILTER_LINEAR,&err_num);
	if (err_num != CL_SUCCESS) {
		cerr << "Error creating CL sampler object" << endl;
		exit(EXIT_FAILURE);
	}
	
	ifstream src_file("convolution.cl");
	checkErr(src_file.is_open() ? CL_SUCCESS : -1, "reading convolution.cl");

	string src_prog(istreambuf_iterator<char>(src_file), (istreambuf_iterator<char>()));
	const char* src = src_prog.c_str();

	program = Program(context, src, false, &err_num);
	checkErr(err_num, "Program()");
	err_num = program.build({ device });
	if (err_num != CL_SUCCESS) {
		cerr<< "Failed to build Program " + program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)<<endl;
	}
	kernel = Kernel(program, "gaussian_filter", &err_num);
	checkErr(err_num, "Kernel()");

	err_num = kernel.setArg(0, test);
	err_num |= kernel.setArg(1, out);
	err_num |= kernel.setArg(2, sampler);
	err_num |= kernel.setArg(3, h_info.biWidth);
	err_num |= kernel.setArg(4, h_info.biHeight);
	checkErr(err_num, "kerenl setArg");

	NDRange local_work_size(3, 3);
	NDRange global_work_size(h_info.biWidth, h_info.biHeight);
	err_num = queue.enqueueNDRangeKernel(kernel, NullRange, global_work_size, local_work_size, 0, NULL);
	/*switch (err_num) {
	case CL_INVALID_PROGRAM_EXECUTABLE:
		cerr << "a" << endl;
		break;
	case CL_INVALID_KERNEL:
		cerr << "b" << endl;
		break;
	case CL_INVALID_CONTEXT:
		cerr << "c" << endl;
		break;
	case CL_INVALID_KERNEL_ARGS:
		cerr << "d" << endl;
		break;
	case CL_INVALID_GLOBAL_WORK_SIZE:
		cerr << "e" << endl;
		break;
	case CL_INVALID_GLOBAL_OFFSET:
		cerr << "f" << endl;
		break;
	case CL_INVALID_WORK_GROUP_SIZE:
		cerr << "g" << endl;
		break;

	}*/

	BYTE* buffer = new BYTE[h_info.biWidth * h_info.biHeight * 4];
	cl::detail::size_t_array origin = { 0,0,0 };
	cl::detail::size_t_array region = { h_info.biWidth,h_info.biHeight,1 };

	err_num = queue.enqueueReadImage(out, CL_TRUE, origin,region,0,0,buffer,NULL,NULL);
	checkErr(err_num, "read image");

	BYTE* bgr_image = cvtBGRA2BGR(buffer, h_info.biWidth, h_info.biHeight);
	ofstream out_stream("gaussian.bmp",ios::binary);
	out_stream.write((char *)&hf, sizeof(BITMAPFILEHEADER));
	out_stream.write((char*)&h_info, sizeof(BITMAPINFOHEADER));
	out_stream.write((char*)hRGB, sizeof(RGBQUAD)*256);
	out_stream.write((char*)bgr_image, sizeof(BYTE)*3*h_info.biWidth*h_info.biHeight);
	out_stream.close();


	delete bgr_image;
	delete[] buffer;
}

Image2D createImage2D(const Context& context, const string image_path, BITMAPFILEHEADER& hf,
 BITMAPINFOHEADER& h_info, RGBQUAD*  hRGB) noexcept(false) {
	cl_int err_num;
	BYTE* image;

	try {
		image = read_bitmap(image_path, &hf, &h_info, hRGB);
	}
	catch (string error) {
		throw error;
	}
	
	BYTE* bgra_image = cvtBGR2BGRA(image, h_info.biWidth, h_info.biHeight);
	
	ImageFormat format;
	format.image_channel_order = CL_BGRA;
	format.image_channel_data_type = CL_UNORM_INT8;
	Image2D image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, format,
		h_info.biWidth, h_info.biHeight, 0, bgra_image, &err_num);

	if (err_num != CL_SUCCESS) {
		cout << "???" << endl;
		throw "Error creating CL image object";
	}

	delete image;
	delete bgra_image;

	return image2D;
}

BYTE* cvtBGR2BGRA(const BYTE* image, int width, int height) {
	BYTE* output = new BYTE[width * height * 4];
	for (int i = 0; i < width * height; i++) {
		output[4*i] = image[3*i];
		output[4*i + 1] = image[3 * i+1];
		output[4 * i + 2] = image[3 * i+2];
		output[4*i + 3] = 1;
	}
	return output;
}

BYTE* cvtBGRA2BGR(const BYTE* image, int width, int height) {
	BYTE* output = new BYTE[width * height * 3];
	for (int i = 0; i < width * height; i++) {
		output[3*i] = image[4*i];
		output[3*i + 1] = image[4*i + 1];
		output[3*i + 2] = image[4*i + 2];
	}
	return output;
}


BYTE* read_bitmap(const string file_name, BITMAPFILEHEADER* hf, BITMAPINFOHEADER* h_info, RGBQUAD* hRGB) noexcept(false) {
	ifstream imageFile(file_name, ios::binary);
	if (!imageFile.is_open()) {
		throw "Can't found image file";
	}

	imageFile.read((char*)hf, sizeof(BITMAPFILEHEADER));
	imageFile.read((char*)h_info, sizeof(BITMAPINFOHEADER));
	imageFile.read((char*)hRGB, sizeof(RGBQUAD) * 256);

	int w = h_info->biWidth;
	int h = h_info->biHeight;

	BYTE* image = new BYTE[w * h * 3];
	imageFile.read((char*)image, sizeof(BYTE) * w * h * 3);
	imageFile.close();

	return image;
}