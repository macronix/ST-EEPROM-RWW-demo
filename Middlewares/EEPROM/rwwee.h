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

#ifndef RWWEE2_H_
#define RWWEE2_H_

#include "stdbool.h"
#include "stdio.h"

struct eeprom_api {
    int (*mx_eeprom_format)(void);
    int (*mx_eeprom_init)(void);
    void (*mx_eeprom_deinit)(void);
    int (*mx_eeprom_read)(uint32_t addr, uint32_t len, uint8_t *buf);
    int (*mx_eeprom_write)(uint32_t addr, uint32_t len, uint8_t *buf);
    int (*mx_eeprom_sync_write)(uint32_t addr, uint32_t len, uint8_t *buf);
    uint32_t offset;
    uint32_t size;
};

/* eeprom.c */
int mx_eeprom_read(uint32_t addr, uint32_t len, uint8_t *buf);
int mx_eeprom_write(uint32_t addr, uint32_t len, uint8_t *buf);
int mx_eeprom_sync_write(uint32_t addr, uint32_t len, uint8_t *buf);
int mx_eeprom_format(void);
int mx_eeprom_init(void);
void mx_eeprom_deinit(void);

#endif /* RWWEE2_H_ */
