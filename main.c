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
#include "main.h"


static void usage(void) {
    (void) fprintf(stderr, "usage: zstream-splitter [-h] [-b size] [-s size]\n");
    (void) fprintf(stderr, "\t-h        -- show this help\n");
    (void) fprintf(stderr, "\t-b <size> -- specify a breakpoint in bytes, where <size> can be given in b,k,M,G\n");
    (void) fprintf(stderr,
                   "\t-s <size> -- specify amount already transferred in bytes, where <size> can be given in b,k,M,G\n");
    (void) fprintf(stderr, "\t-V        -- display version\n\n");
    (void) fprintf(stderr, "  Splits a stream from 'zfs send' and outputs the appropriate\n"
                           "  resume token. STDOUT and STDIN should not be terminals.");
    exit(1);
}

static void * safe_malloc(size_t size) {
    void *rv = malloc(size);
    if (rv == NULL) {
        (void) fprintf(stderr, "ERROR; failed to allocate %zu bytes\n", size);
        abort();
    }
    return (rv);
}

/*
 * ssread - send stream read.
 *
 * Read while computing incremental checksum
 */
static size_t ssread(void *buf, size_t len, zio_cksum_t *cksum) {
    size_t outlen;

    if ((outlen = fread(buf, len, 1, send_stream)) == 0)
        return (0);
    if (fwrite(buf, len, 1, stdout) == 0)
        return (0);

    if (do_cksum) {
        if (do_byteswap)
            fletcher_4_incremental_byteswap(buf, len, cksum);
        else
            fletcher_4_incremental_native(buf, len, cksum);
    }
    cur_chunk_size += len;
//	fprintf(stderr, "--SIZE: %lld\n", (longlong_t) len);
    return (outlen);
}

static size_t read_hdr(dmu_replay_record_t *drr, zio_cksum_t *cksum) {
    ASSERT3U(offsetof(dmu_replay_record_t, drr_u.drr_checksum.drr_checksum),
             ==, sizeof(dmu_replay_record_t) - sizeof(zio_cksum_t));
    size_t r = ssread(drr, sizeof(*drr) - sizeof(zio_cksum_t), cksum);
    if (r == 0)
        return (0);
    zio_cksum_t saved_cksum = *cksum;
    r = ssread(&drr->drr_u.drr_checksum.drr_checksum, sizeof(zio_cksum_t),
               cksum);
    if (r == 0)
        return (0);
    if (!ZIO_CHECKSUM_IS_ZERO(&drr->drr_u.drr_checksum.drr_checksum) &&
        !ZIO_CHECKSUM_EQUAL(saved_cksum,
                            drr->drr_u.drr_checksum.drr_checksum)) {
        fprintf(stderr, "invalid checksum\n");
        (void) fprintf(stderr, "Incorrect checksum in record header.\n");
        (void) fprintf(stderr, "Expected checksum = %llx/%llx/%llx/%llx\n",
                       (longlong_t) saved_cksum.zc_word[0],
                       (longlong_t) saved_cksum.zc_word[1],
                       (longlong_t) saved_cksum.zc_word[2],
                       (longlong_t) saved_cksum.zc_word[3]);
        return (0);
    }
    return (sizeof(*drr));
}

void print_token() {
    nvlist_t *token_nv = fnvlist_alloc();
    char *str;
    void *packed;
    uint8_t *compressed;
    size_t packed_size, compressed_size;
    zio_cksum_t cksum;
    int i;

    if (cur_fromguid != 0)
        fnvlist_add_uint64(token_nv, "fromguid", cur_fromguid);
    fnvlist_add_uint64(token_nv, "object", cur_object);
    fnvlist_add_uint64(token_nv, "offset", cur_offset);
    fnvlist_add_uint64(token_nv, "bytes", total_chunk_size + bytes_transferred);
    fnvlist_add_uint64(token_nv, "toguid", cur_toguid);
    fnvlist_add_string(token_nv, "toname", cur_toname);
    if (largeblock_ok)
        fnvlist_add_boolean(token_nv, "largeblockok");
    if (embed_ok)
        fnvlist_add_boolean(token_nv, "embedok");
    if (compress_ok)
        fnvlist_add_boolean(token_nv, "compressok");

    packed = fnvlist_pack(token_nv, &packed_size);
    fnvlist_free(token_nv);
    compressed = umem_alloc(packed_size, UMEM_NOFAIL);

    compressed_size = gzip_compress(packed, compressed, packed_size,
                                    packed_size, 6);

    fletcher_4_native_varsize(compressed, compressed_size, &cksum);

    str = umem_alloc(compressed_size * 2 + 1, UMEM_NOFAIL);
    for (i = 0; i < compressed_size; i++) {
        (void) sprintf(str + i * 2, "%02x", compressed[i]);
    }
    str[compressed_size * 2] = '\0';
    fprintf(stderr, "receive_resume_token:\t%u-%llx-%llx-%s\n",
            ZFS_SEND_RESUME_TOKEN_VERSION, (longlong_t) cksum.zc_word[0],
            (longlong_t) packed_size, str);
    fprintf(stderr, "bytes:\t%lld\n",
            (longlong_t) total_chunk_size + (longlong_t) bytes_transferred);
}

int main(int argc, char *argv[]) {
    char *buf = safe_malloc(SPA_MAXBLOCKSIZE);
    dmu_replay_record_t thedrr;
    dmu_replay_record_t *drr = &thedrr;
    struct drr_begin *drrb = &thedrr.drr_u.drr_begin;
    struct drr_object *drro = &thedrr.drr_u.drr_object;
    struct drr_write *drrw = &thedrr.drr_u.drr_write;
    struct drr_spill *drrs = &thedrr.drr_u.drr_spill;
    struct drr_write_embedded *drrwe = &thedrr.drr_u.drr_write_embedded;\
    char c;
    boolean_t first = B_TRUE;
    zio_cksum_t zc = {{0}};
    zio_cksum_t pcksum = {{0}};

    while ((c = getopt(argc, argv, ":hb:s:V")) != -1) {
        switch (c) {
            case 'b':
                if (zfs_nicestrtonum(NULL, optarg, &split_at) != 0) {
                    (void) fprintf(stderr, "bad split size '%s'\n", optarg);
                    usage();
                }
                fprintf(stderr, "Splitting approximately every %lld bytes\n",
                        (longlong_t) split_at);
                break;
            case 's':
                if (zfs_nicestrtonum(NULL, optarg, &bytes_transferred) != 0) {
                    (void) fprintf(stderr, "bad transferred size '%s'\n", optarg);
                    usage();
                }
                bytes_transferred += sizeof(zio_cksum_t) * 3; // TODO: why?
                fprintf(stderr, "Adding %lld to bytes transferred\n",
                        (longlong_t) bytes_transferred);
                break;
            case 'h':
                usage();
                break;
            case 'V':
                fprintf(stdout, "zstream-splitter v %s (gcc %s, %s)\n",
                        PROJECT_VERSION, __VERSION__, SYS_NAME_ARCH);
                exit(0);
                break;
            case '?':
                (void) fprintf(stderr, "invalid option '%c'\n", optopt);
                usage();
                break;
        }
    }

    if (isatty(STDIN_FILENO)) {
        (void) fprintf(stderr, "Error: Backup stream can not be read "
                               "from a terminal.\n"
                               "You must redirect standard input.\n\n");
        usage();
    }
    if (isatty(STDOUT_FILENO)) {
        (void) fprintf(stderr, "Error: Backup stream can not be "
                               "written to a terminal.\n"
                               "You must redirect into something.\n\n");
        usage();
    }
    fletcher_4_init();
    send_stream = stdin;
    while (read_hdr(drr, &zc)) {
        /*
         * If this is the first DMU record being processed, check for
         * the magic bytes and figure out the endian-ness based on them.
         */
        if (first) {
            if (drrb->drr_magic == BSWAP_64(DMU_BACKUP_MAGIC)) {
                do_byteswap = B_TRUE;
                if (do_cksum) {
                    ZIO_SET_CHECKSUM(&zc, 0, 0, 0, 0);
                    /*
                     * recalculate header checksum now
                     * that we know it needs to be
                     * byteswapped.
                     */
                    fletcher_4_incremental_byteswap(drr,
                                                    sizeof(dmu_replay_record_t), &zc);
                }
            } else if (drrb->drr_magic != DMU_BACKUP_MAGIC) {
                (void) fprintf(stderr, "Invalid stream "
                                       "(bad magic number)\n");
                exit(1);
            }
            first = B_FALSE;
        }
        if (do_byteswap) {
            drr->drr_type = BSWAP_32(drr->drr_type);
            drr->drr_payloadlen = BSWAP_32(drr->drr_payloadlen);
        }

        /*
         * At this point, the leading fields of the replay record
         * (drr_type and drr_payloadlen) have been byte-swapped if
         * necessary, but the rest of the data structure (the
         * union of type-specific structures) is still in its
         * original state.
         */
        if (drr->drr_type >= DRR_NUMTYPES) {
            (void) fprintf(stderr, "INVALID record found: type 0x%x\n",
                           drr->drr_type);
            (void) fprintf(stderr, "Aborting.\n");
            exit(1);
        }
        switch (drr->drr_type) {
            case DRR_BEGIN:
                if (do_byteswap) {
                    drrb->drr_flags = BSWAP_32(drrb->drr_flags);
                    drrb->drr_toguid = BSWAP_64(drrb->drr_toguid);
                    drrb->drr_fromguid = BSWAP_64(drrb->drr_fromguid);
                }
                if (drr->drr_payloadlen != 0) {
//				nvlist_t *nv;
                    int sz = drr->drr_payloadlen;

                    if (sz > SPA_MAXBLOCKSIZE) {
                        free(buf);
                        buf = safe_malloc(sz);
                    }
                    (void) ssread(buf, sz, &zc);
                }
                cur_chunk_size = 0;
                total_chunk_size = 0;

                cur_fromguid = drrb->drr_fromguid;
                cur_toguid = drrb->drr_toguid;
                strncpy(cur_toname, drrb->drr_toname, MAXNAMELEN);
                uint64_t flags = DMU_GET_FEATUREFLAGS(drrb->drr_versioninfo);
                if (flags & DMU_BACKUP_FEATURE_COMPRESSED)
                    compress_ok = B_TRUE;
                else
                    compress_ok = B_FALSE;

                if (flags & DMU_BACKUP_FEATURE_EMBED_DATA)
                    embed_ok = B_TRUE;
                else
                    embed_ok = B_FALSE;

                if (flags & DMU_BACKUP_FEATURE_LARGE_BLOCKS)
                    largeblock_ok = B_TRUE;
                else
                    largeblock_ok = B_FALSE;
                have_begun = B_TRUE;
                break;

            case DRR_END:
                cur_chunk_size = 0;
                ZIO_SET_CHECKSUM(&zc, 0, 0, 0, 0);
                have_begun = B_FALSE;
                break;

            case DRR_OBJECT:
                if (do_byteswap) {
                    drro->drr_bonuslen = BSWAP_32(drro->drr_bonuslen);
                }

                if (drro->drr_bonuslen > 0) {
                    (void) ssread(buf, P2ROUNDUP(drro->drr_bonuslen, 8), &zc);
                }
                break;

            case DRR_FREEOBJECTS:
                break;

            case DRR_WRITE:
                if (do_byteswap) {
                    drrw->drr_object = BSWAP_64(drrw->drr_object);
                    drrw->drr_offset = BSWAP_64(drrw->drr_offset);
                    drrw->drr_logical_size = BSWAP_64(drrw->drr_logical_size);
                    drrw->drr_compressed_size = BSWAP_64(drrw->drr_compressed_size);
                }

                uint64_t payload_size = DRR_WRITE_PAYLOAD_SIZE(drrw);

                /*
                 * Read the contents of the block in from STDIN to buf
                 */
                int r = ssread(buf, payload_size, &zc);
                if (r) {

                    if (split_at && total_chunk_size + cur_chunk_size >= split_at) {
                        goto filesplit;
                    }
                    total_chunk_size += cur_chunk_size;
                    cur_chunk_size = 0;

                    cur_object = drrw->drr_object;
                    cur_offset = drrw->drr_offset;
                }
                break;

            case DRR_WRITE_BYREF:
                break;

            case DRR_FREE:
                break;

            case DRR_SPILL:
                if (do_byteswap) {
                    drrs->drr_object = BSWAP_64(drrs->drr_object);
                    drrs->drr_length = BSWAP_64(drrs->drr_length);
                }
                (void) ssread(buf, drrs->drr_length, &zc);
                break;

            case DRR_WRITE_EMBEDDED:
                if (do_byteswap) {
                    drrwe->drr_object = BSWAP_64(drrwe->drr_object);
                    drrwe->drr_offset = BSWAP_64(drrwe->drr_offset);
                    drrwe->drr_length = BSWAP_64(drrwe->drr_length);
                    drrwe->drr_toguid = BSWAP_64(drrwe->drr_toguid);
                    drrwe->drr_lsize = BSWAP_32(drrwe->drr_lsize);
                    drrwe->drr_psize = BSWAP_32(drrwe->drr_psize);
                }
                (void) ssread(buf, P2ROUNDUP(drrwe->drr_psize, 8), &zc);
                break;
            case DRR_NUMTYPES:
                /* should never be reached */
                exit(1);
        }
        pcksum = zc;
    }

    filesplit:

    free(buf);
    fletcher_4_fini();

//    fclose(stdout);
    if (have_begun) {
        print_token();
        return (10);
    } else {
        fprintf(stderr, "zfs send completed\n");
        return (0);
    }
}
