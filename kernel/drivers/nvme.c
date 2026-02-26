#include "nvme.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "pci.h"
#include "validate.h"

static nvme_queue_t* nvme_queues = NULL;
static void* nvme_regs = NULL;
static spinlock_t nvme_lock = SPINLOCK_INIT;

void nvme_init(void) {
    /* Would detect NVMe controller via PCI */
    nvme_regs = (void*)0xFED80000; /* Placeholder */
    
    DEBUG_INFO("NVMe (Non-Volatile Memory Express) initialized");
}

int nvme_identify_controller(void* buffer) {
    if (!buffer) return -1;
    
    /* Would send identify command */
    memset(buffer, 0, 4096);
    
    DEBUG_INFO("NVMe controller identified");
    return 0;
}

nvme_queue_t* nvme_create_io_queue(u16 qid, u32 qsize) {
    nvme_queue_t* queue = (nvme_queue_t*)kzalloc(sizeof(nvme_queue_t));
    if (!queue) {
        DEBUG_ERROR("Failed to allocate NVMe queue");
        return NULL;
    }
    
    queue->qid = qid;
    queue->sq_size = qsize;
    queue->cq_size = qsize;
    
    size_t sq_size = qsize * sizeof(nvme_command_t);
    size_t cq_size = qsize * 16; /* Completion entry size */
    
    queue->sq_base = vmm_alloc_pages((sq_size + PAGE_SIZE - 1) / PAGE_SIZE);
    queue->cq_base = vmm_alloc_pages((cq_size + PAGE_SIZE - 1) / PAGE_SIZE);
    
    if (!queue->sq_base || !queue->cq_base) {
        if (queue->sq_base) vmm_free_pages(queue->sq_base, (sq_size + PAGE_SIZE - 1) / PAGE_SIZE);
        if (queue->cq_base) vmm_free_pages(queue->cq_base, (cq_size + PAGE_SIZE - 1) / PAGE_SIZE);
        kfree(queue);
        return NULL;
    }
    
    spinlock_lock(&nvme_lock);
    queue->next = nvme_queues;
    nvme_queues = queue;
    spinlock_unlock(&nvme_lock);
    
    DEBUG_INFO("NVMe I/O queue created: qid=%u, size=%u", qid, qsize);
    return queue;
}

int nvme_submit_command(nvme_queue_t* queue, nvme_command_t* cmd) {
    if (!queue || !cmd) return -1;
    
    /* Would submit to submission queue */
    DEBUG_INFO("NVMe command submitted: qid=%u, opcode=0x%x", queue->qid, cmd->opcode);
    return 0;
}

int nvme_read(nvme_queue_t* queue, u32 nsid, u64 lba, u32 count, void* buffer) {
    if (!queue || !buffer) return -1;
    
    nvme_command_t cmd = {0};
    cmd.opcode = 0x02; /* Read */
    cmd.nsid = nsid;
    cmd.prp1 = (u64)buffer;
    cmd.cdw10 = (u32)lba;
    cmd.cdw11 = (u32)(lba >> 32);
    cmd.cdw12 = (count - 1);
    
    return nvme_submit_command(queue, &cmd);
}

int nvme_write(nvme_queue_t* queue, u32 nsid, u64 lba, u32 count, const void* buffer) {
    if (!queue || !buffer) return -1;
    
    nvme_command_t cmd = {0};
    cmd.opcode = 0x01; /* Write */
    cmd.nsid = nsid;
    cmd.prp1 = (u64)buffer;
    cmd.cdw10 = (u32)lba;
    cmd.cdw11 = (u32)(lba >> 32);
    cmd.cdw12 = (count - 1);
    
    return nvme_submit_command(queue, &cmd);
}
