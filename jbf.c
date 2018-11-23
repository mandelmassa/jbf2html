/***************************************************************************
 *
 * Copyright (c) 2018 Mathias Thore
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ***************************************************************************/
#include <endian.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "jbf.h"

//#define DEBUG

static char *JBF_MAGIC = "JASC BROWS FILE";
#define THUMB_MAGIC 0xffffffff

struct filehdr {
    char          magic[16];
    uint8_t       data1[3];
    uint32_t      count;
} __attribute__ ((packed));

struct entryhdr {
    uint64_t      filetime;
    uint32_t      filetype;
    uint32_t      width;
    uint32_t      height;
    uint32_t      bpp;
    uint32_t      bufsize;
    uint32_t      filesize;
    uint32_t      data1[2];
    uint32_t      thumbmagic;
    uint32_t      thumbsize;
    uint8_t       jpghdr[];
} __attribute__ ((packed));

struct mmap_info {
    void         *addr;
    size_t        length;
};

static int parse_jbf(jbf_file *jbfdata, struct mmap_info *mmap_info);
static int parse_entry(uint8_t *data, jbf_entry *entry);
static void free_jbf(jbf_file *jbf);
static void free_entry(jbf_entry *entry);

int jbf_open(char *filename, jbf_file **jbfp)
{
    int fd;
    struct stat stats;
    int ret;
    int rv;
    jbf_file *jbf = NULL;
    struct mmap_info *mmap_info = NULL;

    // allocate memory
    jbf = (jbf_file *) malloc(sizeof(*jbf));
    if (jbf == NULL) {
        rv = JBFEMEM;
        goto clean;
    }
    memset(jbf, 0, sizeof(*jbf));

    mmap_info = (struct mmap_info *) malloc(sizeof(*mmap_info));
    if (mmap_info == NULL) {
        rv = JBFEMEM;
        goto clean;
    }
    memset(mmap_info, 0, sizeof(*mmap_info));
    mmap_info->addr = MAP_FAILED;

    jbf->_handle = (void *) mmap_info;

    // open file
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        rv = JBFEARGS;
        goto clean;
    }

    // find file size
    ret = fstat(fd, &stats);
    if (ret != 0) {
        close(fd);
        rv = JBFEARGS;
        goto clean;
    }
    mmap_info->length = stats.st_size;

    // jbf header needs to be at least 0x400 bytes long
    if (mmap_info->length < 0x400) {
        close(fd);
        rv = JBFECORRUPT;
        goto clean;
    }

    // mmap the file
    mmap_info->addr = mmap(NULL, mmap_info->length, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (mmap_info->addr == MAP_FAILED) {
        rv = JBFEMEM;
        goto clean;
    }

    // parse file
    ret = parse_jbf(jbf, mmap_info);
    if (ret != 0) {
        rv = JBFECORRUPT;
        goto clean;
    }

    // success
    *jbfp = jbf;
    return JBFSUCCESS;

 clean:
    jbf_close(jbf);
    return rv;
}

int jbf_close(jbf_file *jbf)
{
    if (jbf != NULL) {
        struct mmap_info *mmap_info = (struct mmap_info *) jbf->_handle;
        if (mmap_info != NULL &&
            mmap_info->addr != MAP_FAILED)
        {
            munmap(mmap_info->addr, mmap_info->length);
        }
        free_jbf(jbf);
        free(jbf);
        free(mmap_info);
    }
    return JBFSUCCESS;
}

static int parse_jbf(jbf_file *jbf, struct mmap_info *mmap_info)
{
    struct filehdr *hdr;
    uint32_t offset;
    int ret;
    uint32_t count = 0;
    jbf_entry *entries = NULL;
    uint8_t *addr = mmap_info->addr;

    hdr = (struct filehdr *) addr;

    // validate file magic
    if (memcmp(hdr->magic, JBF_MAGIC, 16)) {
        goto clean;
    }

    count = le32toh(hdr->count);
    entries = (jbf_entry *) calloc(count, sizeof(jbf_entry));

    if (entries == NULL) {
        goto clean;
    }

    jbf->entries = entries;
    jbf->dirname = strndup((char *) &addr[23], 0x400);

    // extract thumbnails
    offset = 0x400;
    for (jbf->entrycount = 0; jbf->entrycount < count; jbf->entrycount++) {
        ret = parse_entry(&addr[offset], &entries[jbf->entrycount]);
        if (ret <= 0) {
            goto clean;
        }
        offset += ret;

        if (offset > mmap_info->length) {
            goto clean;
        }
    }

#ifdef DEBUG
    printf("%d entries parsed, %d expected\n", jbf->entrycount, count);
    printf("offset=0x%08x, length=0x%08zx\n", offset, mmap_info->length);
#endif

    return 0;

 clean:
    free_jbf(jbf);
    return -1;
}

static void free_jbf(jbf_file *jbf)
{
    int i;
    if (jbf) {
        if (jbf->entries) {
            for (i = 0; i < jbf->entrycount; i++) {
                free_entry(&jbf->entries[i]);
            }
            free(jbf->entries);
            jbf->entries = NULL;
        }
        free(jbf->dirname);
        jbf->dirname = NULL;
    }
}

#ifdef DEBUG
static void dump_entry(uint8_t *data)
{
    struct entryhdr *hdr;
    uint32_t filenamelength;
    char *filename;

    filenamelength = le32toh(*(uint32_t *) data);
    filename = strndup((char *) &data[4], filenamelength);
    hdr = (struct entryhdr *) &data[4 + filenamelength];

    printf("name length : %u\n"
           "file name   : %s\n"
           "filetime    : 0x%016lx\n"
           "filetype    : 0x%x\n"
           "width       : %u\n"
           "height      : %u\n"
           "bpp         : %u\n"
           "bufsize     : %u\n"
           "filesize    : %u\n"
           "data1[0]    : 0x%x\n"
           "data1[1]    : 0x%x\n"
           "thumbmagic  : 0x%08x\n"
           "thumbsize   : %u\n"
           "\n",
           filenamelength,
           filename,
           le64toh(hdr->filetime),
           le32toh(hdr->filetype),
           le32toh(hdr->width),
           le32toh(hdr->height),
           le32toh(hdr->bpp),
           le32toh(hdr->bufsize),
           le32toh(hdr->filesize),
           le32toh(hdr->data1[0]),
           le32toh(hdr->data1[1]),
           le32toh(hdr->thumbmagic),
           le32toh(hdr->thumbsize));
}
#endif

static int parse_entry(uint8_t *data, jbf_entry *entry)
{
    struct entryhdr *hdr;
    uint32_t filenamelength;

#ifdef DEBUG
    dump_entry(data);
#endif

    // possibly unaligned accesses
    filenamelength = le32toh(*(uint32_t *) data);
    if (filenamelength > 255) {
        goto clean;
    }

    hdr = (struct entryhdr *) &data[4 + filenamelength];

    // prepare entry
    entry->filenamelength = filenamelength;
    entry->filename = strndup((char *) &data[4], filenamelength);
    entry->filetime = le64toh(hdr->filetime);
    entry->filetype = le32toh(hdr->filetype);
    entry->width    = le32toh(hdr->width);
    entry->height   = le32toh(hdr->height);
    entry->bpp      = le32toh(hdr->bpp);
    entry->bufsize  = le32toh(hdr->bufsize);
    entry->filesize = le32toh(hdr->filesize);

    if (hdr->thumbmagic != THUMB_MAGIC) {
        // entry without thumbnail, happens with raw files
        entry->thumbnail.size = 0;
        entry->thumbnail.data = NULL;
        return 4 +
            entry->filenamelength +
            36;
    }

    // check for SOI marker
    if(hdr->jpghdr[0] != 0xff ||
       hdr->jpghdr[1] != 0xd8)
    {
            goto clean;
    }

    entry->thumbnail.size = le32toh(hdr->thumbsize);
    entry->thumbnail.data = hdr->jpghdr;

    return 4 +                     // file name length
        entry->filenamelength +    // file name
        sizeof(struct entryhdr) +  // header size
        entry->thumbnail.size;     // thumbnail data

 clean:
    free_entry(entry);
    return -1;
}

static void free_entry(jbf_entry *entry)
{
    if (entry) {
        free(entry->filename);
        entry->filename = NULL;
    }
}
