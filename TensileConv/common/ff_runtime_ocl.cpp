#include <iostream> 
#include <fstream>
#include <sstream>

#include "ff_runtime_ocl.h"
#include "ff_log.h"

using namespace feifei;

RuntimeOCL * RuntimeOCL::pInstance = nullptr;
RuntimeOCL * RuntimeOCL::GetInstance()
{
	if (pInstance == nullptr)
	{
		pInstance = new RuntimeOCL();

		if (pInstance->initPlatform() != E_ReturnState::SUCCESS)
		{
			pInstance = nullptr;
		}
	}

	return pInstance;
}
RuntimeOCL * RuntimeOCL::GetInstance(cl_platform_id platformId, cl_context context, cl_device_id deviceId)
{
	if (pInstance == nullptr)
	{
		pInstance = new RuntimeOCL();

		if (pInstance->initPlatform(platformId, context, deviceId) != E_ReturnState::SUCCESS)
		{
			pInstance = nullptr;
		}
	}

	return pInstance;
}

E_ReturnState RuntimeOCL::initPlatform()
{
	cl_int errNum;
	cl_uint pltNum;

	// platform
	errNum = clGetPlatformIDs(0, NULL, &pltNum);
	clCheckErr(errNum);
	if (pltNum == 0)
	{
		FATAL("no opencl platform support.");
	}

	errNum = clGetPlatformIDs(1, &platformId, NULL);
	clCheckErr(errNum);
	getPlatformInfo();

	// context
	cl_context_properties cps[3] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)platformId,
		0
	};
	context = clCreateContextFromType(cps, CL_DEVICE_TYPE_GPU, NULL, NULL, &errNum);
	clCheckErr(errNum);

	// devices
	uint deviceCnt;
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCnt);
	clCheckErr(errNum);
	if (deviceCnt == 0)
	{
		FATAL("no opencl device support.");
	}

	cl_device_id deviceIds[deviceCnt];
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, deviceCnt, deviceIds, NULL);
	clCheckErr(errNum);

	for (int i = 0; i < deviceCnt; i++)
	{
		DeviceOCL *dev = new DeviceOCL(deviceIds[i]);
		devices.push_back(dev);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState RuntimeOCL::initPlatform(cl_platform_id platformId, cl_context context, cl_device_id deviceId)
{
	cl_int errNum;
	cl_uint pltNum;

	// platform
	this->platformId = platformId;
	getPlatformInfo();

	// context
	this->context = context;

	// devices
	uint deviceCnt;
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCnt);
	clCheckErr(errNum);
	if (deviceCnt == 0)
	{
		FATAL("no opencl device support.");
	}

	cl_device_id deviceIds[deviceCnt];
	errNum = clGetDeviceIDs(platformId, CL_DEVICE_TYPE_GPU, deviceCnt, deviceIds, NULL);
	clCheckErr(errNum);

	for (int i = 0; i < deviceCnt; i++)
	{
		DeviceOCL *dev = new DeviceOCL(deviceIds[i]);
		devices.push_back(dev);
	}

	// sellect device
	int devCnt = 0;
	for (devCnt = 0; devCnt < devices.size(); devCnt++)
	{
		if (devices[devCnt]->DeviceId() == deviceId)
		{
			selDevice = devices[devCnt];
			break;
		}
	}
	if (devCnt >= devices.size())
	{
		ERR("Can't find device %d.", deviceId);
	}

	return E_ReturnState::SUCCESS;
}
void RuntimeOCL::getPlatformInfo()
{
	cl_int errNum;
	size_t size;
	char * tmpChar;

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, size, tmpChar, NULL);
	platformInfo.version = std::string(tmpChar);

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_NAME, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_NAME, size, tmpChar, NULL);
	platformInfo.name = std::string(tmpChar);

	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetPlatformInfo(platformId, CL_PLATFORM_VENDOR, size, tmpChar, NULL);
	platformInfo.vendor = std::string(tmpChar);
}
void DeviceOCL::getDeviceInfo()
{
	cl_int errNum;
	size_t size;
	char * tmpChar;

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_TYPE, sizeof(cl_uint), &deviceInfo.type, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR_ID, sizeof(cl_uint), &deviceInfo.vendorId, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &deviceInfo.cuNum, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &deviceInfo.freqMHz, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cl_uint), &deviceInfo.cacheLineB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &deviceInfo.cacheSizeB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.glbMemSizeB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &deviceInfo.ldsMemSizeB, NULL);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platformId, NULL);


	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_NAME, size, tmpChar, NULL);
	deviceInfo.name = std::string(tmpChar);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VENDOR, size, tmpChar, NULL);
	deviceInfo.vendor = std::string(tmpChar);

	errNum = clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, size, tmpChar, NULL);
	deviceInfo.drvVersion = std::string(tmpChar);

	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, 0, NULL, &size);
	tmpChar = (char*)alloca(size);
	errNum = clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, size, tmpChar, NULL);
	deviceInfo.clVersion = std::string(tmpChar);
}

CmdQueueOCL * RuntimeOCL::CreatCmdQueue(bool enProf, int devNum)
{	
	DeviceOCL * dev;
	if (devNum < 0)
	{
		dev = selDevice;
	}
	else
	{
		if (devNum >= DevicesCnt())
			return nullptr;

		dev = devices[devNum];
	}
	
	CmdQueueOCL * q = new CmdQueueOCL(dev);
	if (q->CreatQueue(&context, enProf) == E_ReturnState::SUCCESS)
	{
		dev->AddCmdQueue(q);
		return q;
	}
	else
	{
		return nullptr;
	}
}
E_ReturnState CmdQueueOCL::CreatQueue(cl_context *ctx, bool enProf)
{
	cl_int errNum;

	if (enProf == true)
	{
		prop |= CL_QUEUE_PROFILING_ENABLE;
	}
	cmdQueue = clCreateCommandQueue(*ctx, device->DeviceId(), prop, &errNum);
	clCheckErr(errNum);

	return E_ReturnState::SUCCESS;
}

KernelOCL * RuntimeOCL::CreatKernel(char * content, std::string kernelName, E_ProgramType type, int devNum)
{
	DeviceOCL * dev;
	if (devNum < 0)
	{
		dev = selDevice;
	}
	else
	{
		if (devNum >= DevicesCnt())
			return nullptr;

		dev = devices[devNum];
	}

	KernelOCL * k = new KernelOCL(content, kernelName, type, dev);

	if (k->CreatKernel(&context) == E_ReturnState::SUCCESS)
	{
		dev->AddKernel(k);
		return k;
	}
	else
	{
		return nullptr;
	}
}
E_ReturnState KernelOCL::CreatKernel(cl_context *ctx)
{
	switch (programType)
	{
	case PRO_OCL_FILE:		return creatKernelFromOclFile(ctx);
	case PRO_OCL_STRING:	return creatKernelFromOclString(ctx);
	case PRO_GAS_FILE:		return creatKernelFromGasFile(ctx);
	case PRO_GAS_STRING:	return creatKernelFromGasString(ctx);
	case PRO_BIN_FILE:		return creatKernelFromBinFile(ctx);
	case PRO_BIN_STRING:	return creatKernelFromBinString(ctx);
	default: ERR("not support program type");
	}
	return E_ReturnState::FAIL;
}
E_ReturnState KernelOCL::creatKernelFromOclString(cl_context *ctx)
{
	cl_int errNum;

	program = clCreateProgramWithSource(*ctx, 1, (const char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + programFile);
	}

	if (buildKernel() != E_ReturnState::SUCCESS)
		return E_ReturnState::FAIL;
	if (dumpKernel() != E_ReturnState::SUCCESS)
		return E_ReturnState::FAIL;

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelOCL::creatKernelFromOclFile(cl_context *ctx)
{
	cl_int errNum;

	std::ifstream kernelFile(programFile, std::ios::in);
	if (!kernelFile.is_open())
	{
		ERR("Failed to open file for reading: " + programFile);
	}
	std::ostringstream oss;
	oss << kernelFile.rdbuf();
	std::string str = oss.str();
	programSrc = (char *)(str.c_str());

	return creatKernelFromOclString(ctx);
}
E_ReturnState KernelOCL::creatKernelFromBinString(cl_context *ctx)
{
	cl_int errNum;

	program = clCreateProgramWithBinary(*ctx, 1, device->pDeviceId(), &programSize, (const unsigned char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + programFile);
	}

	return buildKernel();
}
E_ReturnState KernelOCL::creatKernelFromBinFile(cl_context *ctx)
{
	cl_int errNum;

	FILE * fp = fopen(programFile.c_str(), "rb");
	if (fp == NULL)
	{
		ERR("can't open bin file: " + programFile);
	}

	fseek(fp, 0, SEEK_END);
	programSize = ftell(fp);
	rewind(fp);

	programSrc = (char *)malloc(programSize);
	fread(programSrc, 1, programSize, fp);
	fclose(fp);

	return creatKernelFromBinString(ctx);
}
E_ReturnState KernelOCL::creatKernelFromGasFile(cl_context *ctx)
{
	std::string compiler = RuntimeOCL::GetInstance()->Compiler();
	if (kernelFile == "")
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + get_file_name(programFile) + ".bin";

	if (device->DeviceInfo()->name == "gfx900")
		BuildOption = "-x assembler -target amdgcn--amdhsa -mcpu=gfx900 ";
	else if (device->DeviceInfo()->name == "gfx803")
		BuildOption = "-x assembler -target amdgcn--amdhsa -mcpu=gfx803 ";
	else
		ERR("not support hardware.");

	std::string cmd = compiler + " " + BuildOption + "-o " + kernelFile + " " + programFile;
	exec_cmd(cmd);
	//INFO(cmd);

	// creatKernelFromBinString()
	FILE * fp = fopen(kernelFile.c_str(), "rb");
	if (fp == NULL)
	{
		ERR("can't open bin file: " + kernelFile);
	}
	fseek(fp, 0, SEEK_END);
	programSize = ftell(fp);
	rewind(fp);
	programSrc = (char *)malloc(programSize);
	fread(programSrc, 1, programSize, fp);
	fclose(fp);

	// creatKernelFromBinString()
	cl_int errNum;
	program = clCreateProgramWithBinary(*ctx, 1, device->pDeviceId(), &programSize, (const unsigned char**)&programSrc, NULL, &errNum);
	if ((program == NULL) || (errNum != CL_SUCCESS))
	{
		ERR("Failed to create CL program from " + programFile);
	}

	return buildKernel();
}
E_ReturnState KernelOCL::creatKernelFromGasString(cl_context *ctx)
{
	programFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".s";
	kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".bin";

	std::ofstream fout(programFile.c_str(), std::ios::out);
	if (!fout.is_open())
	{
		ERR("can't open save file: " + programFile);
	}
	fout.write(kernelFile.c_str(), sizeof(*programSrc)/sizeof(char));
	fout.close();

	return creatKernelFromGasFile(ctx);
}
E_ReturnState KernelOCL::buildKernel()
{
	cl_int errNum;
	size_t size;
	char * tmpLog;

	errNum = clBuildProgram(program, 1, device->pDeviceId(), "", NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		ERR("Failed to build CL program from " + programFile);
	}

	clGetProgramBuildInfo(program, device->DeviceId(), CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
	tmpLog = (char*)alloca(size);
	clGetProgramBuildInfo(program, device->DeviceId(), CL_PROGRAM_BUILD_LOG, size, tmpLog, NULL);
	//INFO("building log: " + std::string(tmpLog));

	kernel = clCreateKernel(program, kernelName.c_str(), &errNum);
	if ((errNum != CL_SUCCESS)||(kernel == NULL))
	{
		ERR("Failed to create kernel " + kernelName);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelOCL::dumpKernel()
{
	if (programFile == "")
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".bin";
	}
	else
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + get_file_name(programFile) + ".bin";
	}
	size_t binary_size;
	clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, nullptr);
	std::vector<char> binary(binary_size);
	char* src[1] = { binary.data() };
	clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(src), &src, nullptr);

	std::ofstream fout(kernelFile.c_str(), std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		ERR("can't open save file: " + kernelFile);
	}
	fout.write(binary.data(), binary.size());
	fout.close();

	return E_ReturnState::SUCCESS;
}
E_ReturnState KernelOCL::dumpProgram()
{
	if (programFile == "")
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + kernelName + ".cl";
	}
	else
	{
		kernelFile = RuntimeOCL::GetInstance()->KernelTempDir() + "/" + get_file_name(programFile) + ".cl";
	}

	size_t src_size;
	clGetProgramInfo(program, CL_PROGRAM_SOURCE, sizeof(size_t), &src_size, nullptr);
	char clProgram[src_size];
	clGetProgramInfo(program, CL_PROGRAM_SOURCE, sizeof(clProgram), &clProgram, nullptr);

	std::ofstream fout(kernelFile.c_str(), std::ios::out);
	if (!fout.is_open())
	{
		ERR("can't open save file: " + kernelFile);
	}
	fout.write(clProgram, src_size);
	fout.close();

	return E_ReturnState::SUCCESS;
}
E_ReturnState CmdQueueOCL::Launch(KernelOCL *k, dim3 global_sz, dim3 group_sz, cl_event * evt_creat)
{
	cl_int errNum;

	if (k->DeviceId() != this->DeviceId())
	{
		WARN("kernel device not match command queue device.");
	}

	errNum = clEnqueueNDRangeKernel(cmdQueue, k->Kernel(), 3, NULL, global_sz.arr(), group_sz.arr(), 0, NULL, evt_creat);
	if(errNum != CL_SUCCESS)
	{
		clErrInfo(errNum);
		ERR("Failed launch kernel: " + std::string(clGetErrorInfo(errNum)));
	}

	return E_ReturnState::SUCCESS;
}
double RuntimeOCL::GetProfilingTime(cl_event * evt)
{
	if (evt == NULL)
	{
		WARN("no profiling event.");
		return 0.0;
	}

	cl_ulong start_time, end_time;
	clGetEventProfilingInfo(*evt, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);
	clGetEventProfilingInfo(*evt, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);

	return (end_time - start_time) * 1e-9;
}

cl_mem RuntimeOCL::DevMalloc(size_t byteNum)
{
	cl_int errNum;
	cl_mem d_mem;

	d_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, byteNum, NULL, &errNum);
	if ((errNum != CL_SUCCESS) || (d_mem == nullptr))
	{
		clErrInfo(errNum);
		d_mem = nullptr;
	}
	return d_mem;
}
E_ReturnState CmdQueueOCL::MemCopyH2D(cl_mem d_mem, void * h_mem, size_t byteNum)
{
	cl_int errNum;

	errNum = clEnqueueWriteBuffer(cmdQueue, d_mem, CL_TRUE, 0, byteNum, h_mem, 0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		clErrInfo(errNum);
		ERR("Failed to copy memory to device %d Byte", byteNum);
	}

	return E_ReturnState::SUCCESS;
}
E_ReturnState CmdQueueOCL::MemCopyD2H(void * h_mem, cl_mem d_mem, size_t byteNum)
{
	cl_int errNum;

	errNum = clEnqueueReadBuffer(cmdQueue, d_mem, CL_TRUE, 0, byteNum, h_mem, 0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		clErrInfo(errNum);
		ERR("Failed to copy memory to device %d Byte", byteNum);
	}

	return E_ReturnState::SUCCESS;
}
