#include "kvm.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static kvm_t* kvm_list = NULL;
static spinlock_t kvm_lock = SPINLOCK_INIT;
static u64 next_vm_id = 1;
static u64 next_vcpu_id = 1;

void kvm_init(void) {
    DEBUG_INFO("KVM (Kernel Virtual Machine) initialized");
}

kvm_t* kvm_create_vm(void) {
    kvm_t* vm = (kvm_t*)kzalloc(sizeof(kvm_t));
    if (!vm) {
        DEBUG_ERROR("Failed to allocate VM");
        return NULL;
    }
    
    vm->vm_id = next_vm_id++;
    vm->memory_size = 256 * 1024 * 1024; /* 256MB */
    vm->memory = vmm_alloc_pages((vm->memory_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!vm->memory) {
        kfree(vm);
        return NULL;
    }
    
    spinlock_lock(&kvm_lock);
    vm->next = kvm_list;
    kvm_list = vm;
    spinlock_unlock(&kvm_lock);
    
    DEBUG_INFO("VM created: id=%u", (u32)vm->vm_id);
    return vm;
}

kvm_vcpu_t* kvm_create_vcpu(kvm_t* vm) {
    if (!vm) return NULL;
    
    kvm_vcpu_t* vcpu = (kvm_vcpu_t*)kzalloc(sizeof(kvm_vcpu_t));
    if (!vcpu) {
        DEBUG_ERROR("Failed to allocate VCPU");
        return NULL;
    }
    
    vcpu->vcpu_id = next_vcpu_id++;
    vcpu->regs = kzalloc(512); /* Register state */
    vcpu->sregs = kzalloc(256); /* Segment registers */
    vcpu->running = false;
    
    vcpu->next = vm->vcpus;
    vm->vcpus = vcpu;
    
    DEBUG_INFO("VCPU created: id=%u for VM %u", (u32)vcpu->vcpu_id, (u32)vm->vm_id);
    return vcpu;
}

int kvm_run_vcpu(kvm_vcpu_t* vcpu) {
    if (!vcpu) return -1;
    
    vcpu->running = true;
    /* Would execute VM instructions here */
    DEBUG_INFO("VCPU %u running", (u32)vcpu->vcpu_id);
    return 0;
}

void kvm_destroy_vm(kvm_t* vm) {
    if (!vm) return;
    
    /* Free VCPUs */
    kvm_vcpu_t* vcpu = vm->vcpus;
    while (vcpu) {
        kvm_vcpu_t* next = vcpu->next;
        kfree(vcpu->regs);
        kfree(vcpu->sregs);
        kfree(vcpu);
        vcpu = next;
    }
    
    /* Free memory */
    if (vm->memory) {
        vmm_free_pages(vm->memory, (vm->memory_size + PAGE_SIZE - 1) / PAGE_SIZE);
    }
    
    spinlock_lock(&kvm_lock);
    kvm_t** prev = &kvm_list;
    kvm_t* v = kvm_list;
    while (v) {
        if (v == vm) {
            *prev = v->next;
            break;
        }
        prev = &v->next;
        v = v->next;
    }
    spinlock_unlock(&kvm_lock);
    
    kfree(vm);
    DEBUG_INFO("VM destroyed");
}
