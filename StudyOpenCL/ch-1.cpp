#include<CL/cl2.hpp>
#include<iostream>
#include<vector>
#include<fstream>
#include<sstream>

using namespace std;
using namespace cl;

Context create_context() noexcept(false);
CommandQueue create_command_queue(Context context, Device &device) noexcept(false);
Program create_program(Context context, Device device, const string file_name) noexcept(false);
bool create_buffer_objects(Context context, Buffer memObjects[3], float* a, float* b);

int main(void) {
	Context context;
	CommandQueue queue;
	Program program;
	Device device;
	Kernel kernel;
	cl_int errNum;
	Buffer memObjects[3];

	//초기 설정
	try {
		context = create_context();
		queue = create_command_queue(context, device);
		program = create_program(context, device, "hello.cl");
	}
	catch (string errMsg) {
		cerr << errMsg << endl;
		return -1;
	}

	kernel = Kernel(program, "hello", &errNum);
	if (errNum != CL_SUCCESS) {
		cerr << "Failed to create kernel" << endl;
		return -1;
	}

	float result[10];
	float a[10];
	float b[10];

	for (int i = 0; i < 10; i++) {
		a[i] = i;
		b[i] = i * 2;
	}

	if (!create_buffer_objects(context, memObjects, a, b)) {
		return -1;
	}

	errNum = kernel.setArg(0, memObjects[0]);
	errNum |= kernel.setArg(1, memObjects[1]);
	errNum |= kernel.setArg(2, memObjects[2]);

	if (errNum != CL_SUCCESS) {
		cerr << "Error setting kerenl args"<<endl;
		return -1;
	}
	
	errNum = queue.enqueueNDRangeKernel(kernel, NullRange, NDRange(10));
	if (errNum != CL_SUCCESS) {
		cerr << "Error queuing kernel for execution" << endl;
		return -1;
	}

	errNum = queue.enqueueReadBuffer(memObjects[2], CL_TRUE, 0,sizeof(float) * 10, result);
	if (errNum != CL_SUCCESS) {
		cerr << "Error reading result buffer" << endl;
		return -1;
	}

	for (int i = 0; i < 10; i++) {
		cout << result[i] << " ";
	}
	cout << endl;

	return 0;
}

Context create_context() noexcept(false){//첫 번째로 사용 가능한 플랫폼에서 생성
	cl_int errNum;
	Context context;
	std::vector<Platform> platforms;
	errNum = Platform::get(&platforms);

	if (errNum != CL_SUCCESS || platforms.size() == 0) {
		throw "Failed to find any OpenCL Platforms";
	}

	context = Context(CL_DEVICE_TYPE_GPU,NULL,NULL,&errNum);
	if (errNum != CL_SUCCESS) {
		throw "Could not create GPU context";
		
	}
	return context;
}

CommandQueue create_command_queue(Context context, Device &device) noexcept(false){
	cl_int errNum;
	CommandQueue queue;
	std::vector<Device> devices;
	string device_name, device_hardware_version, device_software_version, device_opencl_c_version;

	errNum = context.getInfo(CL_CONTEXT_DEVICES, &devices);
	if (errNum != CL_SUCCESS) {
		throw "Failed call to context getInfo";
	}
	if (devices.size() <= 0) {
		throw "No device available";
	}
	
	queue = CommandQueue::CommandQueue(context, devices[0],0,&errNum);
	if (errNum != CL_SUCCESS) {
		throw "Failed to create commandQueue for device 0";
	}
	device = devices[0];
	device.getInfo(CL_DEVICE_NAME, &device_name);
	device.getInfo(CL_DEVICE_VERSION, &device_hardware_version);
	device.getInfo(CL_DRIVER_VERSION, &device_software_version);
	device.getInfo(CL_DEVICE_OPENCL_C_VERSION, &device_opencl_c_version);
	cout << device_name << ":" << device_hardware_version << ":" << device_software_version << ":" << device_opencl_c_version << endl;

	return queue;
}

Program create_program(Context context, Device device, const string file_name) noexcept(false){
	cl_int errNum;
	Program program;
	
	ifstream kerenlFile(file_name, ios::in);
	if (!kerenlFile.is_open()) {
		throw "Failed to open file for reading " + file_name;
	}

	ostringstream oss;
	oss << kerenlFile.rdbuf();
	string srcStdStr = oss.str();
	program = Program(context, srcStdStr.c_str(),false,&errNum);

	if (errNum != CL_SUCCESS) {
		throw "Failed to create program";
	}

	program.build({ device });
	if (errNum != CL_SUCCESS) {
		throw "Failed to build Program " +  program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
	}

	return program;
}

bool create_buffer_objects(Context context, Buffer memObjects[3], float* a, float* b) {
	cl_int errNums[3];
	memObjects[0] = Buffer::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 10 * sizeof(float), a, &errNums[0]);
	memObjects[1] = Buffer::Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 10 * sizeof(float), b, &errNums[1]);
	memObjects[2] = Buffer::Buffer(context, CL_MEM_READ_WRITE, 10 * sizeof(float), NULL, &errNums[2]);

	if (errNums[0] != CL_SUCCESS || errNums[1] != CL_SUCCESS || errNums[2] != CL_SUCCESS) {
		cerr << "Error creating memory objects" << endl;
		return false;
	}
	return true;
}