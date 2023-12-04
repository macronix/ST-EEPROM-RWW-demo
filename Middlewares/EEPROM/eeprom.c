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

#include <rwwee.h>
#include "string.h"
#include "stdlib.h"

extern struct eeprom_api eeprom_api1;
extern struct eeprom_api eeprom_api2;

extern int mx_ee_rww_init(void);
extern void mx_ee_rww_deinit(void);

/**
 * @brief  EEPROM read API.
 * @param  addr: Start address
 * @param  len: Request length
 * @param  buf: Data buffer
 * @retval Status
 */
int mx_eeprom_read(uint32_t addr, uint32_t len, uint8_t *buf) {
    int ret;

    if (addr < eeprom_api2.offset)
        ret = eeprom_api1.mx_eeprom_read(addr, len, buf);
    else
        ret = eeprom_api2.mx_eeprom_read(addr - eeprom_api2.offset, len, buf);

    return ret;
}
/**
 * @brief  EEPROM write API.
 * @param  addr: Start address
 * @param  len: Request length
 * @param  buf: Data buffer
 * @retval Status
 */
int mx_eeprom_write(uint32_t addr, uint32_t len, uint8_t *buf) {
    int ret;

    if (addr < eeprom_api2.offset)
        ret = eeprom_api1.mx_eeprom_write(addr, len, buf);
    else
        ret = eeprom_api2.mx_eeprom_write(addr - eeprom_api2.offset, len, buf);

    return ret;
}

/**
 * @brief  EEPROM sync write API.
 * @param  addr: Start address
 * @param  len: Request length
 * @param  buf: Data buffer
 * @retval Status
 */
int mx_eeprom_sync_write(uint32_t addr, uint32_t len, uint8_t *buf) {
    int ret;

    if (addr < eeprom_api2.offset)
        ret = eeprom_api1.mx_eeprom_sync_write(addr, len, buf);
    else
        ret = eeprom_api2.mx_eeprom_sync_write(addr - eeprom_api2.offset, len, buf);

    return ret;
}

/**
 * @brief  Initialize EEPROM Emulator.
 * @retval Status
 */
int mx_eeprom_init(void) {
    int ret;

    ret = mx_ee_rww_init();
    if (ret) {
        printf("mxee_init : fail to init RWW layer\r\n");
        return ret;
    }

    eeprom_api1.offset = 0;
    printf("eeprom1 for demo, offs %x, size %x\r\n", eeprom_api1.offset, eeprom_api1.size);
    eeprom_api2.offset = 0x80000000;
    printf("eeprom2 for perf, offs %x, size %x\r\n", eeprom_api2.offset, eeprom_api2.size);

    ret = eeprom_api1.mx_eeprom_init();
    if (ret)
        return ret;

    ret = eeprom_api2.mx_eeprom_init();
    if (ret)
        return ret;

    return 0;
}

/**
 * @brief  Deinit EEPROM Emulator.
 */
void mx_eeprom_deinit(void) {
    eeprom_api1.mx_eeprom_deinit();
    eeprom_api2.mx_eeprom_deinit();
}

/**
 * @brief  Format EEPROM Emulator.
 * @retval Status
 */
int mx_eeprom_format(void) {
    int ret;

    mx_eeprom_deinit();

    ret = eeprom_api1.mx_eeprom_format();
    if (ret)
        return ret;

    ret = eeprom_api2.mx_eeprom_format();
    if (ret)
        return ret;

    return 0;
}
