/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Portions Copyright 2012 Martin Matuska <martin@matuska.org>
 */

/*
 * Copyright (c) 2013, 2015 by Delphix. All rights reserved.
 */

/*
 * Copyright (c) 2018 by Daniel Sullivan <mumblepins@gmail.com>
 */
#ifndef ZFS_RESUME_TOKEN_MAIN_H
#define ZFS_RESUME_TOKEN_MAIN_H

#include "config.h"
#include "define.h"
#include <ctype.h>
#include <libnvpair.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stddef.h>


#include <umem.h>
#include <libzfs.h>
#include <libzfs_core.h>
#include <sys/byteorder.h>
#include <zfs_fletcher.h>
#include <assert.h>

FILE *send_stream = 0;
boolean_t do_byteswap = B_FALSE;
boolean_t do_cksum = B_TRUE;

uint64_t total_chunk_size = 0;
uint64_t cur_chunk_size = 0;

uint64_t cur_object = 0;
uint64_t cur_offset = 0;

uint64_t cur_toguid;
uint64_t cur_fromguid;
char cur_toname[MAXNAMELEN];

boolean_t embed_ok = B_FALSE;
boolean_t compress_ok = B_FALSE;
boolean_t largeblock_ok = B_FALSE;
boolean_t have_begun = B_FALSE;

uint64_t split_at = 0;
uint64_t bytes_transferred = 0;

extern size_t gzip_compress(void *src, void *dst, size_t s_len, size_t d_len,
                            int level);

typedef enum dmu_object_byteswap {
    DMU_BSWAP_UINT8,
    DMU_BSWAP_UINT16,
    DMU_BSWAP_UINT32,
    DMU_BSWAP_UINT64,
    DMU_BSWAP_ZAP,
    DMU_BSWAP_DNODE,
    DMU_BSWAP_OBJSET,
    DMU_BSWAP_ZNODE,
    DMU_BSWAP_OLDACL,
    DMU_BSWAP_ACL,
    /*
     * Allocating a new byteswap type number makes the on-disk format
     * incompatible with any other format that uses the same number.
     *
     * Data can usually be structured to work with one of the
     * DMU_BSWAP_UINT* or DMU_BSWAP_ZAP types.
     */
            DMU_BSWAP_NUMFUNCS
} dmu_object_byteswap_t;


/*
 * On-disk ddt entry:  key (name) and physical storage (value).
 */
typedef struct ddt_key {
    zio_cksum_t ddk_cksum;    /* 256-bit block checksum */
    /*
     * Encoded with logical & physical size, and compression, as follows:
     *   +-------+-------+-------+-------+-------+-------+-------+-------+
     *   |   0   |   0   |   0   | comp  |     PSIZE     |     LSIZE     |
     *   +-------+-------+-------+-------+-------+-------+-------+-------+
     */
    uint64_t ddk_prop;
} ddt_key_t;


typedef enum dmu_object_type {
    DMU_OT_NONE,
    /* general: */
            DMU_OT_OBJECT_DIRECTORY,    /* ZAP */
    DMU_OT_OBJECT_ARRAY,        /* UINT64 */
    DMU_OT_PACKED_NVLIST,        /* UINT8 (XDR by nvlist_pack/unpack) */
    DMU_OT_PACKED_NVLIST_SIZE,    /* UINT64 */
    DMU_OT_BPOBJ,            /* UINT64 */
    DMU_OT_BPOBJ_HDR,        /* UINT64 */
    /* spa: */
            DMU_OT_SPACE_MAP_HEADER,    /* UINT64 */
    DMU_OT_SPACE_MAP,        /* UINT64 */
    /* zil: */
            DMU_OT_INTENT_LOG,        /* UINT64 */
    /* dmu: */
            DMU_OT_DNODE,            /* DNODE */
    DMU_OT_OBJSET,            /* OBJSET */
    /* dsl: */
            DMU_OT_DSL_DIR,            /* UINT64 */
    DMU_OT_DSL_DIR_CHILD_MAP,    /* ZAP */
    DMU_OT_DSL_DS_SNAP_MAP,        /* ZAP */
    DMU_OT_DSL_PROPS,        /* ZAP */
    DMU_OT_DSL_DATASET,        /* UINT64 */
    /* zpl: */
            DMU_OT_ZNODE,            /* ZNODE */
    DMU_OT_OLDACL,            /* Old ACL */
    DMU_OT_PLAIN_FILE_CONTENTS,    /* UINT8 */
    DMU_OT_DIRECTORY_CONTENTS,    /* ZAP */
    DMU_OT_MASTER_NODE,        /* ZAP */
    DMU_OT_UNLINKED_SET,        /* ZAP */
    /* zvol: */
            DMU_OT_ZVOL,            /* UINT8 */
    DMU_OT_ZVOL_PROP,        /* ZAP */
    /* other; for testing only! */
            DMU_OT_PLAIN_OTHER,        /* UINT8 */
    DMU_OT_UINT64_OTHER,        /* UINT64 */
    DMU_OT_ZAP_OTHER,        /* ZAP */
    /* new object types: */
            DMU_OT_ERROR_LOG,        /* ZAP */
    DMU_OT_SPA_HISTORY,        /* UINT8 */
    DMU_OT_SPA_HISTORY_OFFSETS,    /* spa_his_phys_t */
    DMU_OT_POOL_PROPS,        /* ZAP */
    DMU_OT_DSL_PERMS,        /* ZAP */
    DMU_OT_ACL,            /* ACL */
    DMU_OT_SYSACL,            /* SYSACL */
    DMU_OT_FUID,            /* FUID table (Packed NVLIST UINT8) */
    DMU_OT_FUID_SIZE,        /* FUID table size UINT64 */
    DMU_OT_NEXT_CLONES,        /* ZAP */
    DMU_OT_SCAN_QUEUE,        /* ZAP */
    DMU_OT_USERGROUP_USED,        /* ZAP */
    DMU_OT_USERGROUP_QUOTA,        /* ZAP */
    DMU_OT_USERREFS,        /* ZAP */
    DMU_OT_DDT_ZAP,            /* ZAP */
    DMU_OT_DDT_STATS,        /* ZAP */
    DMU_OT_SA,            /* System attr */
    DMU_OT_SA_MASTER_NODE,        /* ZAP */
    DMU_OT_SA_ATTR_REGISTRATION,    /* ZAP */
    DMU_OT_SA_ATTR_LAYOUTS,        /* ZAP */
    DMU_OT_SCAN_XLATE,        /* ZAP */
    DMU_OT_DEDUP,            /* fake dedup BP from ddt_bp_create() */
    DMU_OT_DEADLIST,        /* ZAP */
    DMU_OT_DEADLIST_HDR,        /* UINT64 */
    DMU_OT_DSL_CLONES,        /* ZAP */
    DMU_OT_BPOBJ_SUBOBJ,        /* UINT64 */
    /*
     * Do not allocate new object types here. Doing so makes the on-disk
     * format incompatible with any other format that uses the same object
     * type number.
     *
     * When creating an object which does not have one of the above types
     * use the DMU_OTN_* type with the correct byteswap and metadata
     * values.
     *
     * The DMU_OTN_* types do not have entries in the dmu_ot table,
     * use the DMU_OT_IS_METDATA() and DMU_OT_BYTESWAP() macros instead
     * of indexing into dmu_ot directly (this works for both DMU_OT_* types
     * and DMU_OTN_* types).
     */
            DMU_OT_NUMTYPES,

    /*
     * Names for valid types declared with DMU_OT().
     */
            DMU_OTN_UINT8_DATA = DMU_OT(DMU_BSWAP_UINT8, B_FALSE),
    DMU_OTN_UINT8_METADATA = DMU_OT(DMU_BSWAP_UINT8, B_TRUE),
    DMU_OTN_UINT16_DATA = DMU_OT(DMU_BSWAP_UINT16, B_FALSE),
    DMU_OTN_UINT16_METADATA = DMU_OT(DMU_BSWAP_UINT16, B_TRUE),
    DMU_OTN_UINT32_DATA = DMU_OT(DMU_BSWAP_UINT32, B_FALSE),
    DMU_OTN_UINT32_METADATA = DMU_OT(DMU_BSWAP_UINT32, B_TRUE),
    DMU_OTN_UINT64_DATA = DMU_OT(DMU_BSWAP_UINT64, B_FALSE),
    DMU_OTN_UINT64_METADATA = DMU_OT(DMU_BSWAP_UINT64, B_TRUE),
    DMU_OTN_ZAP_DATA = DMU_OT(DMU_BSWAP_ZAP, B_FALSE),
    DMU_OTN_ZAP_METADATA = DMU_OT(DMU_BSWAP_ZAP, B_TRUE),
} dmu_object_type_t;

//#define dmu_replay_record_t struct dmu_replay_record
//typedef struct dmu_replay_record dmu_replay_record_t;
//typedef  dmu_replay_record dmu_replay_record_t;

typedef struct dmu_replay_record {
    enum {
        DRR_BEGIN, DRR_OBJECT, DRR_FREEOBJECTS,
        DRR_WRITE, DRR_FREE, DRR_END, DRR_WRITE_BYREF,
        DRR_SPILL, DRR_WRITE_EMBEDDED, DRR_NUMTYPES
    } drr_type;
    uint32_t drr_payloadlen;
    union {
        struct drr_begin {
            uint64_t drr_magic;
            uint64_t drr_versioninfo; /* was drr_version */
            uint64_t drr_creation_time;
            dmu_objset_type_t drr_type;
            uint32_t drr_flags;
            uint64_t drr_toguid;
            uint64_t drr_fromguid;
            char drr_toname[MAXNAMELEN];
        } drr_begin;
        struct drr_end {
            zio_cksum_t drr_checksum;
            uint64_t drr_toguid;
        } drr_end;
        struct drr_object {
            uint64_t drr_object;
            dmu_object_type_t drr_type;
            dmu_object_type_t drr_bonustype;
            uint32_t drr_blksz;
            uint32_t drr_bonuslen;
            uint8_t drr_checksumtype;
            uint8_t drr_compress;
            uint8_t drr_dn_slots;
            uint8_t drr_pad[5];
            uint64_t drr_toguid;
            /* bonus content follows */
        } drr_object;
        struct drr_freeobjects {
            uint64_t drr_firstobj;
            uint64_t drr_numobjs;
            uint64_t drr_toguid;
        } drr_freeobjects;
        struct drr_write {
            uint64_t drr_object;
            dmu_object_type_t drr_type;
            uint32_t drr_pad;
            uint64_t drr_offset;
            uint64_t drr_logical_size;
            uint64_t drr_toguid;
            uint8_t drr_checksumtype;
            uint8_t drr_checksumflags;
            uint8_t drr_compressiontype;
            uint8_t drr_pad2[5];
            /* deduplication key */
            ddt_key_t drr_key;
            /* only nonzero if drr_compressiontype is not 0 */
            uint64_t drr_compressed_size;
            /* content follows */
        } drr_write;
        struct drr_free {
            uint64_t drr_object;
            uint64_t drr_offset;
            uint64_t drr_length;
            uint64_t drr_toguid;
        } drr_free;
        struct drr_write_byref {
            /* where to put the data */
            uint64_t drr_object;
            uint64_t drr_offset;
            uint64_t drr_length;
            uint64_t drr_toguid;
            /* where to find the prior copy of the data */
            uint64_t drr_refguid;
            uint64_t drr_refobject;
            uint64_t drr_refoffset;
            /* properties of the data */
            uint8_t drr_checksumtype;
            uint8_t drr_checksumflags;
            uint8_t drr_pad2[6];
            ddt_key_t drr_key; /* deduplication key */
        } drr_write_byref;
        struct drr_spill {
            uint64_t drr_object;
            uint64_t drr_length;
            uint64_t drr_toguid;
            uint64_t drr_pad[4]; /* needed for crypto */
            /* spill data follows */
        } drr_spill;
        struct drr_write_embedded {
            uint64_t drr_object;
            uint64_t drr_offset;
            /* logical length, should equal blocksize */
            uint64_t drr_length;
            uint64_t drr_toguid;
            uint8_t drr_compression;
            uint8_t drr_etype;
            uint8_t drr_pad[6];
            uint32_t drr_lsize; /* uncompressed size of payload */
            uint32_t drr_psize; /* compr. (real) size of payload */
            /* (possibly compressed) content follows */
        } drr_write_embedded;

        /*
         * Nore: drr_checksum is overlaid with all record types
         * except DRR_BEGIN.  Therefore its (non-pad) members
         * must not overlap with members from the other structs.
         * We accomplish this by putting its members at the very
         * end of the struct.
         */
        struct drr_checksum {
            uint64_t drr_pad[34];
            /*
             * fletcher-4 checksum of everything preceding the
             * checksum.
             */
            zio_cksum_t drr_checksum;
        } drr_checksum;
    } drr_u;
} dmu_replay_record_t;

static void usage(void);
static void * safe_malloc(size_t size);
static size_t ssread(void *buf, size_t len, zio_cksum_t *cksum);
static size_t read_hdr(dmu_replay_record_t *drr, zio_cksum_t *cksum);
void print_token();
int main(int argc, char *argv[]);

#endif //ZFS_RESUME_TOKEN_MAIN_H
