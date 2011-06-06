#include "ipseeker.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <iconv.h>
#include <stdint.h>

#define IPFILE_NAME "QQWry.Dat"
#define IPFILE_MAX_STRING_LENGTH 1024
#define IPFILE_INDEX_RECORD_LENGTH 7
#define IPFILE_RECORD_REDIRECT_ALL 0x01
#define IPFILE_RECORD_REDIRECT_COUNTRY 0x02

static FILE* g_file = NULL;
static uint32_t g_ip_index_begin = 0;
static uint32_t g_ip_index_end = 0;
static uint32_t g_ip_index_number = 0;

static uint32_t ipfile_readint(long offset, unsigned len) {
    assert(len <= 4);
    fseek(g_file, offset, SEEK_SET);
    uint32_t val = 0;
    // little endian
    fread(&val, len, 1, g_file);
    return val;
}

static char* ipfile_readstring(long offset) {
    char buf[IPFILE_MAX_STRING_LENGTH];
    fseek(g_file, offset, SEEK_SET);
    fread(&buf, IPFILE_MAX_STRING_LENGTH, 1, g_file);
    buf[IPFILE_MAX_STRING_LENGTH - 1] = '\0';
    return strdup(buf);
}

static char* ipfile_gbk_to_utf8(char* str) {
    char buf[IPFILE_MAX_STRING_LENGTH];
    memset(buf, 0, sizeof(buf));

    iconv_t iconv_desc = iconv_open("utf-8", "gbk");
    char* inbuf = str;
    size_t inbytesleft = strlen(str);
    char* outbuf = buf;
    size_t outbytesleft = IPFILE_MAX_STRING_LENGTH;
    iconv(iconv_desc, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    iconv_close(iconv_desc);
    //free(tmp);
    return strdup(buf);
}

/*
static void ipfile_print_ip(ip_t ip) {
    fprintf(stderr, "Got ip: %d.%d.%d.%d\n",
        ip >> 24, (ip & 0xff0000) >> 16,
        (ip & 0xff00) >> 8, ip & 0xff);
}
*/

/** @return 0 if success */
int ipfile_init() {
    assert(g_file == NULL);
    g_file = fopen(IPFILE_NAME, "rb");
    if (g_file == NULL)
        return 1;
    g_ip_index_begin = ipfile_readint(0, 4);
    g_ip_index_end = ipfile_readint(4, 4) + IPFILE_INDEX_RECORD_LENGTH;
    g_ip_index_number = (g_ip_index_end - g_ip_index_begin) / IPFILE_INDEX_RECORD_LENGTH;
    return 0;
}

void ipfile_cleanup() {
    if (g_file != NULL) {
        fclose(g_file);
        g_file = NULL;
        g_ip_index_begin = g_ip_index_end = g_ip_index_number = 0;
    }
}

static long ipfile_search_record_by_ip(const ip_t ip) {
    uint32_t first = g_ip_index_begin;
    uint32_t half = 0, mid = 0;
    uint32_t len = g_ip_index_number;
    while (len > 1) {
        half = len >> 1;
        mid = first + half * IPFILE_INDEX_RECORD_LENGTH;
        ip_t mid_ip = ipfile_readint(mid, 4);
        if (mid_ip <= ip) {
            first = mid;
            len = len - half;
        } else {
            len = half;
        }
    }
    return ipfile_readint(first + 4, 3);
}

static char* ipfile_get_area_by_offset(long offset) {
    uint8_t ip_record_flag = ipfile_readint(offset, 1);
    if (ip_record_flag == IPFILE_RECORD_REDIRECT_ALL || ip_record_flag == IPFILE_RECORD_REDIRECT_COUNTRY) {
        offset = ipfile_readint(offset + 1, 3);
        if (offset != 0)
            return ipfile_readstring(offset);
        else
            return strdup("");  // unknown area
    } else {
        return ipfile_readstring(offset);
    }
}

ip_record_t ipfile_get_record_by_ip_gbk(const ip_t ip) {
    ip_record_t record;
    long offset = ipfile_search_record_by_ip(ip);
    uint8_t ip_record_flag = ipfile_readint(offset + 4, 1);
    if (ip_record_flag == IPFILE_RECORD_REDIRECT_ALL) {
        long redirect_offset = ipfile_readint(offset + 5, 3);
        ip_record_flag = ipfile_readint(redirect_offset, 1);

        if (ip_record_flag == IPFILE_RECORD_REDIRECT_COUNTRY) {
            long redirect_offset_country = ipfile_readint(redirect_offset + 1, 3);
            record.country_name = ipfile_readstring(redirect_offset_country);
            redirect_offset += 4;
        } else {
            record.country_name = ipfile_readstring(redirect_offset);
            redirect_offset += strlen(record.country_name) + 1;
        }
        record.area_name = ipfile_get_area_by_offset(redirect_offset);
    } else if (ip_record_flag == IPFILE_RECORD_REDIRECT_COUNTRY) {
        long redirect_offset_country = ipfile_readint(offset + 5, 3);;
        record.country_name = ipfile_readstring(redirect_offset_country);
        record.area_name = ipfile_get_area_by_offset(offset + 8);
    } else {
        offset += 4;
        record.country_name = ipfile_readstring(offset);
        offset += strlen(record.country_name) + 1;
        record.area_name = ipfile_get_area_by_offset(offset);
    }
    return record;
}

ip_record_t ipfile_get_record_by_ip_utf8(const ip_t ip) {
    ip_record_t record = ipfile_get_record_by_ip_gbk(ip);
    char* str = record.country_name;
    record.country_name = ipfile_gbk_to_utf8(str);
    free(str);
    str = record.area_name;
    record.area_name = ipfile_gbk_to_utf8(str);
    free(str);
    return record;
}

void ipfile_free_record(ip_record_t* record) {
    if (record->country_name != NULL)
        free(record->country_name);
    if (record->area_name != NULL)
        free(record->area_name);
}

/*
int main() {
    ipfile_init();
    ip_record_t r = ipfile_get_record_by_ip_utf8(0xffffffff);
    printf("%s (%s)\n", r.country_name, r.area_name);
    ipfile_free_record(&r);
    ipfile_cleanup();
    return 0;
}
*/
