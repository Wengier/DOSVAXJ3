/*
 *  Copyright (C) 2002-2015  The DOSBox Team
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


#include "dosbox.h"
#include "mem.h"
#include "dos_inc.h"
#include "callback.h"

#ifdef _MSC_VER
#pragma pack(1)
#endif
struct DOS_TableCase {	
	Bit16u size;
	Bit8u chars[256];
}
GCC_ATTRIBUTE (packed);
#ifdef _MSC_VER
#pragma pack ()
#endif

RealPt DOS_TableUpCase;
RealPt DOS_TableLowCase;

static Bitu call_casemap;

static Bit16u dos_memseg=DOS_PRIVATE_SEGMENT;

Bit16u DOS_GetMemory(Bit16u pages) {
	if ((Bitu)pages+(Bitu)dos_memseg>=DOS_PRIVATE_SEGMENT_END) {
		E_Exit("DOS:Not enough memory for internal tables");
	}
	Bit16u page=dos_memseg;
	dos_memseg+=pages;
	return page;
}

static Bitu DOS_CaseMapFunc(void) {
	//LOG(LOG_DOSMISC,LOG_ERROR)("Case map routine called : %c",reg_al);
	return CBRET_NONE;
}

static Bit8u country_info[0x22] = {
/* Date format      */  0x00, 0x00,
/* Currencystring   */  0x24, 0x00, 0x00, 0x00, 0x00,
/* Thousands sep    */  0x2c, 0x00,
/* Decimal sep      */  0x2e, 0x00,
/* Date sep         */  0x2d, 0x00,
/* time sep         */  0x3a, 0x00,
/* currency form    */  0x00,
/* digits after dec */  0x02,
/* Time format      */  0x00,
/* Casemap          */  0x00, 0x00, 0x00, 0x00,
/* Data sep         */  0x2c, 0x00,
/* Reservered 5     */  0x00, 0x00, 0x00, 0x00, 0x00,
/* Reservered 5     */  0x00, 0x00, 0x00, 0x00, 0x00
};

uint16_t seg_win_startup_info;

void DOS_SetupTables(void) {
	Bit16u seg;Bitu i;
	dos.tables.mediaid=RealMake(DOS_GetMemory(4),0);
	dos.tables.tempdta=RealMake(DOS_GetMemory(4),0);
	dos.tables.tempdta_fcbdelete=RealMake(DOS_GetMemory(4),0);
	for (i=0;i<DOS_DRIVES;i++) mem_writew(Real2Phys(dos.tables.mediaid)+i*2,0);
	/* Create the DOS Info Block */
	dos_infoblock.SetLocation(DOS_INFOBLOCK_SEG); //c2woody
   
	/* create SDA */
	DOS_SDA(DOS_SDA_SEG,0).Init();

	/* Some weird files >20 detection routine */
	/* Possibly obselete when SFT is properly handled */
	real_writed(DOS_CONSTRING_SEG,0x0a,0x204e4f43);
	real_writed(DOS_CONSTRING_SEG,0x1a,0x204e4f43);
	real_writed(DOS_CONSTRING_SEG,0x2a,0x204e4f43);

	/* create a CON device driver */
	if(IS_J3_ARCH || IS_DOSV) {
		seg = DOS_GetMemory(2);
 		real_writew(seg,0x00,0x0000);			// next ptr
 		real_writew(seg,0x02,DOS_CONDRV_SEG);	// next ptr
 		real_writew(seg,0x04,0xc000);			// attributes
  		real_writew(seg,0x06,0x0018);			// strategy routine
  		real_writew(seg,0x08,0x001e);			// interrupt routine
  		real_writed(seg,0x0a,0x4d424924);		// driver name
  		real_writed(seg,0x0e,0x50534441);		// driver name
  		real_writeb(seg,0x18,0x26);				// or word ptr es:[bx+03],0100h .. strategy
  		real_writeb(seg,0x19,0x81);
  		real_writeb(seg,0x1a,0x4f);
  		real_writeb(seg,0x1b,0x03);
  		real_writeb(seg,0x1c,0x00);
  		real_writeb(seg,0x1d,0x01);
  		real_writeb(seg,0x1e,0xcb);				// retf .. interrupt
		// Windows Startup Information Structure
		// Dummy data is required for the Microsoft version of Japanese Windows 3.1 in enhanced mode.
		seg_win_startup_info = DOS_GetMemory(2);
		real_writew(seg_win_startup_info,0x00,0x03);					// version major
		real_writew(seg_win_startup_info,0x01,0x00);					// version minor
		real_writed(seg_win_startup_info,0x02,0);						// next startup information
		real_writed(seg_win_startup_info,0x06,0);						// virtual device name
		real_writed(seg_win_startup_info,0x0a,0);						// virtual device reference data
		real_writew(seg_win_startup_info,0x0e,0x0012);					// instance data record offset
		real_writew(seg_win_startup_info,0x10,seg_win_startup_info);	// instance data record segment
		real_writew(seg_win_startup_info,0x12,0x001c);					// instance data offset
		real_writew(seg_win_startup_info,0x14,seg_win_startup_info);	// instance data segment
		real_writew(seg_win_startup_info,0x16,0x0004);					// instance data size
		real_writed(seg_win_startup_info,0x18,0);						// instance data table end
		real_writed(seg_win_startup_info,0x1c,0);						// instance data
		dos_infoblock.SetDeviceChainStart(RealMake(seg,0));
	}
	seg=DOS_CONDRV_SEG;
 	real_writed(seg,0x00,0xffffffff);	// next ptr
 	real_writew(seg,0x04,0x8013);		// attributes
  	real_writed(seg,0x06,0xffffffff);	// strategy routine
  	real_writed(seg,0x0a,0x204e4f43);	// driver name
  	real_writed(seg,0x0e,0x20202020);	// driver name
	if(!IS_J3_ARCH && !IS_DOSV) dos_infoblock.SetDeviceChainStart(RealMake(seg,0));
   
	/* Create a fake Current Directory Structure */
	seg=DOS_CDS_SEG;
	real_writed(seg,0x00,0x005c3a43);
	dos_infoblock.SetCurDirStruct(RealMake(seg,0));

	/* Allocate DCBS DOUBLE BYTE CHARACTER SET LEAD-BYTE TABLE */
	// If you put it at a higher address, it will go into English mode at the end of Windows 3.1.
	dos.tables.dbcs=RealMake(DOS_MEM_START - 1, 0);
	if (dos.set_ax_enabled || IS_J3_ARCH || IS_DOSV) {//if machine==jega is set (for AX)
		mem_writew(Real2Phys(dos.tables.dbcs) + 0x00, 0x0006); //Set DBCS table
		mem_writeb(Real2Phys(dos.tables.dbcs) + 0x02, 0x81); 
		mem_writeb(Real2Phys(dos.tables.dbcs) + 0x03, 0x9f);
		mem_writeb(Real2Phys(dos.tables.dbcs) + 0x04, 0xe0);
		mem_writeb(Real2Phys(dos.tables.dbcs) + 0x05, 0xfc);
		mem_writeb(Real2Phys(dos.tables.dbcs) + 0x06, 0x00);
		mem_writeb(Real2Phys(dos.tables.dbcs) + 0x07, 0x00);
	} else
		mem_writed(Real2Phys(dos.tables.dbcs),0); //empty table
	/* FILENAME CHARACTER TABLE */
	dos.tables.filenamechar=RealMake(DOS_GetMemory(2),0);
	mem_writew(Real2Phys(dos.tables.filenamechar)+0x00,0x16);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x02,0x01);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x03,0x00);	// allowed chars from
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x04,0xff);	// ...to
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x05,0x00);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x06,0x00);	// excluded chars from
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x07,0x20);	// ...to
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x08,0x02);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x09,0x0e);	// number of illegal separators
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x0a,0x2e);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x0b,0x22);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x0c,0x2f);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x0d,0x5c);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x0e,0x5b);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x0f,0x5d);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x10,0x3a);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x11,0x7c);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x12,0x3c);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x13,0x3e);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x14,0x2b);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x15,0x3d);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x16,0x3b);
	mem_writeb(Real2Phys(dos.tables.filenamechar)+0x17,0x2c);
	/* COLLATING SEQUENCE TABLE + UPCASE TABLE*/
	// 256 bytes for col table, 128 for upcase, 4 for number of entries
	dos.tables.collatingseq=RealMake(DOS_GetMemory(25),0);
	mem_writew(Real2Phys(dos.tables.collatingseq),0x100);
	for (i=0; i<256; i++) mem_writeb(Real2Phys(dos.tables.collatingseq)+i+2,i);
	dos.tables.upcase=dos.tables.collatingseq+258;
	mem_writew(Real2Phys(dos.tables.upcase),0x80);
	for (i=0; i<128; i++) mem_writeb(Real2Phys(dos.tables.upcase)+i+2,0x80+i);
 

	/* Create a fake FCB SFT */
	seg=DOS_GetMemory(4);
	real_writed(seg,0,0xffffffff);		//Last File Table
	real_writew(seg,4,100);				//File Table supports 100 files
	dos_infoblock.SetFCBTable(RealMake(seg,0));

	/* Create a fake DPB */
	dos.tables.dpb=DOS_GetMemory(2);
	for(Bitu d=0;d<26;d++) real_writeb(dos.tables.dpb,d,d);

	/* Create Device command packet area */
	dos.tables.dcp = DOS_GetMemory(3);

	/* Create a fake disk buffer head */
	seg=DOS_GetMemory(6);
	for (Bitu ct=0; ct<0x20; ct++) real_writeb(seg,ct,0);
	real_writew(seg,0x00,0xffff);		// forward ptr
	real_writew(seg,0x02,0xffff);		// backward ptr
	real_writeb(seg,0x04,0xff);			// not in use
	real_writeb(seg,0x0a,0x01);			// number of FATs
	real_writed(seg,0x0d,0xffffffff);	// pointer to DPB
	dos_infoblock.SetDiskBufferHeadPt(RealMake(seg,0));

	/* Set buffers to a nice value */
	dos_infoblock.SetBuffers(50,50);

	/* case map routine INT 0x21 0x38 */
	call_casemap = CALLBACK_Allocate();
	CALLBACK_Setup(call_casemap,DOS_CaseMapFunc,CB_RETF,"DOS CaseMap");
	/* Add it to country structure */
	//host_writed(country_info + 0x12, CALLBACK_RealPointer(call_casemap));
	//dos.tables.country=country_info;
	dos.tables.country_seg = DOS_GetMemory(3);
	PhysPt dest = (dos.tables.country_seg << 4) + 7;
	MEM_BlockWrite(dest, country_info, sizeof(country_info));
	real_writeb(dos.tables.country_seg, 0, 1);
	real_writew(dos.tables.country_seg, 1, 0x26);
	real_writew(dos.tables.country_seg, 5, dos.loaded_codepage);
	real_writed(dos.tables.country_seg, 7 + 0x12, CALLBACK_RealPointer(call_casemap));
	if (IS_AX_ARCH || IS_J3_ARCH || IS_DOSV) {
		real_writew(dos.tables.country_seg, 0x03, 81);
		real_writew(dos.tables.country_seg, 0x07, 2);
		real_writeb(dos.tables.country_seg, 0x09, 0x5c);
		real_writeb(dos.tables.country_seg, 0x17, 0);
		real_writeb(dos.tables.country_seg, 0x18, 1);
	} else {
		real_writew(dos.tables.country_seg, 0x03, 1);
	}

	dos.lfn_dta_seg = DOS_GetMemory(8);
}
