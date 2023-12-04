/*
 * Copyright (c) 2022-2023 Macronix International Co. LTD. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <rwwee1.h>
#include "string.h"
#include "stdlib.h"

extern int mx_ee_rww_read(uint32_t addr, uint32_t len, void *buf);
extern int mx_ee_rww_write(uint32_t addr, uint32_t len, void *buf);
extern int mx_ee_rww_erase(uint32_t addr, uint32_t len);
#ifdef MX_GENERIC_RWW
extern int mx_rww_read(uint32_t addr, uint32_t len, uint8_t *buf);
extern int mx_rww_write(uint32_t addr, uint32_t len, uint8_t *buf);
extern int mx_rww_erase(uint32_t addr, uint32_t len);
#endif
extern int mx_ee_rww_init(void);
extern void mx_ee_rww_deinit(void);

/* EEPROM physical address offset in each bank */
static uint32_t bank_offset[MX_EEPROMS] = { 0x00200000, 0x01200000, 0x02200000,
        0x03200000 };

/* Main structure */
static struct eeprom_info mx_eeprom = { .initialized = false,
#ifdef MX_EEPROM_BACKGROUND_THREAD
  .bgStart = false,
#endif
};

#ifdef MX_EEPROM_CRC_HW
/* CRC handler */
static CRC_HandleTypeDef hcrc;
#endif

#ifdef MX_EEPROM_PC_PROTECTION
/**
  * @brief  Update system entry of current block of current bank.
  * @param  bi: Current bank handle
  * @param  entry: System entry address
  * @param  sys: System entry buffer
  * @retval Status
*/
static int mx_ee_read_sys(struct bank_info *bi, uint32_t entry,
                          struct system_entry *sys)
{
    int ret;
    uint32_t addr;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS) ||
        (entry >= MX_EEPROM_SYSTEM_ENTRIES))
    return MX_EINVAL;

    addr = bi->block_offset + MX_EEPROM_SYSTEM_SECTOR_OFFSET +
        entry * MX_EEPROM_SYSTEM_ENTRY_SIZE;

    /* Read system entry */
    ret = mx_ee_rww_read(addr, sizeof(*sys), sys);
    if (ret)
    {
        mx_err("mxee_rdsys: fail to read, bank %lu, block %lu, entry %lu\r\n",
            bi->bank, bi->block, entry);
        return ret;
    }

    /* Check system entry */
    if ((sys->id != MFTL_ID && sys->id != DATA_NONE16) ||
        (sys->cksum != (sys->id ^ sys->ops ^ sys->arg)))
    {
        mx_err("mxee_rdsys: corrupted entry, bank %lu, block %lu, entry %lu\r\n",
            bi->bank, bi->block, entry);
        return MX_EIO;
    }

    return MX_OK;
}

/**
  * @brief  Update system entry of current block of current bank.
  * @param  bi: Current bank handle
  * @param  ops: Operation type
  * @param  arg: Operation argument
  * @retval Status
*/
static int mx_ee_update_sys(struct bank_info *bi, uint32_t block, rwwee_ops ops, uint32_t arg)
{
    int ret;
    uint32_t addr;
    struct system_entry sys;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (block >= MX_EEPROM_BLOCKS) ||
        (bi->sys_entry[block] >= MX_EEPROM_SYSTEM_ENTRIES))
    return MX_EINVAL;

    addr = bi->bank_offset + block * MX_EEPROM_CLUSTER_SIZE +
        MX_EEPROM_SYSTEM_SECTOR_OFFSET;

    if (++bi->sys_entry[block] == MX_EEPROM_SYSTEM_ENTRIES)
    {
#ifdef MX_DEBUG
        /* Erase count statistics */
        bi->eraseCnt[block][MX_EEPROM_SYSTEM_SECTOR]++;
#endif

        /* Erase system sector */
        ret = mx_ee_rww_erase(addr, MX_FLASH_SECTOR_SIZE);
        if (ret)
        {
            mx_err("mxee_wrsys: fail to erase, bank %lu, block %lu, sector %d\r\n",
                bi->bank, block, MX_EEPROM_SYSTEM_SECTOR);

            bi->sys_entry[block] = DATA_NONE32;
            return ret;
        }

        /* Round-robin method */
        bi->sys_entry[block] = 0;
    }

    /* Fill system entry */
    sys.id = MFTL_ID;
    sys.ops = ops;
    sys.arg = arg;
    sys.cksum = sys.id ^ sys.ops ^ sys.arg;

    addr += bi->sys_entry[block] * MX_EEPROM_SYSTEM_ENTRY_SIZE;

    /* Update system info */
    ret = mx_ee_rww_write(addr, sizeof(sys), &sys);
    if (ret)
    {
        mx_err("mxee_wrsys: fail to update, bank %lu, block %lu, entry %lu\r\n",
            bi->bank, block, bi->sys_entry[block]);
        return ret;
    }

    return MX_OK;
}
#endif

/**
 * @brief  Read the specified entry of current block of current bank.
 * @param  bi: Current bank handle
 * @param  entry: Local entry address
 * @param  buf: Data buffer
 * @param  header: Read entry header only (true) or the whole entry (false)
 * @retval Status
 */
static int readcnt = 0, noerrcnt = 0;

static int mx_ee_read(struct bank_info *bi, uint32_t entry, void *buf, bool header) {
    int ret;
    uint32_t addr, len, cksum;
    struct eeprom_entry *cache = buf;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS)
            || (entry >= MX_EEPROM_ENTRIES_PER_CLUSTER))
        return MX_EINVAL;

    addr = entry * MX_EEPROM_ENTRY_SIZE + bi->block_offset;
    len = header ? MX_EEPROM_HEADER_SIZE : MX_EEPROM_ENTRY_SIZE;

    readcnt++;

    /* Do the real read */
    ret = mx_ee_rww_read(addr, len, buf);
    if (ret) {
        mx_err("mxee_rddat: fail to read %s, bank %lu, block %lu, entry %lu\r\n",
                header ? "header" : "entry", bi->bank, bi->block, entry);
        return ret;
    }

    /* Check entry address */
    cksum = cache->header.LPA + cache->header.LPA_inv;
    if ((cksum != DATA_NONE8) && (cksum != DATA_NONE8 + DATA_NONE8)) {
        mx_err("mxee_rddat: corrupted entry LPA 0x%02x, inv 0x%02x, "
                "bank %lu, block %lu, entry %lu\r\n",
                cache->header.LPA, cache->header.LPA_inv,
                bi->bank, bi->block, entry);
        return MX_EIO;
    }

#ifdef MX_EEPROM_CRC_HW
    /* Check entry data */
    if (!header) {
        if (osMutexWait(mx_eeprom.crcLock, osWaitForever))
            return MX_EOS;

        /* Calculate data CRC */
        cksum = HAL_CRC_Calculate(&hcrc, (uint32_t*) cache->data,
        CRC16_DATA_LENGTH);

        /* Add rwCnt inside the mutex lock */
        mx_eeprom.rwCnt++;

        osMutexRelease(mx_eeprom.crcLock);

        /* Check data CRC */
        if (cache->header.crc != (cksum & DATA_NONE16)) {
            mx_err("mxee_rddat: corrupted entry data, crc 0x%04x -> 0x%04x, "
                    "bank %lu, block %lu, entry %lu\r\n",
                    cache->header.crc, (uint16_t)cksum,
                    bi->bank, bi->block, entry);
            return MX_EIO;
        }
    }
#endif

    noerrcnt++;

    return MX_OK;
}

/**
 * @brief  Write the specified entry of current block of current bank.
 * @param  bi: Current bank handle
 * @param  entry: Local entry address
 * @param  buf: Data buffer
 * @retval Status
 */
static int mx_ee_write(struct bank_info *bi, uint32_t entry, void *buf) {
    int ret;
    uint32_t addr;
    struct eeprom_entry *cache = buf;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS)
        || (entry >= MX_EEPROM_ENTRIES_PER_CLUSTER))
        return MX_EINVAL;

    addr = entry * MX_EEPROM_ENTRY_SIZE + bi->block_offset;

    /* Calculate redundant LPA */
    cache->header.LPA_inv = ~cache->header.LPA;

#ifdef MX_EEPROM_CRC_HW
    if (osMutexWait(mx_eeprom.crcLock, osWaitForever))
        return MX_EOS;

    /* Calculate data CRC */
    cache->header.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*) cache->data,
    CRC16_DATA_LENGTH);

    /* Add rwCnt inside the mutex lock */
    mx_eeprom.rwCnt++;

    osMutexRelease(mx_eeprom.crcLock);
#endif

    /* Do the real write */
    ret = mx_ee_rww_write(addr, MX_EEPROM_ENTRY_SIZE, cache);
    if (ret) {
        mx_err("mxee_wrdat: fail to write, bank %lu, block %lu, entry %lu\r\n",
            bi->bank, bi->block, entry);
    }

    return ret;
}

/**
 * @brief  Erase the obsoleted sector of current bank.
 * @param  bi: Current bank handle
 * @retval Status
 */
static int mx_ee_erase(struct bank_info *bi) {
    int ret;
    uint32_t addr;

    /* Check address validity */
    if (bi->bank >= MX_EEPROMS)
        return MX_EINVAL;
    if ((bi->dirty_block >= MX_EEPROM_BLOCKS) ||
        (bi->dirty_sector >= MX_EEPROM_DATA_SECTORS))
    return MX_OK;

    addr = bi->dirty_sector * MX_FLASH_SECTOR_SIZE
        + bi->dirty_block * MX_EEPROM_CLUSTER_SIZE + bi->bank_offset;

#ifdef MX_DEBUG
    /* Erase count statistics */
    bi->eraseCnt[bi->dirty_block][bi->dirty_sector]++;
#endif

#ifdef MX_EEPROM_PC_PROTECTION
    /* Erase begin */
    mx_ee_update_sys(bi, bi->dirty_block, OPS_ERASE_BEGIN, bi->dirty_sector);
#endif

    /* Erase obsoleted sector */
    ret = mx_ee_rww_erase(addr, MX_FLASH_SECTOR_SIZE);
    if (ret) {
        mx_err("mxee_erase: fail to erase, bank %lu, block %lu, sector %lu\r\n",
                bi->bank, bi->dirty_block, bi->dirty_sector);
    }

#ifdef MX_EEPROM_PC_PROTECTION
  /* Erase end, XXX: will block RWE */
#endif

    if (bi->dirty_block == bi->block) {
        /* Mark as free or bad sector */
        if (!ret)
            bi->p2l[bi->dirty_sector] = DATA_NONE8;
        else
            bi->p2l[bi->dirty_sector] = MX_EEPROM_LPAS_PER_CLUSTER;
    }

    bi->dirty_block = DATA_NONE32;
    bi->dirty_sector = DATA_NONE32;

    return ret;
}

/**
 * @brief  Locate the latest version of specified logical page.
 * @param  bi: Current bank handle
 * @param  LPA: Local logical page address
 * @param  free: Find the latest version (false) or next free slot (true)
 * @retval Local entry address
 */
static uint32_t mx_ee_find_latest(struct bank_info *bi, uint32_t LPA, bool free) {
    struct eeprom_header header;
    uint32_t ofs, entry, cnt = 0, latest = DATA_NONE32;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS) ||
        (LPA >= MX_EEPROM_LPAS_PER_CLUSTER))
        return DATA_NONE32;

    ofs = bi->l2ps[LPA];

    /* No mapping */
    if (ofs >= MX_EEPROM_DATA_SECTORS)
        return DATA_NONE32;

    ofs *= MX_EEPROM_ENTRIES_PER_SECTOR;
    entry = bi->l2pe[LPA];

    /* Check if entry mapping exits */
    if (entry) {
        assert_param(entry < MX_EEPROM_ENTRIES_PER_SECTOR);

        if (!free)
            return entry + ofs;
        else if (entry == MX_EEPROM_ENTRIES_PER_SECTOR - 1)
            return DATA_NONE32;
        else
            cnt = entry + 1;
    }

    /* Read each entry to find the latest, TODO: binary search method */
    for (entry = ofs + cnt; cnt < MX_EEPROM_ENTRIES_PER_SECTOR;
            cnt++, entry++) {
        /* Read entry header, XXX: Potential risk to check header only? */
        if (mx_ee_read(bi, entry, &header, true)) {
            mx_err("mxee_latst: fail to read entry %lu\r\n", entry);
            continue;
        }

        if (header.LPA == LPA) {
            /* Found newer version */
            latest = entry;
        } else if (header.LPA == DATA_NONE8) {
            /* Empty entry */
            break;
        } else {
            mx_err("mxee_latst: corrupted entry, bank %lu, block %lu, entry %lu\r\n",
                bi->bank, bi->block, entry);
        }
    }

    /* Update L2PE mapping */
    if (latest != DATA_NONE32)
        bi->l2pe[LPA] = latest - ofs;

    /* Return latest entry */
    if (!free) {
        assert_param(latest != DATA_NONE32);
        return latest;
    }

    /* fred: For testing */
    if (bi->l2pe[LPA] > 0 && bi->l2pe[LPA] < MX_EEPROM_ENTRIES_PER_SECTOR - 1
        && bi->l2pe[LPA] + 1 != cnt) {
        mx_err("mxee_latst: alert!!! bank %lu, block %lu, LPA %lu, L2PS %u, L2PE %u, free %lu\r\n",
            bi->bank, bi->block, LPA, bi->l2ps[LPA], bi->l2pe[LPA], cnt);
        assert_param(0);
    }

    /* Return next free entry */
    if (cnt != MX_EEPROM_ENTRIES_PER_SECTOR)
        return entry;

    return DATA_NONE32;
}

/**
 * @brief    Scan current block to build mapping table.
 * @param    bi: Current bank handle
 * @param    block: Local block address
 * @retval Status
 */
static int mx_ee_build_mapping(struct bank_info *bi, uint32_t block) {
    struct eeprom_header header;
    uint32_t sector, entry, victim;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (block >= MX_EEPROM_BLOCKS))
        goto err;

    /* Reset L2P and P2L mappings */
    bi->block = block;
    bi->block_offset = block * MX_EEPROM_CLUSTER_SIZE + bi->bank_offset;
    memset(bi->l2ps, DATA_NONE8, sizeof(bi->l2ps));
    memset(bi->l2pe, 0, sizeof(bi->l2pe));
    memset(bi->p2l, DATA_NONE8, sizeof(bi->p2l));

    for (sector = 0; sector < MX_EEPROM_DATA_SECTORS; sector++) {
        entry = sector * MX_EEPROM_ENTRIES_PER_SECTOR;

        /* Read the first entry header of each sector */
        if (mx_ee_read(bi, entry, &header, true)) {
            mx_err("mxee_build: fail to read entry %lu\r\n", entry);

            /* XXX: Potential risk to erase? */
            victim = sector;
            goto erase;
        }

        /* Free sector */
        if (header.LPA == DATA_NONE8)
            continue;

        /* Update P2L mapping */
        bi->p2l[sector] = header.LPA;

        if (header.LPA < MX_EEPROM_LPAS_PER_CLUSTER) {
            /* Update L2P mapping */
            if (bi->l2ps[header.LPA] == DATA_NONE8) {
                bi->l2ps[header.LPA] = sector;
                continue;
            }

            /* Handle mapping conflict */
            victim = bi->l2ps[header.LPA];
            entry = mx_ee_find_latest(bi, header.LPA, true);
            if (entry / MX_EEPROM_ENTRIES_PER_SECTOR == victim)
                victim = sector;
        } else
            victim = sector;

        erase:
        /* Erase obsoleted sector */
        bi->dirty_block = block;
        bi->dirty_sector = victim;
        if (mx_ee_erase(bi))
            mx_err("mxee_build: fail to erase sector %lu\r\n", victim);

        /* Repair L2P mapping */
        if (victim != sector) {
            bi->l2ps[header.LPA] = sector;
            bi->l2pe[header.LPA] = 0;
        }
    }

    return MX_OK;
    err: bi->block = DATA_NONE32;
    bi->block_offset = DATA_NONE32;
    return MX_EINVAL;
}

/**
 * @brief    Find a free entry for given logical page.
 * @param    bi: Current bank handle
 * @param    LPA: Local logical page address
 * @retval Local free entry address
 */
static uint32_t mx_ee_search_free(struct bank_info *bi, uint32_t LPA) {
    uint32_t entry, sector, cnt;

    /* Check if corresponding sector used up */
    entry = mx_ee_find_latest(bi, LPA, true);
    if (entry < MX_EEPROM_ENTRIES_PER_CLUSTER)
        return entry;

    /* Pick a random sector to start the search */
    sector = rand() % MX_EEPROM_DATA_SECTORS;

    /* Search from the random sector to the end sector */
    for (cnt = sector; cnt < MX_EEPROM_DATA_SECTORS; cnt++) {
        if (bi->p2l[cnt] == DATA_NONE8) {
            entry = cnt * MX_EEPROM_ENTRIES_PER_SECTOR;
            return entry;
        }
    }

    /* Search from the start sector to the random sector */
    for (cnt = 0; cnt < sector; cnt++) {
        if (bi->p2l[cnt] == DATA_NONE8) {
            entry = cnt * MX_EEPROM_ENTRIES_PER_SECTOR;
            return entry;
        }
    }

    return DATA_NONE32;
}

/**
 * @brief    Read specified logical page of current block of current bank.
 * @param    bi: Current bank handle
 * @param    LPA: Local logical page address
 * @retval Status
 */
static int mx_ee_read_page(struct bank_info *bi, uint32_t LPA) {
    uint32_t entry;
    int ret, retries = 0;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS)
        || (LPA >= MX_EEPROM_LPAS_PER_CLUSTER)) {
        ret = MX_EINVAL;
        goto err;
    }

    /* Find the latest version */
    entry = mx_ee_find_latest(bi, LPA, false);

    /* Fresh read */
    if (entry >= MX_EEPROM_ENTRIES_PER_CLUSTER) {
        memset(bi->cache.data, DATA_NONE8, MX_EEPROM_PAGE_SIZE);
        bi->cache.header.LPA = LPA;
        return MX_OK;
    }

    retry:
    /* Do the real read */
    if (mx_ee_read(bi, entry, &bi->cache, false)
        || bi->cache.header.LPA != LPA) {
        mx_err("mxee_rpage: fail to read entry %lu\r\n", entry);

        if (retries++ < MX_EEPROM_READ_RETRIES) {
#ifdef MX_EEPROM_READ_ROLLBACK
            /* Roll back to the prior version */
            if (entry % MX_EEPROM_ENTRIES_PER_SECTOR)
                entry--;
#endif
            goto retry;
        }

        ret = MX_EIO;
        goto err;
    }

    return MX_OK;
    err: bi->cache_dirty = false;
    bi->cache.header.LPA = DATA_NONE8;
    return ret;
}

/**
 * @brief    Write specified logical page of current block of current bank.
 * @param    bi: Current bank handle
 * @param    LPA: Local logical page address
 * @retval Status
 */
static int mx_ee_write_page(struct bank_info *bi, uint32_t LPA) {
    uint32_t entry, ofs;
    int ret, retries = 0;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS)
            || (LPA >= MX_EEPROM_LPAS_PER_CLUSTER))
        return MX_EINVAL;

    retry:
    /* Find next free entry */
    entry = mx_ee_search_free(bi, LPA);
    if (entry >= MX_EEPROM_ENTRIES_PER_CLUSTER) {
        mx_err("mxee_wpage: no free entry left, bank %lu, block %lu\r\n",
                bi->bank, bi->block);
        return MX_ENOSPC;
    }

    /* Do the real write */
    ret = mx_ee_write(bi, entry, &bi->cache);
    if (ret) {
        mx_err("mxee_wpage: fail to write entry %lu\r\n", entry);

        if (retries++ < MX_EEPROM_WRITE_RETRIES)
            goto retry;

        return ret;
    }

    ofs = entry % MX_EEPROM_ENTRIES_PER_SECTOR;
    if (ofs) {
        /* Update L2E mapping */
        bi->l2pe[LPA] = ofs;
    } else {
        ofs = bi->l2ps[LPA];
        if (ofs < MX_EEPROM_DATA_SECTORS) {
            /* Obsolete sector */
            bi->dirty_block = bi->block;
            bi->dirty_sector = ofs;
        }

        /* Update L2P and P2L mapping */
        ofs = entry / MX_EEPROM_ENTRIES_PER_SECTOR;
        bi->l2ps[LPA] = ofs;
        bi->l2pe[LPA] = 0;
        bi->p2l[ofs] = LPA;
    }

    /* Clean page cache */
    bi->cache_dirty = false;

    return MX_OK;
}

/**
 * @brief    Handle buffer and cache.
 * @param    bi: Current bank handle
 * @param    addr: Local logical start address
 * @param    len: Request length
 * @param    buf: Data buffer
 * @param    rw: Read (false) or write (true)
 * @retval Status
 */
static int mx_ee_rw_buffer(struct bank_info *bi, uint32_t addr, uint32_t len, uint8_t *buf, bool rw) {
    int ret;
    uint32_t block, page, ofs;

    /* Calculate current block, page, offset */
    block = addr / MX_EEPROM_BLOCK_SIZE;
    ofs = addr % MX_EEPROM_BLOCK_SIZE;
    page = ofs / MX_EEPROM_PAGE_SIZE;
    ofs = ofs % MX_EEPROM_PAGE_SIZE;

    if ((bi->block != block) || (bi->cache.header.LPA != page)) {
        /* Flush dirty page cache */
        if (bi->cache_dirty) {
            ret = mx_ee_write_page(bi, bi->cache.header.LPA);
            if (ret) {
                mx_err("mxee_rwbuf: fail to flush page cache\r\n");
                return ret;
            }
        }

        /* Build mapping */
        if (bi->block != block) {
            ret = mx_ee_build_mapping(bi, block);
            if (ret) {
                mx_err("mxee_rwbuf: fail to build mapping table, bank %lu, block %lu\r\n",
                    bi->bank, bi->block);
                return ret;
            }
        }

        /* Fill page cache */
        if (!rw || len < MX_EEPROM_PAGE_SIZE) {
            ret = mx_ee_read_page(bi, page);
            if (ret) {
                mx_err("mxee_rwbuf: fail to fill page cache\r\n");
                bi->cache.header.LPA = DATA_NONE8;
                return ret;
            }
        } else
            bi->cache.header.LPA = page;
    }

    /* Update page cache/buffer */
    if (rw) {
        memcpy(&bi->cache.data[ofs], buf, len);
        bi->cache_dirty = true;
    } else
        memcpy(buf, &bi->cache.data[ofs], len);

    /* Handle obsoleted sector */
    if (mx_ee_erase(bi))
        mx_err("mxee_rwbuf: fail to erase\r\n");

    return MX_OK;
}

/**
 * @brief    Distribute continuous logical address into different banks.
 * @param    addr: Global logical start address
 * @param    len: Request length
 * @param    buf: Data buffer
 * @param    rw: Read (false) or write (true)
 * @retval Status
 */
#if (MX_EEPROM_HASH_AlGORITHM == MX_EEPROM_HASH_CROSSBANK)

static int mx_ee_rw(uint32_t addr, uint32_t len, uint8_t *buf, bool rw) {
    int ret;
    struct bank_info *bi;
    uint32_t page, ofs, bank, rwpos, rwlen;

    if (addr + len > MX_EEPROM_TOTAL_SIZE)
        return MX_EINVAL;

    /* Determine the rwpos and rwlen */
    page = addr / MX_EEPROM_PAGE_SIZE;
    ofs = addr % MX_EEPROM_PAGE_SIZE;
    bank = page % MX_EEPROMS;
    page = page / MX_EEPROMS;
    rwpos = page * MX_EEPROM_PAGE_SIZE + ofs;
    rwlen = min_t(uint32_t, MX_EEPROM_PAGE_SIZE - ofs, len);

    /* Loop to R/W each logical page */
    while (len) {
        bi = &mx_eeprom.bi[bank];

        /* Only allow one request per bank per time */
        if (osMutexWait(bi->lock, osWaitForever))
            return MX_EOS;

        ret = mx_ee_rw_buffer(bi, rwpos, rwlen, buf, rw);

        osMutexRelease(bi->lock);

        if (ret) {
            mx_err("mxee_toprw: fail to %s laddr 0x%08lx, len %lu\r\n", rw ? "write" : "read", addr, rwlen);
            return ret;
        }

        /* Calculate the next rwpos and rwlen */
        buf += rwlen;
        addr += rwlen;
        len -= rwlen;

        if (ofs) {
            rwpos -= ofs;
            ofs = 0;
        }

        if (++bank == MX_EEPROMS) {
            bank = 0;
            rwpos += MX_EEPROM_PAGE_SIZE;
        }

        rwlen = min_t(uint32_t, MX_EEPROM_PAGE_SIZE, len);
    }

    return MX_OK;
}

#elif (MX_EEPROM_HASH_AlGORITHM == MX_EEPROM_HASH_HYBRID)

#define MX_EEPROM_SUPERBLOCK_SIZE        (MX_EEPROM_BLOCK_SIZE * MX_EEPROMS)
static int mx_ee_rw(uint32_t addr, uint32_t len, uint8_t *buf, bool rw)
{
    int ret;
    struct bank_info *bi;
    uint32_t ofs, bank, base, size, rwpos, rwlen;

    /* Determine the rwpos and rwlen */
    ofs = addr / MX_EEPROM_BLOCK_SIZE;
    bank = ofs % MX_EEPROMS;
    base = (ofs / MX_EEPROMS) * MX_EEPROM_BLOCK_SIZE;
    size = addr % MX_EEPROM_BLOCK_SIZE;
    rwpos = base + size;
    ofs = size % MX_EEPROM_PAGE_SIZE;
    rwlen = min_t(uint32_t, MX_EEPROM_PAGE_SIZE - ofs, len);
    size = MX_EEPROM_BLOCK_SIZE - size;

    /* Loop to R/W each logical page */
    while (len)
    {
        bi = &mx_eeprom.bi[bank];

        /* Only allow one request per bank per time */
        if (osMutexWait(bi->lock, osWaitForever))
            return MX_EOS;

        while (size && len)
        {
            ret = mx_ee_rw_buffer(bi, rwpos, rwlen, buf, rw);
            if (ret)
            {
                mx_err("mxee_toprw: fail to %s laddr 0x%08lx, len %lu\r\n",
                    rw ? "write" : "read", addr, rwlen);
                osMutexRelease(bi->lock);
                return ret;
            }

            buf += rwlen;
            rwpos += rwlen;
            addr += rwlen;
            size -= rwlen;
            len -= rwlen;
            rwlen = min_t(uint32_t, MX_EEPROM_PAGE_SIZE, len);
        }

        osMutexRelease(bi->lock);

        if (++bank == MX_EEPROMS)
        {
            bank = 0;
            base += MX_EEPROM_BLOCK_SIZE;
        }

        rwpos = base;
        size = MX_EEPROM_BLOCK_SIZE;
    }

    return MX_OK;
}

#elif (MX_EEPROM_HASH_AlGORITHM == MX_EEPROM_HASH_SEQUENTIAL)

static int mx_ee_rw(uint32_t addr, uint32_t len, uint8_t *buf, bool rw)
{
    int ret;
    struct bank_info *bi;
    uint32_t ofs, bank, rwpos, rwlen;

    if (addr + len > MX_EEPROM_TOTAL_SIZE)
        return MX_EINVAL;

    /* Determine the rwpos and rwlen */
    bank = addr / MX_EEPROM_SIZE;
    ofs = addr % MX_EEPROM_SIZE;
    rwpos = ofs;
    ofs = ofs % MX_EEPROM_PAGE_SIZE;
    rwlen = min_t(uint32_t, MX_EEPROM_PAGE_SIZE - ofs, len);

    /* Loop to R/W each logical page */
    while (len)
    {
        bi = &mx_eeprom.bi[bank];

        /* Only allow one request per bank per time */
        if (osMutexWait(bi->lock, osWaitForever))
            return MX_EOS;

        do
        {
            ret = mx_ee_rw_buffer(bi, rwpos, rwlen, buf, rw);
            if (ret)
            {
                mx_err("mxee_toprw: fail to %s laddr 0x%08lx, len %lu\r\n",
                    rw ? "write" : "read", addr, rwlen);
                osMutexRelease(bi->lock);
                return ret;
            }

            /* Calculate the next rwpos and rwlen */
            buf += rwlen;
            addr += rwlen;
            rwpos += rwlen;
            len -= rwlen;
            rwlen = min_t(uint32_t, MX_EEPROM_PAGE_SIZE, len);

        } while (len && (rwpos != MX_EEPROM_SIZE));

        osMutexRelease(bi->lock);

        bank++;
        rwpos = 0;
    }

    return MX_OK;
}

#endif

/**
 * @brief    EEPROM read API.
 * @param    addr: Start address
 * @param    len: Request length
 * @param    buf: Data buffer
 * @retval Status
 */
static int mx_eeprom_read(uint32_t addr, uint32_t len, uint8_t *buf) {
    if (!mx_eeprom.initialized)
        return MX_ENODEV;

    return mx_ee_rw(addr, len, buf, false);
}

/**
 * @brief    EEPROM write API.
 * @param    addr: Start address
 * @param    len: Request length
 * @param    buf: Data buffer
 * @retval Status
 */
static int mx_eeprom_write(uint32_t addr, uint32_t len, uint8_t *buf) {
    if (!mx_eeprom.initialized)
        return MX_ENODEV;

    return mx_ee_rw(addr, len, buf, true);
}

/**
 * @brief    Write dirty cache back (For internal use only).
 * @param    bi: Current bank handle
 * @retval Status
 */
static int mx_eeprom_wb(struct bank_info *bi) {
    /* Write page cache back */
    if (mx_ee_write_page(bi, bi->cache.header.LPA)) {
        mx_err("mxee_wback: fail to flush page cache\r\n");
        return MX_EIO;
    }

    /* Handle obsoleted sector */
    if (mx_ee_erase(bi)) {
        mx_err("mxee_wback: fail to erase\r\n");
        return MX_EIO;
    }

    return MX_OK;
}

/**
 * @brief    EEPROM cache write back API.
 * @retval Status
 */
static int mx_eeprom_write_back(void) {
    uint32_t bank;
    int ret = MX_OK;
    struct bank_info *bi;

    if (!mx_eeprom.initialized)
        return MX_ENODEV;

    /* Loop to check each bank */
    for (bank = 0; bank < MX_EEPROMS; bank++) {
        bi = &mx_eeprom.bi[bank];

        /* Check dirty cache */
        if (!bi->cache_dirty)
            continue;

        if (osMutexWait(bi->lock, osWaitForever))
            return MX_EOS;

        /* Double check */
        if (bi->cache_dirty) {
            /* Write back */
            if (mx_eeprom_wb(bi)) {
                mx_err("mxee_wback: fail to write back\r\n");
                ret = MX_EIO;
            }
        }

        osMutexRelease(bi->lock);
    }

    return ret;
}

/**
 * @brief    EEPROM sync write API.
 * @param    addr: Start address
 * @param    len: Request length
 * @param    buf: Data buffer
 * @retval Status
 */
static int mx_eeprom_sync_write(uint32_t addr, uint32_t len, uint8_t *buf) {
    return (mx_eeprom_write(addr, len, buf) || mx_eeprom_write_back());
}

/**
 * @brief    EEPROM user cache and meta data flush API.
 *                 NOTE: Call this API just before power down.
 * @retval Status
 */
static int mx_eeprom_flush(void) {
    int ret = MX_OK;
#ifdef MX_EEPROM_PC_PROTECTION
    struct bank_info *bi;
    uint32_t bank, block;
#endif

    ret = mx_eeprom_write_back();
    if (ret)
        return ret;

#ifdef MX_EEPROM_PC_PROTECTION
    /* Update system entry */
    for (bank = 0; bank < MX_EEPROMS; bank++)
    {
        bi = &mx_eeprom.bi[bank];

        for (block = 0; block < MX_EEPROM_BLOCKS; block++)
        {
            if (osMutexWait(bi->lock, osWaitForever))
                return MX_EOS;

            if (mx_ee_update_sys(bi, block, OPS_NONE, DATA_NONE32))
            {
                mx_err("mxee_flush: fail to update system entry\r\n");
                ret = MX_EIO;
            }

            osMutexRelease(bi->lock);
        }
    }
#endif

    return ret;
}

/**
 * @brief    Handle wear leveling.
 * @retval Status
 */
static int mx_eeprom_wear_leveling(void) {
    int ret = MX_OK;
    struct bank_info *bi;
    static uint32_t rwCnt = 0;
    uint32_t bank, page, sector;

    /* Do wear leveling at fixed frequency */
    if (mx_eeprom.rwCnt - rwCnt < MX_EEPROM_WL_INTERVAL) {
        rwCnt = mx_eeprom.rwCnt;
        return MX_OK;
    }

    /* Choose a random logical page */
    bank = rand() % MX_EEPROMS;
    page = rand() % MX_EEPROM_LPAS_PER_CLUSTER;

    bi = &mx_eeprom.bi[bank];

    /* Get current bank lock */
    if (osMutexWait(bi->lock, osWaitForever))
        return MX_EOS;

    /* Skip unmapped page */
    sector = bi->l2ps[page];
    if (sector >= MX_EEPROM_DATA_SECTORS)
        goto out;

    if (bi->cache_dirty) {
        /* Skip the current dirty page */
        if (bi->cache.header.LPA == page)
            goto out;

        /* Flush dirty cache first */
        ret = mx_eeprom_wb(bi);
        if (ret) {
            mx_err("mxee_wearl: fail to clean cache\r\n");
            goto out;
        }
    }

    /* Read the page out */
    ret = mx_ee_read_page(bi, page);
    if (ret) {
        mx_err("mxee_wearl: fail to read page %lu\r\n", page);
        bi->cache.header.LPA = DATA_NONE8;
        goto out;
    }

    /* Set cache dirty */
    bi->cache_dirty = true;

    /* Cheat the free entry selector */
    bi->l2pe[page] = MX_EEPROM_ENTRIES_PER_SECTOR - 1;

    /* Flush the dirty cache */
    ret = mx_eeprom_wb(bi);
    if (ret)
        mx_err("mxee_wearl: fail to write back\r\n");

    out:
    /* Release current bank lock */
    osMutexRelease(bi->lock);

    return ret;
}

/**
 * @brief    EEPROM background task API.
 *                 NOTE: Call this API just before MCU sleep.
 * @param    always: Do the bg task periodically (true) or only once (false).
 */
static void mx_eeprom_background(const void *always) {
#ifdef MX_EEPROM_BACKGROUND_THREAD
    uint32_t cnt, prevTime;
    bool threadMode = *(bool *)always;

    prevTime = osKernelSysTick();

    for (cnt = 0; ; cnt++)
    {
        mx_info("mxee_bTask: wake-up times: %lu\r\n", cnt);
#endif
    /* Flush dirty page cache */
    if (mx_eeprom_write_back())
        mx_err("mxee_bTask: fail to flush cache\r\n");

    /* Check Wear eveling */
    if (mx_eeprom_wear_leveling())
        mx_err("mxee_bTask: fail to WL\r\n");

#ifdef MX_EEPROM_BACKGROUND_THREAD
        if (!(*(bool *)always))
            break;

        /* Wake up periodically */
        osDelayUntil(&prevTime, MX_EEPROM_BG_THREAD_DELAY);
    }

    if (threadMode)
    {
        mx_info("mxee_bTask: Goodbye\r\n");

        /* Terminate itself */
        osThreadTerminate(NULL);
    }
#endif
}

#ifdef MX_EEPROM_PC_PROTECTION
/**
  * @brief    Handle insufficient sector erase.
  * @param    bi: Current bank handle
  * @param    sector: Victim sector address
  * @retval Status
*/
static int mx_ee_check_erase(struct bank_info *bi, uint32_t sector)
{
    uint32_t entry;
    struct eeprom_header header;

    /* Check address validity */
    if ((bi->bank >= MX_EEPROMS) || (bi->block >= MX_EEPROM_BLOCKS) ||
        (sector >= MX_EEPROM_DATA_SECTORS))
        return MX_EINVAL;

    /* XXX: Only check the first and last entry? */

    /* Check the first entry header */
    entry = sector * MX_EEPROM_ENTRIES_PER_SECTOR;
    if (mx_ee_read(bi, entry, &header, true) ||
         (header.LPA != DATA_NONE8 && header.LPA >= MX_EEPROM_LPAS_PER_CLUSTER))
    {
        mx_err("mxee_ckers: fail to read entry 0\r\n");
        goto erase;
    }

    /* Check the last entry header */
    entry += MX_EEPROM_ENTRIES_PER_SECTOR - 1;
    if (mx_ee_read(bi, MX_EEPROM_ENTRIES_PER_SECTOR - 1, &header, true) ||
         (header.LPA != DATA_NONE8))
    {
        mx_err("mxee_ckers: fail to read entry %d\r\n", MX_EEPROM_ENTRIES_PER_SECTOR - 1);
        goto erase;
    }

    return MX_OK;

erase:
    mx_info("mxee_ckers: detected insufficient erase, block %lu, sector %lu\r\n",
        bi->block, sector);
    bi->dirty_block = bi->block;
    bi->dirty_sector = sector;
    return mx_ee_erase(bi);
}
#endif

/**
 * @brief    Check system info and handle power cycling.
 * @retval Status
 */
static int mx_ee_check_sys(void) {
    uint32_t bank, block;
    bool formatted = false;
    struct system_entry sys;

#ifdef MX_EEPROM_PC_PROTECTION
    struct bank_info *bi;
    uint32_t entry, lowerBound, upperBound;

    /* Loop to check each bank */
    for (bank = 0; bank < MX_EEPROMS; bank++)
    {
        bi = &mx_eeprom.bi[bank];
        bi->block = 0;
        bi->block_offset = bi->bank_offset;

        /* Loop to check each block */
        for (block = 0; block < MX_EEPROM_BLOCKS; block++)
        {
            lowerBound = 0;
            upperBound = MX_EEPROM_SYSTEM_ENTRIES - 1;

            while (lowerBound < upperBound)
            {
                /* Select the middle entry */
                entry = (lowerBound + upperBound) / 2;
                if (entry == lowerBound)
                    entry++;

retry:
                /* Read system entry */
                if (mx_ee_read_sys(bi, entry, &sys))
                {
                    mx_err("mxee_cksys: fail to read system entry %lu\r\n", entry);
                    goto err;
                }

                /* Check entry format */
                if (sys.id == MFTL_ID)
                {
                    /* Valid entry */
                    formatted = true;
                    lowerBound = entry;
                    continue;
                }
                else if (sys.ops == DATA_NONE16 && sys.arg == DATA_NONE16)
                {
                    /* Empty entry */
                    upperBound = entry - 1;
                    continue;
                }

err:
                mx_err("mxee_cksys: corrupted system entry %lu\r\n", entry);

                /* XXX: Potential risk? Try sequential search? */
                if (entry < upperBound)
                {
                    entry++;
                    goto retry;
                }

                lowerBound = upperBound;
            }

            /* Read the latest entry */
            bi->sys_entry[block] = lowerBound;
            if (!mx_ee_read_sys(bi, lowerBound, &sys))
            {
                if (sys.id == MFTL_ID)
                    formatted = true;

                /* Check insufficient erase */
                if (sys.ops == OPS_ERASE_BEGIN)
                    mx_ee_check_erase(bi, sys.arg);
            }

            /* Next block */
            bi->block++;
            bi->block_offset += MX_EEPROM_CLUSTER_SIZE;
        }

        /* Clean up */
        bi->block = DATA_NONE32;
        bi->block_offset = DATA_NONE32;
        bi->dirty_block = DATA_NONE32;
        bi->dirty_sector = DATA_NONE32;
    }
#else
    uint32_t addr;

    /* Loop to check each bank */
    for (bank = 0; bank < MX_EEPROMS; bank++) {
        addr = bank_offset[bank] + MX_EEPROM_SYSTEM_SECTOR_OFFSET;

        /* Loop to check each block */
        for (block = 0; block < MX_EEPROM_BLOCKS; block++) {
            if (mx_ee_rww_read(addr, sizeof(sys), (uint8_t*) &sys)) {
                mx_err("mxee_formt: fail to read addr 0x%08lx\r\n", addr);
                continue;
            }

            /* Check entry format */
            if ((sys.id == MFTL_ID) && (sys.cksum == (sys.id ^ sys.ops ^ sys.arg))) {
                formatted = true;
                break;
            }

            addr += MX_EEPROM_CLUSTER_SIZE;
        }

        if (formatted)
            break;
    }
#endif

    return formatted ? MX_OK : MX_ENOFS;
}

/**
 * @brief    Format EEPROM Emulator.
 * @retval Status
 */
static int mx_eeprom_format(void) {
    struct system_entry sys;
    uint32_t i, j, k, sector, addr;

    /* Should not format EEPROM after init */
    if (mx_eeprom.initialized)
        return MX_EPERM;

    /* Loop to erase each sector */
    for (i = 0; i < MX_EEPROMS; i++) {
        addr = bank_offset[i];

        for (j = 0; j < MX_EEPROM_BLOCKS; j++) {
            sector = addr / MX_FLASH_SECTOR_SIZE;

            for (k = 0; k < MX_EEPROM_SECTORS_PER_CLUSTER; k++, sector++) {
                if (mx_ee_rww_erase(sector * MX_FLASH_SECTOR_SIZE, MX_FLASH_SECTOR_SIZE)) {
                    mx_err("mxee_formt: fail to erase sector %lu\r\n", sector);
                    return MX_EIO;
                }
            }

            addr += MX_EEPROM_CLUSTER_SIZE;
        }
    }

    /* Fill system entry */
    sys.id = MFTL_ID;
    sys.ops = OPS_NONE;
    sys.arg = DATA_NONE16;
    sys.cksum = sys.id ^ sys.ops ^ sys.arg;

    /* Write RWWEE ID (Actually no need to write every block) */
    for (i = 0; i < MX_EEPROMS; i++) {
        addr = bank_offset[i] + MX_EEPROM_SYSTEM_SECTOR_OFFSET;

        for (j = 0; j < MX_EEPROM_BLOCKS; j++) {
            if (mx_ee_rww_write(addr, sizeof(sys), (uint8_t*) &sys)) {
                mx_err("mxee_formt: fail to write addr 0x%08lx\r\n", addr);
                return MX_EIO;
            }

            addr += MX_EEPROM_CLUSTER_SIZE;
        }
    }

    return MX_OK;
}

/**
 * @brief    Handle insufficient sector erase.
 * @param    param: eeprom_param structure pointer
 */
static void mx_eeprom_get_param(struct eeprom_param *param) {
    param->eeprom_page_size = MX_EEPROM_PAGE_SIZE;
    param->eeprom_bank_size = MX_EEPROM_SIZE;
    param->eeprom_banks = MX_EEPROMS;
    param->eeprom_total_size = MX_EEPROM_TOTAL_SIZE;
    param->eeprom_hash_algorithm = MX_EEPROM_HASH_AlGORITHM;
}

/**
 * @brief    Initialize EEPROM Emulator.
 * @retval Status
 */
static int mx_eeprom_init(void) {
    int ret;
    uint32_t bank, ofs;

    osMutexDef(MUTEX);

    if (mx_eeprom.initialized)
        return MX_OK;

    for (bank = 0; bank < MX_EEPROMS; bank++) {
        /* Check address validity */
#ifdef MX_FLASH_SUPPORT_RWW
        ofs = bank_offset[bank];
        if ((ofs / MX_FLASH_BANK_SIZE != bank) ||
             ((ofs + MX_EEPROM_BANK_SIZE - 1) / MX_FLASH_BANK_SIZE != bank))
        {
            mx_err("mxee_init : invalid bank%lu offset\r\n", bank);
            ret = MX_EINVAL;
            goto err0;
        }
#else
        for (ofs = bank + 1; ofs < MX_EEPROMS; ofs++) {
            if (abs(bank_offset[ofs] - bank_offset[bank]) < MX_EEPROM_BANK_SIZE) {
                mx_err("mxee_init : offset conflict: bank%lu, bank %lu\r\n", bank, ofs);
                ret = MX_EINVAL;
                goto err0;
            }
        }
#endif

        /* Init bank info */
        mx_eeprom.bi[bank].bank = bank;
        mx_eeprom.bi[bank].bank_offset = bank_offset[bank];

        /* Empty current block info */
        mx_eeprom.bi[bank].block = DATA_NONE32;
        mx_eeprom.bi[bank].block_offset = DATA_NONE32;

        /* Empty current page cache */
        mx_eeprom.bi[bank].cache_dirty = false;
        memset(&mx_eeprom.bi[bank].cache, DATA_NONE8, MX_EEPROM_ENTRY_SIZE);

        /* No obsoleted sector */
        mx_eeprom.bi[bank].dirty_block = DATA_NONE32;
        mx_eeprom.bi[bank].dirty_sector = DATA_NONE32;

        /* Init bank mutex lock */
        mx_eeprom.bi[bank].lock = osMutexCreate(osMutex(MUTEX));
        if (!mx_eeprom.bi[bank].lock) {
            mx_err("mxee_init : out of memory (bankLock)\r\n");
            ret = MX_ENOMEM;
            goto err1;
        }

#ifdef MX_DEBUG
        /* Reset erase count statistics */
        memset(mx_eeprom.bi[bank].eraseCnt, 0, sizeof(mx_eeprom.bi[bank].eraseCnt));
#endif
    }

    /* Reset R/W counter */
    mx_eeprom.rwCnt = 0;

#ifdef MX_EEPROM_CRC_HW
    /* Init HW CRC */
    hcrc.Instance = CRC;
    hcrc.Init.GeneratingPolynomial = CRC16_POLY;
    hcrc.Init.CRCLength = CRC_POLYLENGTH_16B;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;

    if (HAL_CRC_Init(&hcrc)) {
        mx_err("mxee_init : fail to init HW CRC\r\n");
        ret = MX_ENXIO;
        goto err2;
    }

    /* Init CRC mutex lock */
    mx_eeprom.crcLock = osMutexCreate(osMutex(MUTEX));
    if (!mx_eeprom.crcLock) {
        mx_err("mxee_init : out of memory (crcLock)\r\n");
        ret = MX_ENOMEM;
        goto err2;
    }
#endif

    /* Check RWWEE format */
    ret = mx_ee_check_sys();
    if (ret) {
        mx_err("mxee_init : not found valid RWWEE format\r\n");
        goto err4;
    }

#ifdef MX_EEPROM_BACKGROUND_THREAD
    /* Start background thread */
    mx_info("mxee_init : starting background thread\r\n");
    mx_eeprom.bgStart = true;
    osThreadDef(eeback, mx_eeprom_background, MX_EEPROM_BG_THREAD_PRIORITY, 0,
                            MX_EEPROM_BG_THREAD_STACK_SIZE);
    mx_eeprom.bgThreadID = osThreadCreate(osThread(eeback), &mx_eeprom.bgStart);

    if (!mx_eeprom.bgThreadID)
    {
        mx_err("mxee_init : fail to start background thread\r\n");
        mx_eeprom.bgStart = false;
    }
#endif

    /* Init done */
    mx_eeprom.initialized = true;

    return MX_OK;
    err4:
    err3:
#ifdef MX_EEPROM_CRC_HW
    osMutexDelete(mx_eeprom.crcLock);
    mx_eeprom.crcLock = NULL;
    err2: HAL_CRC_DeInit(&hcrc);
#endif
    err1: for (bank--; bank < MX_EEPROMS; bank--) {
        osMutexDelete(mx_eeprom.bi[bank].lock);
        mx_eeprom.bi[bank].lock = NULL;
    }
    err0: return ret;
}

/**
 * @brief    Deinit EEPROM Emulator.
 */
static void mx_eeprom_deinit(void) {
    uint32_t cnt;

    if (!mx_eeprom.initialized)
        return;

    /* Block further user request */
    mx_eeprom.initialized = false;

#ifdef MX_EEPROM_BACKGROUND_THREAD
    /* Stop background thread */
    if (mx_eeprom.bgStart)
    {
        mx_info("mxee_deini: stopping the background thread\r\n");

        /* Terminate the background thread */
        mx_eeprom.bgStart = false;
        cnt = osKernelSysTick();
        while (osThreadGetState(mx_eeprom.bgThreadID) != osThreadDeleted)
        {
            if (osKernelSysTick() - cnt > MX_EEPROM_BG_THREAD_TIMEOUT)
            {
                mx_err("mxee_deini: killing the background thread\r\n");
                osThreadTerminate(mx_eeprom.bgThreadID);
                break;
            }

            osDelay(1);
        }

        mx_eeprom.bgThreadID = NULL;
    }
#endif

    for (cnt = 0; cnt < MX_EEPROMS; cnt++) {
        /* Wait for the current request to finish */
        osMutexWait(mx_eeprom.bi[cnt].lock, osWaitForever);

        /* Delete bank mutex lock */
        osMutexDelete(mx_eeprom.bi[cnt].lock);
        mx_eeprom.bi[cnt].lock = NULL;
    }

#ifdef MX_EEPROM_CRC_HW
    /* Deinit HW CRC */
    HAL_CRC_DeInit(&hcrc);

    /* Delete CRC mutex lock */
    osMutexDelete(mx_eeprom.crcLock);
    mx_eeprom.crcLock = NULL;
#endif
}

struct eeprom_api eeprom_api1 = { .mx_eeprom_format = mx_eeprom_format,
        .mx_eeprom_init = mx_eeprom_init, .mx_eeprom_deinit = mx_eeprom_deinit,
        .mx_eeprom_read = mx_eeprom_read, .mx_eeprom_write = mx_eeprom_write,
        .mx_eeprom_sync_write = mx_eeprom_sync_write, .size =
                MX_EEPROM_TOTAL_SIZE };
