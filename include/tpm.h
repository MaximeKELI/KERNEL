#ifndef TPM_H
#define TPM_H

#include "types.h"

/* TPM commands */
#define TPM_CMD_GET_CAPABILITY  0x65
#define TPM_CMD_PCR_READ        0x15
#define TPM_CMD_PCR_EXTEND      0x14

/* Initialize TPM */
void tpm_init(void);

/* TPM read PCR */
int tpm_read_pcr(u32 pcr_index, u8* digest);

/* TPM extend PCR */
int tpm_extend_pcr(u32 pcr_index, const u8* digest);

/* TPM get random */
int tpm_get_random(u8* buffer, size_t size);

/* TPM seal data */
int tpm_seal_data(const u8* data, size_t data_size, u8* sealed, size_t* sealed_size);

/* TPM unseal data */
int tpm_unseal_data(const u8* sealed, size_t sealed_size, u8* data, size_t* data_size);

#endif /* TPM_H */
