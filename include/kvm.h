#ifndef KVM_H
#define KVM_H

#include "types.h"

/* KVM (Kernel-based Virtual Machine) support */

/* Virtual CPU */
typedef struct kvm_vcpu {
    u64 vcpu_id;
    void* regs;
    void* sregs;
    bool running;
    struct kvm_vcpu* next;
} kvm_vcpu_t;

/* Virtual machine */
typedef struct kvm {
    u64 vm_id;
    kvm_vcpu_t* vcpus;
    void* memory;
    u64 memory_size;
    struct kvm* next;
} kvm_t;

/* Initialize KVM */
void kvm_init(void);

/* Create VM */
kvm_t* kvm_create_vm(void);

/* Create VCPU */
kvm_vcpu_t* kvm_create_vcpu(kvm_t* vm);

/* Run VCPU */
int kvm_run_vcpu(kvm_vcpu_t* vcpu);

/* Destroy VM */
void kvm_destroy_vm(kvm_t* vm);

#endif /* KVM_H */
