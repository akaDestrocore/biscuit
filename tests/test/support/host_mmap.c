/**
  * @file    host_mmap.c
  * @brief   host_mmap.h implementation
  */
#include "host_mmap.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/mman.h>
#else
#error Unsupported host platform
#endif

/**
  * @brief Reserves and commits a fixed-address memory region from the host OS.
  * @param pAddress Requested fixed virtual address.
  * @param size Region size in bytes.
  * @retval Pointer to mapped memory on success, NULL on failure.
  */
void *hostMmap_reserveFixed(void *pAddress, size_t size) {

#if defined(_WIN32)

    return VirtualAlloc(pAddress, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#elif defined(__unix__) || defined(__APPLE__)

    void *pMap = mmap(pAddress, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    if (MAP_FAILED == pMap) {
        return NULL;
    }

    return pMap;

#endif
}

/**
  * @brief Releases a region previously reserved by hostMmap_reserveFixed().
  * @param pAddress Base address returned by hostMmap_reserveFixed().
  * @param size Region size in bytes.
  * @retval None
  */
void hostMmap_release(void *pAddress, size_t size) {

#if defined(_WIN32)

    (void)size;
    (void)VirtualFree(pAddress, 0U, MEM_RELEASE);

#elif defined(__unix__) || defined(__APPLE__)

    (void)munmap(pAddress, size);

#endif
}