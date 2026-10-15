// Create only a logical network as root. Tart imports it as the VM owner.
#include <vmnet/vmnet.h>
#include <CoreFoundation/CoreFoundation.h>
#include <xpc/xpc.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
 if(argc!=4 || geteuid()!=0){fprintf(stderr,"usage (root): vmnet-prepare CIDR NEW_FILE OWNER_UID\n");return 2;}
 char ip[INET_ADDRSTRLEN],extra;unsigned prefix;struct in_addr a,m;
 if(sscanf(argv[1],"%15[^/]/%u%c",ip,&prefix,&extra)!=2 || prefix<8 || prefix>30 || inet_pton(AF_INET,ip,&a)!=1)return 2;
 uint32_t addr=ntohl(a.s_addr), mask=UINT32_MAX<<(32-prefix), end=addr|~mask;
 if((addr&mask)!=addr || !((addr>=0x0a000000 && end<=0x0affffff)||(addr>=0xac100000 && end<=0xac1fffff)||(addr>=0xc0a80000 && end<=0xc0a8ffff)))return 2;
 char *tail;unsigned long owner=strtoul(argv[3],&tail,10);if(*tail || owner==0 || owner>UINT32_MAX)return 2;
 m.s_addr=htonl(mask);a.s_addr=htonl(addr+1);vmnet_return_t status;
 vmnet_network_configuration_ref config=vmnet_network_configuration_create(VMNET_SHARED_MODE,&status);
 if(!config)return 3;
 status=vmnet_network_configuration_set_ipv4_subnet(config,&a,&m);
 if(status!=VMNET_SUCCESS){CFRelease(config);return 3;}
 vmnet_network_ref network=vmnet_network_create(config,&status);CFRelease(config);
 if(!network){fprintf(stderr,"network creation status=%d\n",status);return 3;}
 struct in_addr actual,actualMask;vmnet_network_get_ipv4_subnet(network,&actual,&actualMask);
 if((ntohl(actual.s_addr)&mask)!=addr || actualMask.s_addr!=m.s_addr){CFRelease(network);return 3;}
 xpc_object_t object=vmnet_network_copy_serialization(network,&status);
 if(!object){CFRelease(network);return 3;}
 CFMutableDictionaryRef dict=CFDictionaryCreateMutable(NULL,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
 __block int valid=1;
 xpc_dictionary_apply(object, ^bool(const char *key,xpc_object_t value){
  if(xpc_get_type(value)!=XPC_TYPE_DATA){valid=0;return false;}
  CFStringRef k=CFStringCreateWithCString(NULL,key,kCFStringEncodingUTF8);
  CFDataRef v=CFDataCreate(NULL,xpc_data_get_bytes_ptr(value),xpc_data_get_length(value));
  CFDictionarySetValue(dict,k,v);CFRelease(k);CFRelease(v);return true;
 });
 xpc_release(object);
 if(!valid){CFRelease(dict);CFRelease(network);return 3;}
 CFDataRef payload=CFPropertyListCreateData(NULL,dict,kCFPropertyListBinaryFormat_v1_0,0,NULL);CFRelease(dict);
 if(!payload){CFRelease(network);return 3;}
 int fd=open(argv[2],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
 if(fd<0){perror("create private network file");return 4;}
 const unsigned char *bytes=CFDataGetBytePtr(payload);size_t left=CFDataGetLength(payload);
 while(left){ssize_t n=write(fd,bytes,left);if(n<0&&errno==EINTR)continue;if(n<=0){close(fd);unlink(argv[2]);return 4;}bytes+=n;left-=n;}
 if(fchown(fd,(uid_t)owner,(gid_t)-1)!=0){close(fd);unlink(argv[2]);return 4;}
 close(fd);CFRelease(payload);
 printf("Prepared %s for uid %lu\n",argv[1],owner);fflush(stdout);
 // The creator must remain alive until VZ has attached; retain ownership for
 // the session. The caller removes this private token on exit. Bound lifetime.
 for(unsigned elapsed=0;elapsed<28800 && access(argv[2],F_OK)==0;elapsed++)sleep(1);
 CFRelease(network);
 printf("Prepared %s for uid %lu\n",argv[1],owner);return 0;
}
