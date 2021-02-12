#define CL_TARGET_OPENCL_VERSION 220

#include <CL/cl.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROC(NAME, ...) { if (NAME(__VA_ARGS__)) errx(EXIT_FAILURE, #NAME " failed"); }

void read_source(const char *source_path, char **buffer, size_t *length) {
    FILE *file = fopen(source_path, "rb");
    long tmp;
    if (!file || fseek(file, 0, SEEK_END) || (tmp = ftell(file)) < 0) {
        err(EXIT_FAILURE, "cannot open file %s", source_path);
    }
    rewind(file);
    *length = tmp;
    *buffer = malloc(*length + 1); // adding newline at the end
    if (!*buffer || fread(*buffer, 1, *length, file) != *length) {
        err(EXIT_FAILURE, "cannot read from file %s", source_path);
    }
    (*buffer)[(*length)++] = '\n';

    fclose(file);
}

cl_program create_program(cl_context context, int source_path_count, ...) {
    char **sources = calloc(source_path_count, sizeof(char *));
    size_t *lengths = calloc(source_path_count, sizeof(size_t));

    va_list args;
    va_start(args, source_path_count);
    for (int i = 0; i < source_path_count; ++i) {
        char *source_path = va_arg(args, char*);
        read_source(source_path, &sources[i], &lengths[i]);
    }
    va_end(args);

    cl_program program = clCreateProgramWithSource(context, source_path_count, (const char **) sources, lengths, NULL);

    for (int i = 0; i<source_path_count; ++i) {
        free(sources[i]);
    }
    free(sources);
    free(lengths);

    return program;
}

cl_command_queue queue;
cl_kernel kernel;
cl_mem input_buffer;
cl_mem output_buffer;

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        errx(EXIT_FAILURE, "USAGE: %s platform device [ compiler_flags ]", argv[0]);
    }
    const int platform_index = atoi(argv[1]);
    const int device_index = atoi(argv[2]);
    const char* compiler_flags = argv[3];

    // 1. Get a platform.
    cl_platform_id platforms[4];
    unsigned num_platforms = 0;
    PROC(clGetPlatformIDs, 4, platforms, &num_platforms);
    if (platform_index < 1 || (unsigned) platform_index > num_platforms) {
        errx(EXIT_FAILURE, "ERROR: platform should be between 1 and %u, inclusive, got %d", num_platforms, platform_index);
    }
    cl_platform_id platform = platforms[platform_index - 1];

    // 2. Find a gpu device.
    cl_device_id devices[4];
    unsigned num_devices = 0;
    PROC(clGetDeviceIDs, platform, CL_DEVICE_TYPE_ALL, 4, devices, &num_devices);
    if (device_index < 1 || (unsigned) device_index > num_devices) {
        errx(EXIT_FAILURE, "ERROR: device should be between 1 and %u, inclusive, got %d", num_devices, device_index);
    }
    cl_device_id device = devices[device_index - 1];

    // 3. Create a context and command queue on that device.
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    queue = clCreateCommandQueue(context, device, 0, NULL);

    // 4. Perform runtime source compilation, and obtain kernel entry point.
    cl_program program = create_program(context, 1, "weird.cl");
    if (!program) {
        errx(EXIT_FAILURE, "create_program failed");
    }

    int err = clBuildProgram(program, 1, &device, compiler_flags, NULL, NULL);
    if (err) {
        // Determine the size of the log
        size_t log_size;
        PROC(clGetProgramBuildInfo, program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        PROC(clGetProgramBuildInfo, program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        printf("%s\n", log);
        return 1;
    }

    kernel = clCreateKernel(program, "weird", NULL);
    if (!kernel) {
        errx(EXIT_FAILURE, "clCreateKernel failed");
    }

    unsigned char output[32];
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof output, NULL, NULL);

    // 6. Launch the kernel.
    size_t global_work_size = 32;
    size_t local_work_size = 32;

    PROC(clSetKernelArg, kernel, 0, sizeof output_buffer, &output_buffer);
    PROC(clFinish, queue);

    PROC(clEnqueueNDRangeKernel, queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    PROC(clFinish, queue);

    PROC(clEnqueueReadBuffer, queue, output_buffer, 1, 0, sizeof output, output, 0, NULL, NULL);
    PROC(clFinish, queue);

    for (int i = 0; i < 32; ++i) {
        printf("%02x", output[i]);
    }
    putchar('\n');
}
