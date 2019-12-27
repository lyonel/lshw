/*
 * based on Kay Sievers' volume_id - reads filesystem label and uuid
 *
 * CAUTION: this version is severely brain-damaged, don't blame Kay
 *
 * for instance, it will only detect the volume label on FAT32 if it
 *      is declared within the first cluster of the root directory
 *      (i.e. usually within the first 128 files)
 *
 * original copyright follows:
 *
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *	Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with this library; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "fat.h"
#include "osutils.h"
#include <stdlib.h>
#include <string.h>

/* linux/msdos_fs.h says: */
#define FAT12_MAX                       0xff4
#define FAT16_MAX                       0xfff4
#define FAT32_MAX                       0x0ffffff6

#define FAT_ATTR_VOLUME_ID              0x08
#define FAT_ATTR_DIR                    0x10
#define FAT_ATTR_LONG_NAME              0x0f
#define FAT_ATTR_MASK                   0x3f
#define FAT_ENTRY_FREE                  0xe5

struct vfat_super_block {
	uint8_t boot_jump[3];
	uint8_t sysid[8];
	uint16_t sector_size_bytes;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sct;
	uint8_t fats;
	uint16_t dir_entries;
	uint16_t sectors;
	uint8_t media;
	uint16_t fat_length;
	uint16_t secs_track;
	uint16_t heads;
	uint32_t hidden;
	uint32_t total_sect;
	union {
		struct fat_super_block {
			uint8_t unknown[3];
			uint8_t serno[4];
			uint8_t label[11];
			uint8_t magic[8];
			uint8_t dummy2[192];
			uint8_t pmagic[2];
		} __attribute__((__packed__)) fat;
		struct fat32_super_block {
			uint32_t fat32_length;
			uint16_t flags;
			uint8_t version[2];
			uint32_t root_cluster;
			uint16_t insfo_sector;
			uint16_t backup_boot;
			uint16_t reserved2[6];
			uint8_t unknown[3];
			uint8_t serno[4];
			uint8_t label[11];
			uint8_t magic[8];
			uint8_t dummy2[164];
			uint8_t pmagic[2];
		} __attribute__((__packed__)) fat32;
		char sector[512];	// to make sure the whole struct is at least 512 bytes long
	} __attribute__((__packed__)) type;
} __attribute__((__packed__));

struct vfat_dir_entry {
	uint8_t name[11];
	uint8_t attr;
	uint16_t time_creat;
	uint16_t date_creat;
	uint16_t time_acc;
	uint16_t date_acc;
	uint16_t cluster_high;
	uint16_t time_write;
	uint16_t date_write;
	uint16_t cluster_low;
	uint32_t size;
} __attribute__((__packed__));

static uint8_t *get_attr_volume_id(struct vfat_dir_entry *dir, int count)
{
	for (; --count >= 0; dir++) {
		/* end marker */
		if (dir->name[0] == 0x00) {
			break;
		}

		/* empty entry */
		if (dir->name[0] == FAT_ENTRY_FREE)
			continue;

		/* long name */
		if ((dir->attr & FAT_ATTR_MASK) == FAT_ATTR_LONG_NAME)
			continue;

		if ((dir->attr & (FAT_ATTR_VOLUME_ID | FAT_ATTR_DIR)) == FAT_ATTR_VOLUME_ID) {
			/* labels do not have file data */
			if (dir->cluster_high != 0 || dir->cluster_low != 0)
				continue;

			return dir->name;
		}

	}

	return NULL;
}

bool scan_fat(hwNode & n, source & id)
{
	struct vfat_super_block vs;
	struct vfat_dir_entry *dir;
	uint16_t sector_size_bytes;
	uint16_t dir_entries;
	uint32_t sect_count;
	uint16_t reserved_sct;
	uint32_t fat_size_sct;
	uint32_t root_cluster;
	uint32_t dir_size_sct;
	uint32_t cluster_count;
	uint64_t root_start_sect;
	uint32_t start_data_sct;
	uint8_t *buf = NULL;
	uint32_t buf_size;
	uint8_t *label = NULL;
	uint32_t next_cluster;


	if(!readlogicalblocks(id, &vs, 0, 1))
		return false;

	/* believe only that's fat, don't trust the version
	 * the cluster_count will tell us
	 */
	if (memcmp(vs.sysid, "NTFS", 4) == 0)
		return -1;

	if (memcmp(vs.type.fat32.magic, "MSWIN", 5) == 0)
		goto valid;

	if (memcmp(vs.type.fat32.magic, "FAT32   ", 8) == 0)
		goto valid;

	if (memcmp(vs.type.fat.magic, "FAT16   ", 8) == 0)
		goto valid;

	if (memcmp(vs.type.fat.magic, "MSDOS", 5) == 0)
		goto valid;

	if (memcmp(vs.type.fat.magic, "FAT12   ", 8) == 0)
		goto valid;

	/*
	 * There are old floppies out there without a magic, so we check
	 * for well known values and guess if it's a fat volume
	 */

	/* boot jump address check */
	if ((vs.boot_jump[0] != 0xeb || vs.boot_jump[2] != 0x90) &&
	    vs.boot_jump[0] != 0xe9)
		return false;

	/* heads check */
	if (vs.heads == 0)
		return false;

	/* media check */
	if (vs.media < 0xf8 && vs.media != 0xf0)
		return false;

	/* fat count*/
	if (vs.fats != 2)
		return false;

valid:
	/* cluster size check */
	if (vs.sectors_per_cluster == 0 ||
	    (vs.sectors_per_cluster & (vs.sectors_per_cluster-1)))
		return false;

	/* sector size check */
	sector_size_bytes = le_short(&vs.sector_size_bytes);
	if (sector_size_bytes != 0x200 && sector_size_bytes != 0x400 &&
	    sector_size_bytes != 0x800 && sector_size_bytes != 0x1000)
		return false;

	id.blocksize = sector_size_bytes;

	reserved_sct = le_short(&vs.reserved_sct);

	sect_count = le_short(&vs.sectors);
	if (sect_count == 0)
		sect_count = le_long(&vs.total_sect);

	fat_size_sct = le_short(&vs.fat_length);
	if (fat_size_sct == 0)
		fat_size_sct = le_long(&vs.type.fat32.fat32_length);
	fat_size_sct *= vs.fats;

	dir_entries = le_short(&vs.dir_entries);
	dir_size_sct = ((dir_entries * sizeof(struct vfat_dir_entry)) +
	                (sector_size_bytes-1)) / sector_size_bytes;

	cluster_count = sect_count - (reserved_sct + fat_size_sct + dir_size_sct);
	cluster_count /= vs.sectors_per_cluster;

//	if (cluster_count < FAT12_MAX) {
//		strcpy(id->type_version, "FAT12");
//	} else if (cluster_count < FAT16_MAX) {
//		strcpy(id->type_version, "FAT16");
//	} else {
//		strcpy(id->type_version, "FAT32");
//		goto fat32;
//	}
	if (cluster_count < FAT16_MAX)
	{

		/* the label may be an attribute in the root directory */
		root_start_sect = reserved_sct + fat_size_sct;

		buf_size = dir_entries * sizeof(struct vfat_dir_entry);
		buf = (uint8_t*)malloc(buf_size);
		if(!readlogicalblocks(id, buf, root_start_sect, buf_size / sector_size_bytes))
			goto end;

		label = get_attr_volume_id((struct vfat_dir_entry*) buf, dir_entries);

		if (label != NULL && memcmp(label, "NO NAME    ", 11) != 0) {
			n.setConfig("label", hw::strip(string((char*)label, 11)));
		} else if (memcmp(vs.type.fat.label, "NO NAME    ", 11) != 0) {
			n.setConfig("label", hw::strip(string((char*)vs.type.fat.label, 11)));
		}
	}

	else {
		/* FAT32 root dir is a cluster chain like any other directory */
		/* FIXME: we actually don't bother checking the whole chain */
		/* --> the volume label is assumed to be within the first cluster (usually 128 entries)*/
		buf_size = vs.sectors_per_cluster * sector_size_bytes;
		buf = (uint8_t*)malloc(buf_size);
		root_cluster = le_long(&vs.type.fat32.root_cluster);
		start_data_sct = reserved_sct + fat_size_sct;
		id.blocksize = sector_size_bytes;

		next_cluster = root_cluster;
		{
			uint32_t next_off_sct;
			uint64_t next_off;
			int count;

			next_off_sct = (next_cluster - 2) * vs.sectors_per_cluster;
			next_off = start_data_sct + next_off_sct;

			/* get cluster */
			if(!readlogicalblocks(id, buf, next_off, vs.sectors_per_cluster))
				goto end;

			dir = (struct vfat_dir_entry*) buf;
			count = buf_size / sizeof(struct vfat_dir_entry);

			label = get_attr_volume_id(dir, count);
		}

		if (label != NULL && memcmp(label, "NO NAME    ", 11) != 0) {
			n.setConfig("label", hw::strip(string((char*)label, 11)));
		} else if (memcmp(vs.type.fat32.label, "NO NAME    ", 11) != 0) {
			n.setConfig("label", hw::strip(string((char*)vs.type.fat.label, 11)));
		}
	}

end:
	if(buf) free(buf);
	return true;
}

