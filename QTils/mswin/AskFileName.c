/*
 *  AskFileName.c
 *  QTilities
 *
 *  Created by René J.V. Bertin on 20110125.
 *  Copyright 2010 INRETS. All rights reserved.
 *
 * QuickTime player toolkit; this file contains the MSWin32-specific routines and those that depend
 * on them directly.
 *
 */

#include "winixdefs.h"
#include "copyright.h"
IDENTIFY("AskFileName: MSWin32 file dialog");

#include <errno.h>
#include <stdlib.h>
#include <errno.h>

#include <Windows.h>


char *AskFileName(char *title)
{ OPENFILENAME lp;
  static char fileName[1024];
	memset( &lp, 0, sizeof(lp) );
	lp.lStructSize = sizeof(OPENFILENAME);
	lp.lpstrFilter = "QuickTime File\0*.mov\0Supported Media\0*.mov;*.qi2m;*.VOD;*.jpgs;*.mpg;*.mp4;*.mpeg;*.avi;*.wmv;*.mp3;*.aif;*.wav;*.mid;*.jpg;*.jpeg\0All Files\0*.*\0\0";
	lp.nFilterIndex = 1;
	fileName[0] = '\0';
	lp.lpstrFile = fileName;
	lp.nMaxFile = sizeof(fileName);
	lp.lpstrTitle = title;
	lp.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if( GetOpenFileName(&lp) ){
		return fileName;
	}
	else{
		return NULL;
	}
}

char *AskSaveFileName(char *title)
{ OPENFILENAME lp;
  static char fileName[1024];
	memset( &lp, 0, sizeof(lp) );
	lp.lStructSize = sizeof(OPENFILENAME);
	lp.lpstrFilter = NULL;
	lp.nFilterIndex = 1;
	fileName[0] = '\0';
	lp.lpstrFile = fileName;
	lp.nMaxFile = sizeof(fileName);
	lp.lpstrTitle = title;
	lp.Flags = OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT;
//	lp.lpstrDefExt = "mov";
	if( GetOpenFileName(&lp) ){
		return fileName;
	}
	else{
		return NULL;
	}
}
