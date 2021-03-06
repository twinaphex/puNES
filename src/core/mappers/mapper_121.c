/*
 *  Copyright (C) 2010-2016 Fabio Cavallo (aka FHorse)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include "mappers.h"
#include "info.h"
#include "mem_map.h"
#include "cpu.h"
#include "irqA12.h"
#include "save_slot.h"

#define m121_swap_8k_prg()\
	if (m121.reg[0]) {\
		value = m121.reg[0];\
		control_bank(info.prg.rom.max.banks_8k)\
		map_prg_rom_8k(1, 2, value);\
	} else {\
		mapper.rom_map_to[2] = m121.bck[1];\
	}\
	if (m121.reg[1]) {\
		value = m121.reg[1];\
		control_bank(info.prg.rom.max.banks_8k)\
		map_prg_rom_8k(1, 3, value);\
	} else {\
		mapper.rom_map_to[3] = info.prg.rom.max.banks_8k;\
	}\
	map_prg_rom_8k_update()

static const BYTE vlu121[4] = { 0x00, 0x83, 0x42, 0x00 };

void map_init_121(void) {
	EXTCL_CPU_WR_MEM(121);
	EXTCL_CPU_RD_MEM(121);
	EXTCL_SAVE_MAPPER(121);
	EXTCL_CPU_EVERY_CYCLE(MMC3);
	EXTCL_PPU_000_TO_34X(MMC3);
	EXTCL_PPU_000_TO_255(MMC3);
	EXTCL_PPU_256_TO_319(MMC3);
	EXTCL_PPU_320_TO_34X(MMC3);
	EXTCL_UPDATE_R2006(MMC3);
	mapper.internal_struct[0] = (BYTE *) &m121;
	mapper.internal_struct_size[0] = sizeof(m121);
	mapper.internal_struct[1] = (BYTE *) &mmc3;
	mapper.internal_struct_size[1] = sizeof(mmc3);

	info.mapper.extend_wr = TRUE;

	if (info.reset >= HARD) {
		memset(&m121, 0x00, sizeof(m121));
		memset(&mmc3, 0x00, sizeof(mmc3));

		m121.bck[0] = mapper.rom_map_to[0];
		m121.bck[1] = mapper.rom_map_to[2];
	}

	memset(&irqA12, 0x00, sizeof(irqA12));

	irqA12.present = TRUE;
	irqA12_delay = 1;
}
void extcl_cpu_wr_mem_121(WORD address, BYTE value) {
	if (address >= 0x8000) {
		const BYTE prg_rom_cfg = (value & 0x40) >> 5;

		extcl_cpu_wr_mem_MMC3(address, value);

		switch (address & 0xE003) {
			case 0x8000: {
				if (mmc3.prg_rom_cfg != prg_rom_cfg) {
					mapper.rom_map_to[2] = m121.bck[0];
					mapper.rom_map_to[0] = m121.bck[1];
					m121.bck[0] = mapper.rom_map_to[0];
					m121.bck[1] = mapper.rom_map_to[2];
				}
				m121_swap_8k_prg();
				break;
			}
			case 0x8001:
				if (mmc3.bank_to_update == 6) {
					if (mmc3.prg_rom_cfg) {
						control_bank(info.prg.rom.max.banks_8k)
						m121.bck[1] = value;
					} else {
						control_bank(info.prg.rom.max.banks_8k)
						m121.bck[0] = value;
					}
				}
				m121_swap_8k_prg();
				break;
			case 0x8003:
				switch (value) {
					case 0x20:
						m121.reg[1] = 0x13;
						break;
					case 0x29:
						m121.reg[1] = 0x1B;
						break;
					case 0x28:
						m121.reg[0] = 0x0C;
						break;
					case 0x26:
						m121.reg[1] = 0x08;
						break;
					case 0xAB:
						m121.reg[1] = 0x07;
						break;
					case 0xEC:
					case 0xEF:
						m121.reg[1] = 0x0D;
						break;
					case 0xFF:
						m121.reg[1] = 0x09;
						break;
					default:
						m121.reg[0] = m121.reg[1] = 0;
						break;
				}
				m121_swap_8k_prg();
				break;
		}
		return;
	}

	if ((address < 0x5000) || (address > 0x5FFF)) {
		return;
	}

	m121.reg[2] = vlu121[value & 0x03];
	return;
}
BYTE extcl_cpu_rd_mem_121(WORD address, BYTE openbus, BYTE before) {
	if ((address < 0x5000) || (address > 0x5FFF)) {
		return (openbus);
	}

	return (m121.reg[2]);
}
BYTE extcl_save_mapper_121(BYTE mode, BYTE slot, FILE *fp) {
	if (save_slot.version < 6) {
		if (mode == SAVE_SLOT_READ) {
			BYTE old_prg_rom_bank[2], i;

			save_slot_ele(mode, slot, old_prg_rom_bank)

			for (i = 0; i < 2; i++) {
				m121.bck[i] = old_prg_rom_bank[i];
			}
		} else if (mode == SAVE_SLOT_COUNT) {
			save_slot.tot_size[slot] += sizeof(BYTE) * 2;
		}
	} else {
		save_slot_ele(mode, slot, m121.bck);
	}
	save_slot_ele(mode, slot, m121.reg);
	extcl_save_mapper_MMC3(mode, slot, fp);

	return (EXIT_OK);
}
