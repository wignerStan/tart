#include <CoreFoundation/CoreFoundation.h>

#include "TartCoreFoundationShim.h"

void tart_cf_release(const void *object) {
  CFRelease((CFTypeRef)object);
}

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000
#include <vmnet/vmnet.h>
#include <xpc/xpc.h>
void *tart_vmnet_network_import_plist(const void *bytes, size_t length, int *status) {
  *status = VMNET_INVALID_ARGUMENT;
  if (__builtin_available(macOS 26.0, *)) {
    CFDataRef data = CFDataCreate(NULL, bytes, length);
    CFPropertyListRef plist = CFPropertyListCreateWithData(NULL, data, kCFPropertyListImmutable, NULL, NULL);
    CFRelease(data);
    if (!plist || CFGetTypeID(plist) != CFDictionaryGetTypeID()) { if (plist) CFRelease(plist); return NULL; }
    CFIndex count = CFDictionaryGetCount(plist);
    if (count < 1 || count > 32) { CFRelease(plist); return NULL; }
    const void *keys[32], *values[32];
    CFDictionaryGetKeysAndValues(plist, keys, values);
    xpc_object_t object = xpc_dictionary_create(NULL, NULL, 0);
    for (CFIndex i=0; i<count; i++) {
      char key[256];
      if (CFGetTypeID(keys[i]) != CFStringGetTypeID() || CFGetTypeID(values[i]) != CFDataGetTypeID() ||
          !CFStringGetCString(keys[i], key, sizeof(key), kCFStringEncodingUTF8)) {
        CFRelease(plist); xpc_release(object); return NULL;
      }
      xpc_dictionary_set_data(object, key, CFDataGetBytePtr(values[i]), CFDataGetLength(values[i]));
    }
    vmnet_return_t result;
    vmnet_network_ref network = vmnet_network_create_with_serialization(object, &result);
    *status = result;
    xpc_release(object); CFRelease(plist);
    return network;
  }
  return NULL;
}
#else
void *tart_vmnet_network_import_plist(const void *bytes, size_t length, int *status) {
  *status = 1009; return NULL;
}
#endif
