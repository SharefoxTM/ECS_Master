/* Minimal TLS symbol stubs for single-threaded bare-metal build.
 * These satisfy picolibc runtime references when TLS is not used.
 */

#include <stdint.h>

/* Offset from thread pointer to TCB for ARM TLS ABI.
 * In single-threaded systems we can set this to 0.
 */
uintptr_t __arm32_tls_tcb_offset = 0;

/* Base address of the TLS data area. If not using TLS, set to 0. */
uintptr_t __tls_base = 0;
