/* $Id: dskread.c,v 1.4 2001/12/27 01:53:07 nurgle Exp $
 *
 * dskread.c - Small utility to read CPC disk images from a floppy disk under
 * Linux with a standard PC FDC.
 * Copyright (C)2001 Andreas Micklei <nurgle@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "common.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fd.h>
#include <linux/fdreg.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

void rotateleft_sectorids(Trackinfo *trackinfo, int pos) {

	Sectorinfo sectorinfo[29];
	int i, spt;

	memcpy( sectorinfo, trackinfo->sectorinfo, sizeof( Sectorinfo ) * 30 );

	spt = trackinfo->spt;
	for( i=0; i<spt; i++ ) {
		memcpy( &trackinfo->sectorinfo[i],
			&sectorinfo[(i+pos)%spt], sizeof( Sectorinfo ) );
	}

}

void rotate_sectorids(Trackinfo *trackinfo) {

	int i, low, pos, sector;

	low = 0xFF;
	pos = 0;

	/* Find lowest sector number */
	for( i=0; i<trackinfo->spt; i++ ) {
		sector = trackinfo->sectorinfo[i].sector;
		if ( sector < low ) {
			low = sector;
			pos = i;
		}
	}

	/* Rotate sectorids in trackinfo left by pos positions */
	rotateleft_sectorids(trackinfo, pos);

}

void read_ids(int fd, Trackinfo *trackinfo) {

	int i, err;
	struct floppy_raw_cmd raw_cmd;
	unsigned char mask = 0xFF;

	init_raw_cmd(&raw_cmd);
	raw_cmd.flags = FD_RAW_READ | FD_RAW_INTR;
	raw_cmd.flags |= FD_RAW_NEED_SEEK;
	raw_cmd.track = trackinfo->track;
	raw_cmd.rate  = 2;	/* SD */
	raw_cmd.length= (128<<(trackinfo->bps));

	raw_cmd.cmd[raw_cmd.cmd_count++] = FD_READID & mask;

	raw_cmd.cmd[raw_cmd.cmd_count++] = 0;			/* head */	//FIXME - this is the physical side to read from

	for( i=0; i<trackinfo->spt; i++ ) {
		err = ioctl(fd, FDRAWCMD, &raw_cmd);
		if (err < 0) {
			perror("Error reading id");
			exit(1);
		}
		if (raw_cmd.reply[0] & 0x40) {
			fprintf(stderr, "Could not read sector id\n");
		}
		trackinfo->sectorinfo[i].track = raw_cmd.reply[3];
		trackinfo->sectorinfo[i].head = raw_cmd.reply[4];
		trackinfo->sectorinfo[i].sector = raw_cmd.reply[5];
		trackinfo->sectorinfo[i].bps = raw_cmd.reply[6];
	}

	rotate_sectorids( trackinfo );
}

void read_sect(int fd, Trackinfo *trackinfo, Sectorinfo *sectorinfo,
	unsigned char *data) {

	int i, err;
	struct floppy_raw_cmd raw_cmd;
	unsigned char mask = 0xFF;

	init_raw_cmd(&raw_cmd);
	raw_cmd.flags = FD_RAW_READ | FD_RAW_INTR;
	raw_cmd.flags |= FD_RAW_NEED_SEEK;
	raw_cmd.track = sectorinfo->track;
	raw_cmd.rate  = 2;	/* SD */
	raw_cmd.length= (128<<(sectorinfo->bps));
	raw_cmd.data  = data;

	raw_cmd.cmd[raw_cmd.cmd_count++] = FD_READ & mask;

	raw_cmd.cmd[raw_cmd.cmd_count++] = 0;			/* head */	//FIXME - this is the physical side to read from
	raw_cmd.cmd[raw_cmd.cmd_count++] = sectorinfo->track;	/* track */
	raw_cmd.cmd[raw_cmd.cmd_count++] = sectorinfo->head;	/* head */	
	raw_cmd.cmd[raw_cmd.cmd_count++] = sectorinfo->sector;	/* sector */
	raw_cmd.cmd[raw_cmd.cmd_count++] = sectorinfo->bps;	/* sectorsize */
	raw_cmd.cmd[raw_cmd.cmd_count++] = sectorinfo->sector;	/* sector */
	raw_cmd.cmd[raw_cmd.cmd_count++] = trackinfo->gap;	/* GPL */
	raw_cmd.cmd[raw_cmd.cmd_count++] = 0xFF;		/* DTL */

	err = ioctl(fd, FDRAWCMD, &raw_cmd);
	if (err < 0) {
		perror("Error reading");
		exit(1);
	}
	if (raw_cmd.reply[0] & 0x40) {
		fprintf(stderr, "Could not read sector %0X\n",
			sectorinfo->sector);
	}
}

void init_trackinfo( Trackinfo *trackinfo, int track ) {

	int i;

	memset(trackinfo, 0, sizeof(*trackinfo));

	strncpy( trackinfo->magic, MAGIC_TRACK, sizeof( trackinfo->magic ) );
	//unsigned char unused1[0x03];
	trackinfo->track = track;
	trackinfo->head = 0;
	//unsigned char unused2[0x02];
	trackinfo->bps = 2;
	trackinfo->spt = 9;
	trackinfo->gap = 82;
	trackinfo->fill = 0xFF;
	//trackinfo->sectorinfo[29];
	for ( i=0; i<9; i++ ) {
		init_sectorinfo( &trackinfo->sectorinfo[i], track, 0, 0xC1+i );
	}

}

void init_diskinfo( Diskinfo *diskinfo, int tracks, int heads, int tracklen ) {

	memset(diskinfo, 0, sizeof(*diskinfo));

	strncpy( diskinfo->magic, MAGIC_DISK_WRITE, sizeof( diskinfo->magic ) );
	diskinfo->tracks = tracks;
	diskinfo->heads = heads;
	diskinfo->tracklen[0] = (char) tracklen;
	diskinfo->tracklen[1] = (char) (tracklen >> 8);
	//unsigned char tracklenhigh[0xCC];

}

void timestamp_diskinfo( Diskinfo *diskinfo ) {

	time_t t;
	struct tm *ltime;

	t = time(NULL);
	ltime = localtime(&t);
	/* FIXME: Can the formatting be messed up by locale settings? */
	strftime( diskinfo->magic+14, 16, "%d %b %g %H:%M", ltime );

}

void readdsk(char *filename) {

	/* Variable declarations */
	int fd, tmp, err;
	char *drive;
	struct floppy_raw_cmd raw_cmd;

	Diskinfo diskinfo;
	Trackinfo trackinfo[TRACKS];
	Sectorinfo *sectorinfo, **sectorinfos;
	unsigned char data[TRACKLEN*TRACKS], *sect, *track;
	int tracklen;
	FILE *file;
	int i, j, count;
	char *magic_disk = MAGIC_DISK;
	char *magic_edisk = MAGIC_EDISK;
	char *magic_track = MAGIC_TRACK;
	char flag_edisk = FALSE;	// indicates extended disk image format

	/* initialization */
	drive = "/dev/fd0";

	/* open drive */
	fd = open( drive, O_ACCMODE | O_NDELAY);
	if ( fd < 0 ){
		perror("Error opening floppy device");
		exit(1);
	}

	/* open file */
	file = fopen(filename, "w");
	if (file == NULL) {
		perror("Error opening image file");
		exit(1);
	}

	init( fd );

	/* FIXME: Extremely unflexible after here */
	
	sect = data;
	for ( i=0; i<40; i++ ) {
		init_trackinfo( &trackinfo[i], i );
		printtrackinfo(stderr, &trackinfo[i]);
		fprintf(stderr, " [");
		read_ids(fd, &trackinfo[i]);
		/* Slow version: Read sectors in order */
		/*
		for ( j=0; j<trackinfo->spt; j++ ) {
			sectorinfo = &trackinfo[i].sectorinfo[j];
			fprintf(stderr, "%0X ", sectorinfo->sector);
			read_sect(fd, &trackinfo[i], sectorinfo, sect);
			sect += 0x200;
		}
		*/
		/* Fast version: Read sectors interleaved in two passes */
		for ( j=0; j<trackinfo->spt; j+=2 ) {
			sectorinfo = &trackinfo[i].sectorinfo[j];
			fprintf(stderr, "%0X ", sectorinfo->sector);
			read_sect(fd, &trackinfo[i], sectorinfo, sect);
			sect += 0x400;
		}
		sect = sect - ( 0x400 * j/2 ) + 0x200;
		for ( j=1; j<trackinfo->spt; j+=2 ) {
			sectorinfo = &trackinfo[i].sectorinfo[j];
			fprintf(stderr, "%0X ", sectorinfo->sector);
			read_sect(fd, &trackinfo[i], sectorinfo, sect);
			sect += 0x400;
		}
		fprintf(stderr, "]\n");
	}

	init_diskinfo( &diskinfo, 40, 1, TRACKLEN_INFO );
	timestamp_diskinfo( &diskinfo );
	printdiskinfo(stderr, &diskinfo);

	count = fwrite(&diskinfo, 1, sizeof(diskinfo), file);
	if (count != sizeof(diskinfo)) {
		myabort("Error writing Disk-Info: File to short\n");
	}

	track = data;
	tracklen = TRACKLEN;
	for (i=0; i<diskinfo.tracks; i++) {
		count = fwrite(&trackinfo[i], 1, sizeof(trackinfo[i]), file);
		if (count != sizeof(trackinfo[i])) {
			myabort("Error writing Track-Info: File to short\n");
		}
		count = fwrite(track, 1, tracklen, file);
		if (count != tracklen) {
			myabort("Error writing Track: File to short\n");
		}
		track += tracklen;
	}

	fclose(file);

}

int main(int argc, char **argv) {

	if (argc == 2) { readdsk(argv[1]);
	} else { fprintf(stderr, "usage: dskread <filename>\n");
	}
	return 0;

}

