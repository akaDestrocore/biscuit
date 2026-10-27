/**
  * @file    host_mmap.h
  * @brief   OS memory reservation abstraction used by the flash simulator.
  */

#ifndef HOST_MMAP_H
#define HOST_MMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
  * @brief Reserves and commits a fixed-address memory region from the host OS.
  * @param pAddress Requested fixed virtual address.
  * @param size Region size in bytes.
  * @retval Pointer to mapped memory on success, NULL on failure.
  */
void *hostMmap_reserveFixed(void *pAddress, size_t size);

/**
  * @brief Releases a region previously reserved by `hostMmap_reserveFixed()`
  * @param pAddress Base address returned by hostMmap_reserveFixed().
  * @param size Region size in bytes.
  * @retval None
  */
void hostMmap_release(void *pAddress, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* HOST_MMAP_H */