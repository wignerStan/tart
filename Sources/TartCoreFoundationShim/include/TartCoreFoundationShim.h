#ifndef TART_CORE_FOUNDATION_SHIM_H
#define TART_CORE_FOUNDATION_SHIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
void *tart_vmnet_network_import_plist(const void *bytes, size_t length, int *status);

void tart_cf_release(const void *object);

#ifdef __cplusplus
}
#endif

#endif
