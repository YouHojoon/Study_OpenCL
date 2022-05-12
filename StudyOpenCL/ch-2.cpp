#include<CL/cl2.hpp>
#include<iostream>
#include<vector>
#include<fstream>
#include<sstream>
using namespace cl;
using namespace std;

const unsigned int INPUT_SIGNAL_WIDTH = 8;
const unsigned int INPUT_SIGNAL_HEIGHT = 8;
const unsigned int OUTPUT_SIGNAL_WIDTH = 6;
const unsigned int OUTPUT_SIGNAL_HEIGHT = 6;
const unsigned int MASK_WIDTH = 3;
const unsigned int MASK_HEIGHT = 3;

cl_uint input[INPUT_SIGNAL_WIDTH][INPUT_SIGNAL_HEIGHT] = {
		{3,1,1,4,8,2,1,3},
		{4,2,1,1,2,1,2,3},
		{4,4,4,4,3,2,2,2},
		{9,8,3,8,9,0,0,0},
		{9,3,3,9,0,0,0,0},
		{0,9,0,8,0,0,0,0},
		{3,0,8,8,9,4,4,4},
		{5,9,8,1,8,1,1,1}
};
cl_uint output_signal[OUTPUT_SIGNAL_WIDTH][OUTPUT_SIGNAL_HEIGHT];
cl_uint mask[MASK_WIDTH][MASK_HEIGHT] = {
	{1,1,1},{1,0,1},{1,1,1}
};
void displayInfo() {
	cl_int errNum;
	std::vector<Platform> platforms;
	errNum = Platform::get(&platforms);

	if (errNum != CL_SUCCESS || platforms.size() <= 0) {
		cerr << "Failed to find any opencl platforms" << endl;
		return;
	}

	cout << "Number of platforms " << platforms.size() << endl;
	for (Platform platform : platforms) {
		string platform_profile, platform_version, platform_vendor;

		errNum = platform.getInfo(CL_PLATFORM_PROFILE, &platform_profile);
		errNum |= platform.getInfo(CL_PLATFORM_VERSION, &platform_version);
		errNum |= platform.getInfo(CL_PLATFORM_VENDOR, &platform_vendor);

		if (errNum != CL_SUCCESS) {
			cerr << "Failed to get info about platform" << endl;
			return;
		}

		cout << platform_profile << ":" << platform_version << ":" << platform_vendor << endl;
	}
}

void convolve() {
	
	

}

inline void checkErr(cl_int err, const string name) {
	if (err != CL_SUCCESS) {
		cerr << "ERROR: " << name << " " << err << endl;
		exit(EXIT_FAILURE);
	}
}

void CL_CALLBACK context_callBack(const char *errInfo, const void *private_info, size_t cb, void* user_data) {
	cout << "Error occurred during context use: " << errInfo << endl;
	exit(EXIT_FAILURE);
}

void ch2_main(void) {
	displayInfo();

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

	context = Context(device, NULL,&context_callBack, &err_num);
	checkErr(err_num, "Context()");

	ifstream src_file("convolution.cl");
	checkErr(src_file.is_open() ? CL_SUCCESS : -1, "reading convolution.cl");

	string src_prog(istreambuf_iterator<char>(src_file), (istreambuf_iterator<char>()));
	const char* src = src_prog.c_str();

	program = Program(context, src, false, &err_num);
	checkErr(err_num, "Program()");
	err_num = program.build({ device });
	kernel = Kernel(program, "convolve",&err_num);
	checkErr(err_num, "Kernel()");

	Buffer input_signal_buffer = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * INPUT_SIGNAL_WIDTH * INPUT_SIGNAL_HEIGHT
	,input, &err_num);
	checkErr(err_num, "Buffer(intput)");
	Buffer mask_buffer = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint) * MASK_WIDTH * MASK_HEIGHT
		, mask, &err_num);
	checkErr(err_num, "Buffer(mask)");
	Buffer output_signal_buffer = Buffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint) * OUTPUT_SIGNAL_WIDTH * OUTPUT_SIGNAL_HEIGHT
		, NULL, &err_num);
	checkErr(err_num, "Buffer(output)");
	

	CommandQueue queue = CommandQueue(context, device, 0, &err_num);
	checkErr(err_num, "CommandQueue()");

	err_num = kernel.setArg(0, input_signal_buffer);
	err_num |= kernel.setArg(1, mask_buffer);
	err_num |= kernel.setArg(2, output_signal_buffer);
	err_num |= kernel.setArg(3, INPUT_SIGNAL_WIDTH);
	err_num |= kernel.setArg(4, MASK_WIDTH);
	checkErr(err_num, "kernel.setArg");

	err_num = queue.enqueueNDRangeKernel(kernel,NullRange,NDRange(OUTPUT_SIGNAL_WIDTH, OUTPUT_SIGNAL_HEIGHT),NullRange);
	checkErr(err_num, "queue.enqueueNDRangeKernel");

	err_num = queue.enqueueReadBuffer(output_signal_buffer, CL_TRUE, 0, sizeof(cl_uint) * OUTPUT_SIGNAL_HEIGHT * OUTPUT_SIGNAL_WIDTH, output_signal);
	checkErr(err_num, "enqueueReadBuffer");

	for (int i = 0; i < OUTPUT_SIGNAL_HEIGHT; i++) {
		for (int j = 0; j < OUTPUT_SIGNAL_WIDTH; j++) {
			cout << output_signal[i][j] << " ";
		}
		cout << endl;
	}
}