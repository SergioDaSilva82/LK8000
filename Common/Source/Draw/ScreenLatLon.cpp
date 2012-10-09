/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"

extern POINT Sideview_Orig_InUse;
extern double Sideview_Zoom_InUse;
extern double Sideview_Angle_InUse;

// RETURNS Longitude, Latitude!
void MapWindow::OrigScreen2LatLon(const int &x, const int &y, double &X, double &Y) {
  int sx = x;
  int sy = y;
  irotate(sx, sy, DisplayAngle);
  Y= PanLatitude  - sy*zoom.InvDrawScale();
  X= PanLongitude + sx*invfastcosine(Y)*zoom.InvDrawScale();
}



void MapWindow::Screen2LatLon(const int &x, const int &y, double &X, double &Y) {
  int sx = x-(int)Orig_Screen.x;
  int sy = y-(int)Orig_Screen.y;
  irotate(sx, sy, DisplayAngle);
  Y= PanLatitude  - sy*zoom.InvDrawScale();
  X= PanLongitude + sx*invfastcosine(Y)*zoom.InvDrawScale();
}


//
// Called Sideview, but really we are talking about TOPVIEW!!
//
void MapWindow::SideviewScreen2LatLon(const int &x, const int &y, double &X, double &Y) {
  int sx = x-(int)Sideview_Orig_InUse.x;
  int sy = y-(int)Sideview_Orig_InUse.y;
  irotate(sx, sy, Sideview_Angle_InUse);
  Y= PanLatitude  - sy*Sideview_Zoom_InUse;
  X= PanLongitude + sx*invfastcosine(Y)*Sideview_Zoom_InUse;
}


void MapWindow::LatLon2Screen(const double &lon, const double &lat, POINT &sc) {
  int Y = Real2Int((PanLatitude-lat)*zoom.DrawScale());
  int X = Real2Int((PanLongitude-lon)*fastcosine(lat)*zoom.DrawScale());
    
  irotate(X, Y, DisplayAngle);
    
  sc.x = Orig_Screen.x - X;
  sc.y = Orig_Screen.y + Y;
}



// This one is optimised for long polygons
void MapWindow::LatLon2Screen(pointObj *ptin, POINT *ptout, const int n, const int skip) {
  static double lastangle = -1;
  static int cost=1024, sint=0;
  const double mDisplayAngle = DisplayAngle;

  if(mDisplayAngle != lastangle) {
    lastangle = mDisplayAngle;
    int deg = DEG_TO_INT(AngleLimit360(mDisplayAngle));
    cost = ICOSTABLE[deg];
    sint = ISINETABLE[deg];
  }
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
  const double mDrawScale = zoom.DrawScale();
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;
  pointObj* p = ptin;
  const pointObj* ptend = ptin+n;

  while (p<ptend) {
    int Y = Real2Int((mPanLatitude-p->y)*mDrawScale);
    int X = Real2Int((mPanLongitude-p->x)*fastcosine(p->y)*mDrawScale);
    ptout->x = (xxs-X*cost + Y*sint)/1024;
    ptout->y = (Y*cost + X*sint + yys)/1024;
    ptout++;
    p+= skip;
  }
}


