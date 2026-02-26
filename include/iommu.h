#ifndef IOMMU_H
#define IOMMU_H

#include "types.h"

/* IOMMU domain */
typedef struct iommu_domain {
    u32 domain_id;
    void* page_table;
    u64 iova_start;
    u64 iova_end;
    struct iommu_domain* next;
} iommu_domain_t;

/* Initialize IOMMU */
void iommu_init(void);

/* Create IOMMU domain */
iommu_domain_t* iommu_domain_alloc(void);

/* Map device */
int iommu_map(iommu_domain_t* domain, u64 iova, u64 paddr, size_t size, u32 flags);

/* Unmap device */
int iommu_unmap(iommu_domain_t* domain, u64 iova, size_t size);

/* Attach device to domain */
int iommu_attach_device(iommu_domain_t* domain, u32 device_id);

/* Detach device from domain */
int iommu_detach_device(iommu_domain_t* domain, u32 device_id);

#endif /* IOMMU_H */
