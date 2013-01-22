/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "Multimap.h"
#include "Sideview.h"
#include "LKObjects.h"
#include "RGB.h"
#include "LKStyle.h"

extern short GetVisualGlidePoints(unsigned short numslots );

// Green area is up, red is down. A choice..
#define GREENUP	1

// border margins in boxed text
#define XYMARGIN NIBLSCALE(2) 

// space between two adjacent boxes on the same row
#define BOXINTERVAL NIBLSCALE(2)

// space between row0 and center line
#define CENTERYSPACE NIBLSCALE(1)

// How many boxes on a row, max
#define MAXBSLOT 10

#define LGREEN RGB(150,255,150)
#define LYELLOW RGB(255,255,150)
#define LRED   RGB(255,150,150)

// Size of the box, fixed for each waypoint at this resolution
static unsigned int boxSizeX=0 ,boxSizeY=0;
static int maxtSizeX=0, maxtSizeY=0;
static HFONT line1Font, line2Font;

static int slotWpIndex[MAXBSLOT+1];
static double tmpSlotBrgDiff[MAXBSLOT+1];

//#define DEBUG_DVG	1
//#define DEBUG_GVG	1

void MapWindow::DrawVisualGlide(HDC hdc, DiagrammStruct* pDia) {

  unsigned short numboxrows=1;

  #if BUGSTOP
  LKASSERT(Current_Multimap_SizeY<SIZE4);
  #endif
  switch(Current_Multimap_SizeY) {
	case SIZE0:
	case SIZE1:
		numboxrows=3;
		break;
	case SIZE2:
		numboxrows=2;
		break;
	case SIZE3:
		numboxrows=1;
		break;
	case SIZE4:
		return;
	default:
		LKASSERT(0);
		break;
  }

  switch(ScreenSize) {
	case ss800x480:
		line1Font=MapLabelFont;
		line2Font=LK8PanelUnitFont;
		break;
	default:
		line1Font=MapLabelFont;
		line2Font=LK8PanelUnitFont;
		break;
  }


  SIZE textSize;
  TCHAR tmpT[30];
  _tcscpy(tmpT,_T("MMMMMMMM"));
  SelectObject(hdc, line1Font);
  GetTextExtentPoint(hdc, tmpT, _tcslen(tmpT), &textSize);
  maxtSizeX=textSize.cx;
  maxtSizeY=textSize.cy;
  boxSizeX=textSize.cx+XYMARGIN+XYMARGIN;
  boxSizeY=(textSize.cy*numboxrows)+XYMARGIN+XYMARGIN;

  //StartupStore(_T("boxX=%d boxY=%d  \n"),  boxSizeX,boxSizeY);


  RECT vrc;
  vrc.left=0;
  vrc.right=ScreenSizeX;
  vrc.bottom=ScreenSizeY-BottomSize;
  if (Current_Multimap_SizeY==SIZE0) // Full screen?
	vrc.top=0;
  else
	vrc.top=pDia->rc.bottom;
  #if 0
  StartupStore(_T("fxMinMax=%f,%f  fyMinMax=%f,%f rcTLBR=%d,%d %d,%d\n"),
	pDia->fXMin,pDia->fXMax,pDia->fYMin,pDia->fYMax,
	pDia->rc.top, pDia->rc.left, pDia->rc.bottom,pDia->rc.right);
  #endif
  //StartupStore(_T("VG AREA LTRB: %d,%d %d,%d\n"),vrc.left,vrc.top,vrc.right,vrc.bottom);

  HBRUSH oldBrush=(HBRUSH) SelectObject(hdc,GetStockObject(WHITE_BRUSH));
  HPEN   oldPen  =(HPEN)   SelectObject(hdc, GetStockObject(BLACK_PEN));

  HBRUSH brush_back;
  if (!INVERTCOLORS) {
    brush_back = LKBrush_Black;
  } else {
    brush_back = LKBrush_Nlight;
  }

  FillRect(hdc, &vrc, brush_back);

  POINT center, p1, p2;
  center.y=vrc.top+(vrc.bottom-vrc.top)/2;
  center.x=vrc.left+(vrc.right-vrc.left)/2;

  // numSlotX is the number items we can print horizontally.
  unsigned short numSlotX=(vrc.right-vrc.left)/(boxSizeX+BOXINTERVAL);
  if (numSlotX>MAXBSLOT) numSlotX=MAXBSLOT;
  #if BUGSTOP
  LKASSERT(numSlotX>0);
  #endif
  if (numSlotX==0) return;

  unsigned short boxInterval=((vrc.right-vrc.left)-(boxSizeX*numSlotX))/(numSlotX+1);

  //StartupStore(_T("numSlotX=%d boxInterval=%d\n"),numSlotX,boxInterval);

  unsigned int t;

  // The horizontal grid
  unsigned int slotCenterX[MAXBSLOT+1];
  for (t=0; t<numSlotX; t++) {
	slotCenterX[t]=(t*boxSizeX) + boxInterval*(t+1)+(boxSizeX/2);
	//StartupStore(_T("slotCenterX[%d]=%d\n"),t,slotCenterX[t]);
  }

  // Vertical coordinates of each up/down subwindow, excluding center line
  int upYtop=vrc.top;
  int upYbottom=center.y-CENTERYSPACE;
  int upSizeY=upYbottom-upYtop;
  int downYtop=center.y+CENTERYSPACE;
  int downYbottom=vrc.bottom;
  int downSizeY=downYbottom-downYtop;

  #if 0
  // Reassign dynamically the vertical scale for each subwindow size
  int vscale=VSCALE*(100-Current_Multimap_SizeY)/100;
  #else
  // Set the vertical range 
  int vscale;
  if (Units::GetUserAltitudeUnit()==unFeet)
	vscale=(unsigned int)(1000/TOFEET);
  else
	vscale=300;
  #endif



  SetBkMode(hdc,TRANSPARENT);

  RECT trc;
  trc=vrc;

  // Top part of visual rect, target is over us=unreachable=red
  trc.top=vrc.top;
  trc.bottom=center.y-1;
  #if GREENUP
  RenderSky( hdc, trc, RGB_WHITE, LGREEN,GC_NO_COLOR_STEPS/2);
  #else
  RenderSky( hdc, trc, RGB_WHITE, LRED , GC_NO_COLOR_STEPS/2);
  #endif
  // Bottom part, target is below us=reachable=green
  trc.top=center.y+1;
  trc.bottom=vrc.bottom;
  #if GREENUP
  RenderSky( hdc, trc, LRED, RGB_WHITE, GC_NO_COLOR_STEPS/2);
  #else
  RenderSky( hdc, trc, LGREEN , RGB_WHITE, GC_NO_COLOR_STEPS/2);
  #endif

  // Draw center line
  p1.x=vrc.left+1; p1.y=center.y;
  p2.x=vrc.right-1; p2.y=center.y;
  SelectObject(hdc, LKPen_Black_N1);
  DrawSolidLine(hdc, p1, p2, vrc);



  SelectObject(hdc, line1Font);
  SelectObject(hdc,LKPen_Black_N0);

  short res=GetVisualGlidePoints(numSlotX);

  if (res==INVALID_VALUE) {
	#if DEBUG_DVG
	StartupStore(_T("...... GVGP says not ready, wait..\n"));
	#endif
	return;
  }
  if (res==0) {
	#if DEBUG_DVG
	StartupStore(_T("...... GVGP says no data available!\n"));
	#endif
	return;
  }

  // Print them all!
  int offset=(boxSizeY/2)+CENTERYSPACE;
  HBRUSH BRED=CreateSolidBrush(COLORREF LRED);
  HBRUSH BYEL=CreateSolidBrush(COLORREF LYELLOW);
  HBRUSH BGRE=CreateSolidBrush(COLORREF LGREEN);

  HBRUSH bcolor;
  for (unsigned short n=0; n<numSlotX; n++) {

	int wp=slotWpIndex[n];
	if (!ValidWayPoint(wp)) {
		// empty slot nothing to print
		continue;
	}
	#if DEBUG_DVG
	StartupStore(_T("... DVG PRINT [%d]=%d <%s>\n"),n,wp,WayPointList[wp].Name);
	#endif


	int altdiff=(int)WayPointCalc[wp].AltArriv[AltArrivMode];
	int ty;

	#if GREENUP
	// Positive arrival altitude for the waypoint, upper window
	if (altdiff>=0) {
		if (altdiff==0)altdiff=1;
		int d=vscale/altdiff;
		if (d==0) d=1;
		ty=upYbottom - (upSizeY/d); 
		if ((ty-offset)<upYtop) ty=upYtop+offset;
		if ((ty+offset)>upYbottom) ty=upYbottom-offset;
		bcolor=BGRE;
	} else {
		int d=vscale/altdiff;
		if (d==0) d=-1;
		ty=downYtop - (downSizeY/d); // - because the left part is negative, we are really adding.
		if ((ty-offset)<downYtop) ty=downYtop+offset;
		if ((ty+offset)>downYbottom) ty=downYbottom-offset;
		bcolor=BRED;
	}
	#else
	// Positive arrival altitude for the waypoint, lower window
	if (altdiff>=0) {
		if (altdiff==0)altdiff=1;
		int d=vscale/altdiff;
		if (d==0) d=1;
		ty=downYtop + (downSizeY/d);
		if ((ty-offset)<downYtop) ty=downYtop+offset;
		if ((ty+offset)>downYbottom) ty=downYbottom-offset;
		bcolor=BGRE;
	} else {
		int d=vscale/altdiff;
		if (d==0) d=-1;
		ty=upYbottom + (upSizeY/d); // + because the left part is negative. We are really subtracting
		if ((ty-offset)<upYtop) ty=upYtop+offset;
		if ((ty+offset)>upYbottom) ty=upYbottom-offset;
		bcolor=BRED;
	}
	#endif

	TCHAR line2[80], line3[80];
	TCHAR value[40], unit[30];
	switch (numboxrows) {
		case 0:
			#if BUGSTOP
			LKASSERT(0);
			#endif
			return;

		case 1:
			// 1 line: waypoint name
			VGTextInBox(hdc,1,WayPointList[wp].Name, NULL,NULL, slotCenterX[n] , ty,  RGB_BLACK, bcolor);
			break;

		case 2:
			// 2 lines: waypoint name + altdiff
			LKFormatAltDiff(wp, false, value, unit);
			_stprintf(line2,_T("%s%s"),value,unit);
			VGTextInBox(hdc,2,WayPointList[wp].Name, line2, NULL, slotCenterX[n] , ty,  RGB_BLACK, bcolor);
			break;

		case 3:
			// 3 lines: waypoint name + dist + altdiff
			LKFormatDist(wp, false, value, unit);
			_stprintf(line2,_T("%s%s"),value,unit);

			LKFormatAltDiff(wp, false, value, unit);
			_stprintf(line3,_T("%s%s"),value,unit);
			VGTextInBox(hdc,3,WayPointList[wp].Name, line2, line3, slotCenterX[n] , ty,  RGB_BLACK, bcolor);
			break;
		default:
			#if BUGSTOP
			LKASSERT(0);
			#endif
			return;
	}
			
  } // for numSlotX



  // Cleanup and return
//_end:
  SelectObject(hdc,oldBrush); 
  SelectObject(hdc,oldPen); 
  if (BRED) DeleteObject(BRED);
  if (BYEL) DeleteObject(BYEL);
  if (BGRE) DeleteObject(BGRE);
  return;
}






void MapWindow::VGTextInBox(HDC hDC, short numlines, const TCHAR* wText1, const TCHAR* wText2, const TCHAR *wText3, int x, int y, COLORREF trgb, HBRUSH bbrush) {

  #if BUGSTOP
  LKASSERT(wText1!=NULL);
  #endif
  if (!wText1) return;

  COLORREF oldTextColor=SetTextColor(hDC,trgb);

  SIZE tsize;
  int tx, ty, rowsize;


  SelectObject(hDC, line1Font);
  unsigned int tlen=_tcslen(wText1);
  GetTextExtentPoint(hDC, wText1, tlen, &tsize);
  rowsize=tsize.cy;

  // Fit as many characters in the available boxed space
  if (tsize.cx>maxtSizeX) {
	LKASSERT(tlen>0);
	for (short t=tlen-1; t>0; t--) {
		GetTextExtentPoint(hDC, wText1, t, &tsize);
		if (tsize.cx<=maxtSizeX) {
			tlen=t;
			break;
		}
	}
  }

  short vy=y+(boxSizeY/2);
  SelectObject(hDC,bbrush);
  Rectangle(hDC, 
	x-(boxSizeX/2),
	y-(boxSizeY/2),
	x+(boxSizeX/2),
	vy);

  tx = x-(tsize.cx/2);
  ty = y-(vy-y);

  ExtTextOut(hDC, tx, ty, ETO_OPAQUE, NULL, wText1, tlen, NULL);

  if (numlines==1) goto _end;
  #if BUGSTOP
  LKASSERT(wText2!=NULL);
  #endif
  if (!wText2) goto _end;

  SelectObject(hDC, line2Font);
  tlen=_tcslen(wText2);
  GetTextExtentPoint(hDC, wText2, tlen, &tsize);
  tx = x-(tsize.cx/2);
  ty += rowsize;
  ExtTextOut(hDC, tx, ty, ETO_OPAQUE, NULL, wText2, tlen, NULL);

  if (numlines==2) goto _end;
  #if BUGSTOP
  LKASSERT(wText3!=NULL);
  #endif
  if (!wText3) goto _end;

  tlen=_tcslen(wText3);
  GetTextExtentPoint(hDC, wText3, tlen, &tsize);
  tx = x-(tsize.cx/2);
  ty += rowsize;
  ExtTextOut(hDC, tx, ty, ETO_OPAQUE, NULL, wText3, tlen, NULL);

_end:
  SetTextColor(hDC,oldTextColor);
  return;
}



// This is filling up the slotWpIndex[MAXBSLOT] list.
// DoNearest is automatically updating its data every 5 seconds.
// We are returning the number of items filled, or -1. In this case
// we should not print anything at all because there are no valid
// wpts, or they were not calculated , etc.
// The difference between 0 and -1:
//  0 means no waypoints found!
// -1 means data not yet ready, wait please.
//
// This is also called by DrawMultimap_Asp when a EVENT_NEWRUN is detected for visualglide mode.
// We are resetting from there.
//

short MapWindow::GetVisualGlidePoints(unsigned short numslots ) {

  LKASSERT(numslots<MAXBSLOT);
  static short currentFilledNumber=-1;

  int i;

  // RESET COMMAND by passing 0, normally by EVENT_NEWRUN 
  if (numslots==0) {
	#if DEBUG_GVG
	StartupStore(_T("...... GVGP: RESET\n"));
	#endif
	for (i=0; i<MAXBSLOT; i++) {
		slotWpIndex[i]=INVALID_VALUE;
	}
	currentFilledNumber=-1;
	return INVALID_VALUE;
  }

  bool ndr=NearestDataReady;
  NearestDataReady=false;

  // No data ready..
  // if cfn is -1 we did not ever calculate it yet
  // otherwise 0 or >0  means use what we have already in the list
  if (!ndr) {
	#if DEBUG_GVG
	StartupStore(_T("...... GVGP: no data ready, currentFN=%d\n"),currentFilledNumber);
	#endif
	return currentFilledNumber;
  }

  if (SortedNumber<=0) {
	#if DEBUG_GVG
	StartupStore(_T("...... GVGP: SortedNumber is 0, no available wpts in this range!\n"));
	#endif
	return 0;
  }

  int *pindex;
  int wpindex=0;
  pindex=SortedTurnpointIndex;
  //
  // Reset  content
  //
  currentFilledNumber=0;
  for (i=0; i<MAXBSLOT; i++) {
	slotWpIndex[i]=INVALID_VALUE;
	tmpSlotBrgDiff[i]=-999;
  }

  //
  // set up fine tuned parameters for this run
  //
  int maxgratio=1;
  double maxdistance=300;
  if (ISPARAGLIDER) {
	maxgratio=2;
	maxdistance=100;
  }
  if (ISGLIDER) {
	maxgratio=10;
	maxdistance=300;
  }
  if (ISCAR) {
	maxgratio=1;
	maxdistance=100;
  }

  //
  // WE USE THE 2.3 PAGE (Nearest turnpoints) sorted by DIRECTION
  //

  // We do this in several passes. 
  #define MAXPHASES	5
  unsigned short phase=1;

  #if DEBUG_GVG
  StartupStore(_T("GVGP: USING  %d Sorted Items available\n"),SortedNumber);
  int count=0;
  #endif
  _tryagain:

  for (i=0; i<numslots; i++) {
	LKASSERT(phase<=MAXPHASES);
	#if DEBUG_GVG
	if (i>=SortedNumber) {
		StartupStore(_T("...... GVGP: PHASE %d warning not enough SortedNumber (%d) for i=%d\n"),phase,SortedNumber,i);
	}
	#endif

	// look up for an empty slot, needed if running after phase 1
	if (slotWpIndex[i]!=INVALID_VALUE) continue;

	// browse results for the best usable items
	for (int k=0; k<SortedNumber; k++) {
		wpindex=*(pindex+k);
		if (!ValidWayPoint(wpindex)) {
			#if BUGSTOP
			StartupStore(_T("...... GVGP: PHASE %d invalid wpindex = %d!!\n"),phase,wpindex);
			LKASSERT(0);
			#endif
			continue;
		}

		#if DEBUG_GVG
		count++;
		#endif

		// did we already use it?
		bool alreadyused=false;
		for (int j=0; j<numslots; j++) {
			if (slotWpIndex[j]==INVALID_VALUE) break;
			if (slotWpIndex[j]==wpindex) {
				alreadyused=true;
				break;
			}
		}
		if (alreadyused) continue;

		// unused one, decide if good or not
		// We do this in 3 phases..
		double distance = WayPointCalc[wpindex].Distance;
		double brgdiff = WayPointCalc[wpindex].Bearing -  DrawInfo.TrackBearing;
                if (brgdiff < -180.0) {
			brgdiff += 360.0;
                } else {
			if (brgdiff > 180.0) brgdiff -= 360.0;
		}
		double abrgdiff=brgdiff;
		if (abrgdiff<0) abrgdiff*=-1;
		if (abrgdiff>45) continue;

		// First we insert unconditionally mountain passes and not too close task points
		if (WayPointList[wpindex].Style==STYLE_MTPASS) goto _useit;
		if ((WayPointList[wpindex].InTask) && (distance>100)) goto _useit;


		// Then we make a selective insertion, in several steps
		// Normally we use only phase 1
		if (phase==1) {
			// if we are practically over the waypoint, skip it 
			if (distance<maxdistance) continue;
			// if we are within an obvious glide ratio, no need to use it at all costs
			if (WayPointCalc[wpindex].AltArriv[AltArrivMode]>50) {
				if ( WayPointCalc[wpindex].GR<=maxgratio) continue;
			}

		}
		if (phase==2) {
			if (distance<maxdistance) continue;
			// use mountain tops, if not too close and not too obviously reachable
			if (WayPointList[wpindex].Style!=STYLE_MTTOP) continue;
			if (WayPointCalc[wpindex].AltArriv[AltArrivMode]>50) {
				if ( WayPointCalc[wpindex].GR<=(maxgratio*2)) continue;
			}
		}
		if (phase==3) {
			if (distance<maxdistance) continue;
			if (WayPointCalc[wpindex].AltArriv[AltArrivMode]>50) {
				if ( WayPointCalc[wpindex].GR<=(maxgratio*2)) continue;
			}
		}
		if (phase==4) {
			if (distance<maxdistance) continue;
		}


		// if (phase==MAXPHASES)  do nothing, simply accept the waypoint!


_useit:

		// ok good, use it
		slotWpIndex[i]=wpindex;
		tmpSlotBrgDiff[i]=brgdiff;

		#if DEBUG_GVG
		StartupStore(_T("PHASE %d  slot [%d] of %d : wp=%d <%s> brg=%f\n"),
			phase, i, numslots, wpindex, WayPointList[wpindex].Name,brgdiff);
		#endif
		currentFilledNumber++;
		break;
	} // for all sorted wps

	if (currentFilledNumber>=numslots) {
		#if DEBUG_GVG
		StartupStore(_T("PHASE %d  stop search, all slots taken\n"),phase);
		#endif
		break;
	}
  } // for all slots to be filled

  if ((currentFilledNumber<numslots) && (phase<MAXPHASES)) {
	#if DEBUG_GVG
	StartupStore(_T("PHASE %d  filled %d of %d slots, going phase %d\n"),phase,currentFilledNumber,numslots,phase+1);
	#endif
	phase++;
	goto _tryagain;
  }
  #if DEBUG_GVG
  else {
	StartupStore(_T("PHASE %d  filled %d of %d slots\n"),phase,currentFilledNumber,numslots);
  }
  StartupStore(_T("TOTAL COUNTS=%d\n"),count);
  #endif


  //
  // All wpts in the array are shuffled, unsorted by direction.
  // We must reposition them horizontally, using their bearing
  //
  int tmpSlotWpIndex[MAXBSLOT+1];
  for (i=0; i<MAXBSLOT; i++) tmpSlotWpIndex[i]=INVALID_VALUE;

  #if DEBUG_GVG
  for (i=0; i<numslots; i++) {
	StartupStore(_T(">>> [%d] %f  (wp=%d)\n"),i,tmpSlotBrgDiff[i],slotWpIndex[i]);
  }
  #endif

  bool invalid=true, valid=false;
  unsigned short g;

  //#define DEBUGSORT 1

  for (unsigned short nslot=0; nslot<numslots; nslot++) {
	g=0;
  	double minim=999;
	valid=false;
	#if DEBUGSORT
	StartupStore(_T(".... Slot [%d]:\n"),  nslot);
	#endif
	for (unsigned short k=0; k<numslots; k++) {
		if (tmpSlotBrgDiff[k]<=minim ) {
			// is this already used?
			invalid=false;
			if ( slotWpIndex[k]==-1 ) {
				#if DEBUGSORT
				StartupStore(_T(".... not using g=%d, it is an invalid waypoint\n"),k);
				#endif
				continue;
			}
			for (unsigned short n=0; n<nslot; n++) {
				if ( tmpSlotWpIndex[n]==slotWpIndex[k]) {
					#if DEBUGSORT
					StartupStore(_T(".... not using g=%d, it is already used in newslot=%d\n"),k,n);
					#endif
					invalid=true;
					continue;
				}
			}

			if (invalid || !ValidWayPoint(slotWpIndex[k])) continue;

			// We do have a valid choice 
			g=k;
			minim=tmpSlotBrgDiff[k];
			#if DEBUGSORT
			StartupStore(_T(".... minim=%f g=%d\n"),minim, g);
			#endif
			valid=true;
		}
	}

	if (valid) {
		tmpSlotWpIndex[nslot]=slotWpIndex[g];
		#if DEBUGSORT
		StartupStore(_T(".... FINAL for SLOT %d:  minim=%f g=%d\n"),nslot,minim, g);
		#endif
	}
	#if DEBUGSORT
	else {
		StartupStore(_T(".... FINAL for SLOT %d:  no valid point\n"),nslot);
	}
	#endif
  }
			

  for (i=0; i<numslots; i++) {
	slotWpIndex[i]=tmpSlotWpIndex[i];
  }
 
  return currentFilledNumber;
}
