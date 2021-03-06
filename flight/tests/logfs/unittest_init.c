/*
 * These need to be defined in a .c file so that we can use
 * designated initializer syntax which c++ doesn't support (yet).
 */

#include "pios_flash_ut_priv.h"


const struct pios_flash_ut_cfg flash_config = {
    .size_of_flash  = 0x00300000,
    .size_of_sector = 0x00010000,
};

#include "pios_flashfs_logfs_priv.h"

const struct flashfs_logfs_cfg flashfs_config_partition_a = {
    .fs_magic      = 0x89abceef,
    .total_fs_size = 0x00200000, /* 2M bytes (32 sectors) */
    .arena_size    = 0x00010000, /* 256 * slot size */
    .slot_size     = 0x00000100, /* 256 bytes */

    .start_offset  = 0,          /* start at the beginning of the chip */
    .sector_size   = 0x00010000, /* 64K bytes */
    .page_size     = 0x00000100, /* 256 bytes */
};

const struct flashfs_logfs_cfg flashfs_config_partition_b = {
    .fs_magic      = 0x89abceef,
    .total_fs_size = 0x00100000, /* 1M bytes (16 sectors) */
    .arena_size    = 0x00010000, /* 64 * slot size */
    .slot_size     = 0x00000400, /* 256 bytes */

    .start_offset  = 0x00200000, /* start after partition a */
    .sector_size   = 0x00010000, /* 64K bytes */
    .page_size     = 0x00000100, /* 256 bytes */
};
