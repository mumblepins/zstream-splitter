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

#ifndef ZSTREAM_SPLITTER_DEFINE_H
#define ZSTREAM_SPLITTER_DEFINE_H


/*
 * We currently support block sizes from 512 bytes to 16MB.
 * The benefits of larger blocks, and thus larger IO, need to be weighed
 * against the cost of COWing a giant block to modify one byte, and the
 * large latency of reading or writing a large block.
 *
 * Note that although blocks up to 16MB are supported, the recordsize
 * property can not be set larger than zfs_max_recordsize (default 1MB).
 * See the comment near zfs_max_recordsize in dsl_dataset.c for details.
 *
 * Note that although the LSIZE field of the blkptr_t can store sizes up
 * to 32MB, the dnode's dn_datablkszsec can only store sizes up to
 * 32MB - 512 bytes.  Therefore, we limit SPA_MAXBLOCKSIZE to 16MB.
 */
#define    SPA_MINBLOCKSHIFT    9
#define    SPA_OLD_MAXBLOCKSHIFT    17
#define    SPA_MAXBLOCKSHIFT    24
#define    SPA_MINBLOCKSIZE    (1ULL << SPA_MINBLOCKSHIFT)
#define    SPA_OLD_MAXBLOCKSIZE    (1ULL << SPA_OLD_MAXBLOCKSHIFT)
#define    SPA_MAXBLOCKSIZE    (1ULL << SPA_MAXBLOCKSHIFT)

#define    DMU_BACKUP_MAGIC 0x2F5bacbacULL

#define    BF64_DECODE(x, low, len)    P2PHASE((x) >> (low), 1ULL << (len))
#define    BF64_GET(x, low, len)        BF64_DECODE(x, low, len)
#define    DMU_GET_FEATUREFLAGS(vi)    BF64_GET((vi), 2, 30)

#define    DRR_WRITE_COMPRESSED(drrw)    ((drrw)->drr_compressiontype != 0)
#define    DRR_WRITE_PAYLOAD_SIZE(drrw) \
    (DRR_WRITE_COMPRESSED(drrw) ? (drrw)->drr_compressed_size : \
    (drrw)->drr_logical_size)

/*
 * Feature flags for zfs send streams (flags in drr_versioninfo)
 */

#define    DMU_BACKUP_FEATURE_DEDUP        (1 << 0)
#define    DMU_BACKUP_FEATURE_DEDUPPROPS        (1 << 1)
#define    DMU_BACKUP_FEATURE_SA_SPILL        (1 << 2)
/* flags #3 - #15 are reserved for incompatible closed-source implementations */
#define    DMU_BACKUP_FEATURE_EMBED_DATA        (1 << 16)
#define    DMU_BACKUP_FEATURE_LZ4            (1 << 17)
/* flag #18 is reserved for a Delphix feature */
#define    DMU_BACKUP_FEATURE_LARGE_BLOCKS        (1 << 19)
#define    DMU_BACKUP_FEATURE_RESUMING        (1 << 20)
/* flag #21 is reserved for a Delphix feature */
#define    DMU_BACKUP_FEATURE_COMPRESSED        (1 << 22)
#define    DMU_BACKUP_FEATURE_LARGE_DNODE        (1 << 23)


typedef enum dmu_send_resume_token_version {
    ZFS_SEND_RESUME_TOKEN_VERSION = 1
} dmu_send_resume_token_version_t;

#define    DMU_OT_NEWTYPE 0x80
#define    DMU_OT_METADATA 0x40
#define    DMU_OT_BYTESWAP_MASK 0x3f
/*
 * Defines a uint8_t object type. Object types specify if the data
 * in the object is metadata (boolean) and how to byteswap the data
 * (dmu_object_byteswap_t).
 */
#define    DMU_OT(byteswap, metadata) \
    (DMU_OT_NEWTYPE | \
    ((metadata) ? DMU_OT_METADATA : 0) | \
    ((byteswap) & DMU_OT_BYTESWAP_MASK))


#endif //ZSTREAM_SPLITTER_DEFINE_H
