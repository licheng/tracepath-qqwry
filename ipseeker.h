#ifndef _IPSEEKER_H_
#define _IPSEEKER_H_

#include <stdint.h>

typedef uint32_t ip_t;
typedef struct {
    char* country_name;
    char* area_name;
} ip_record_t;

#ifdef __cplusplus
extern "C" {
#endif

    int ipfile_init();
    void ipfile_cleanup();
    ip_record_t ipfile_get_record_by_ip_gbk(const ip_t ip);
    ip_record_t ipfile_get_record_by_ip_utf8(const ip_t ip);
    void ipfile_free_record(ip_record_t* record);

#ifdef __cplusplus
}
#endif

#endif
