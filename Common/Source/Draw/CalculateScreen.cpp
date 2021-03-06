/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "externs.h"
#include "Multimap.h"

int MapWindow::iSnailNext=0;
int MapWindow::iLongSnailNext=0;

rectObj MapWindow::screenbounds_latlon;



rectObj MapWindow::CalculateScreenBounds(double scale, const RECT& rc) {
  // compute lat lon extents of visible screen
  rectObj sb;

  if (scale>= 1.0) {
    POINT screen_center;
    LatLon2Screen(PanLongitude, 
                  PanLatitude,
                  screen_center);
    
    sb.minx = sb.maxx = PanLongitude;
    sb.miny = sb.maxy = PanLatitude;
    
    int dx, dy;
    unsigned int maxsc=0;
    dx = screen_center.x-rc.right;
    dy = screen_center.y-rc.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-rc.left;
    dy = screen_center.y-rc.top;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-rc.left;
    dy = screen_center.y-rc.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    dx = screen_center.x-rc.right;
    dy = screen_center.y-rc.bottom;
    maxsc = max(maxsc, isqrt4(dx*dx+dy*dy));
    
    for (int i=0; i<10; i++) {
      double ang = i*360.0/10;
      POINT p;
      double X, Y;
      p.x = screen_center.x + iround(fastcosine(ang)*maxsc*scale);
      p.y = screen_center.y + iround(fastsine(ang)*maxsc*scale);
      Screen2LatLon(p.x, p.y, X, Y);
      sb.minx = min(X, sb.minx);
      sb.miny = min(Y, sb.miny);
      sb.maxx = max(X, sb.maxx);
      sb.maxy = max(Y, sb.maxy);
    }

  } else {

    double xmin, xmax, ymin, ymax;
    int x, y;
    double X, Y;
    
    x = rc.left; 
    y = rc.top; 
    Screen2LatLon(x, y, X, Y);
    xmin = X; xmax = X;
    ymin = Y; ymax = Y;

    x = rc.right; 
    y = rc.top; 
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);
  
    x = rc.right; 
    y = rc.bottom; 
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);
  
    x = rc.left; 
    y = rc.bottom; 
    Screen2LatLon(x, y, X, Y);
    xmin = min(xmin, X); xmax = max(xmax, X);
    ymin = min(ymin, Y); ymax = max(ymax, Y);
  

    sb.minx = xmin;
    sb.maxx = xmax;
    sb.miny = ymin;
    sb.maxy = ymax;

  }

  return sb;
}



void MapWindow::CalculateScreenPositionsThermalSources() {
  for (int i=0; i<MAX_THERMAL_SOURCES; i++) {
    if (DerivedDrawInfo.ThermalSources[i].LiftRate>0) {
      double dh = DerivedDrawInfo.NavAltitude
        -DerivedDrawInfo.ThermalSources[i].GroundHeight;
      if (dh<0) {
        DerivedDrawInfo.ThermalSources[i].Visible = false;
        continue;
      }

      double t = dh/DerivedDrawInfo.ThermalSources[i].LiftRate;
      double lat, lon;
      FindLatitudeLongitude(DerivedDrawInfo.ThermalSources[i].Latitude, 
                            DerivedDrawInfo.ThermalSources[i].Longitude,
                            DerivedDrawInfo.WindBearing, 
                            -DerivedDrawInfo.WindSpeed*t,
                            &lat, &lon);
      if (PointVisible(lon,lat)) {
        LatLon2Screen(lon, 
                      lat, 
                      DerivedDrawInfo.ThermalSources[i].Screen);
        DerivedDrawInfo.ThermalSources[i].Visible = 
          PointVisible(DerivedDrawInfo.ThermalSources[i].Screen);
      } else {
        DerivedDrawInfo.ThermalSources[i].Visible = false;
      }
    } else {
      DerivedDrawInfo.ThermalSources[i].Visible = false;
    }
  }
}



void MapWindow::CalculateScreenPositionsAirspace(const RECT& rcDraw)
{
#ifndef HAVE_HATCHED_BRUSH
  // iAirspaceBrush is not used and don't exist if we don't have Hatched Brush
  // this is workarround for compatibility with #CalculateScreenPositionsAirspace
  constexpr int iAirspaceBrush[AIRSPACECLASSCOUNT] = {}; 
#endif
  CAirspaceManager::Instance().CalculateScreenPositionsAirspace(screenbounds_latlon, iAirspaceMode, iAirspaceBrush, rcDraw, zoom.ResScaleOverDistanceModify());
}




void MapWindow::CalculateScreenPositions(POINT Orig, RECT rc, 
                                         POINT *Orig_Aircraft)
{

  unsigned int i;

  Orig_Screen = Orig;

  if (!mode.AnyPan()) {
  
    if (GliderCenter 
        && DerivedDrawInfo.Circling 
        && (EnableThermalLocator==2)) {
      
      if (DerivedDrawInfo.ThermalEstimate_R>0) {
        PanLongitude = DerivedDrawInfo.ThermalEstimate_Longitude; 
        PanLatitude = DerivedDrawInfo.ThermalEstimate_Latitude;
        // TODO enhancement: only pan if distance of center to
        // aircraft is smaller than one third screen width

        POINT screen;
        LatLon2Screen(PanLongitude, 
                      PanLatitude, 
                      screen);

        LatLon2Screen(DrawInfo.Longitude, 
                      DrawInfo.Latitude, 
                      *Orig_Aircraft);
        if ((fabs((double)Orig_Aircraft->x-screen.x)<(rc.right-rc.left)/3)
            && (fabs((double)Orig_Aircraft->y-screen.y)<(rc.bottom-rc.top)/3)) {
          
        } else {
          // out of bounds, center on aircraft
          PanLongitude = DrawInfo.Longitude;
          PanLatitude = DrawInfo.Latitude;
        }
      } else {
        PanLongitude = DrawInfo.Longitude;
        PanLatitude = DrawInfo.Latitude;
      }
    } else {
      // Pan is off
      PanLongitude = DrawInfo.Longitude;
      PanLatitude = DrawInfo.Latitude;
    }
  }

  LatLon2Screen(DrawInfo.Longitude, 
                DrawInfo.Latitude, 
                *Orig_Aircraft);

  // very important
  screenbounds_latlon = CalculateScreenBounds(0.0, rc);

  // Old note obsoleted 121111: 
  // preserve this calculation for 0.0 until next round!
  // This is already done since screenbounds_latlon is global. Beware that DrawTrail will change it later on
  // to expand boundaries by 1 minute

  // get screen coordinates for all task waypoints

  LockTaskData();

  if (!WayPointList.empty()) {
    for (i=0; i<MAXTASKPOINTS; i++) {
      unsigned index = Task[i].Index;
      if (index < WayPointList.size()) {
        
        LatLon2Screen(WayPointList[index].Longitude, 
                      WayPointList[index].Latitude, 
                      WayPointList[index].Screen);
        WayPointList[index].Visible = 
          PointVisible(WayPointList[index].Screen);
       } else {
       	 // No need to continue.
         break;
      }      
    }
    if (EnableMultipleStartPoints) {
      for(i=0;i<MAXSTARTPOINTS-1;i++) {
        unsigned index = StartPoints[i].Index;
        if (StartPoints[i].Active && (index < WayPointList.size())) {

          LatLon2Screen(WayPointList[index].Longitude, 
                        WayPointList[index].Latitude, 
                        WayPointList[index].Screen);
          WayPointList[index].Visible = 
            PointVisible(WayPointList[index].Screen);
         } else {
           // No Need to continue.
           break;
        }
      }
    }

    // only calculate screen coordinates for waypoints that are visible

    // TODO 110203 OPTIMIZE THIS !
    for(i=0;i<WayPointList.size();i++)
      {
        WayPointList[i].Visible = false;
        if (!WayPointList[i].FarVisible) continue;
        if(PointVisible(WayPointList[i].Longitude, WayPointList[i].Latitude) )
          {
            LatLon2Screen(WayPointList[i].Longitude, WayPointList[i].Latitude,
                          WayPointList[i].Screen);
            WayPointList[i].Visible = PointVisible(WayPointList[i].Screen);
          }
      }
  }

  if(TrailActive)
  {
    iSnailNext = SnailNext; 
    iLongSnailNext = LongSnailNext; 
    // set this so that new data doesn't arrive between calculating
    // this and the screen updates
  }

  if (EnableMultipleStartPoints) {
    for(i=0;i<MAXSTARTPOINTS-1;i++) {
      if (StartPoints[i].Active && ValidWayPointFast(StartPoints[i].Index)) {
        LatLon2Screen(StartPoints[i].SectorEndLon, 
                      StartPoints[i].SectorEndLat, StartPoints[i].End);
        LatLon2Screen(StartPoints[i].SectorStartLon, 
                      StartPoints[i].SectorStartLat, StartPoints[i].Start);
      }
    }
  }
  
  for(i=0;i<MAXTASKPOINTS-1;i++)
  {
    bool this_valid = ValidTaskPointFast(i);
    bool next_valid = ValidTaskPointFast(i+1);
    if (AATEnabled && this_valid) {
      LatLon2Screen(Task[i].AATTargetLon, Task[i].AATTargetLat, 
                    Task[i].Target);
    }

    if(this_valid && !next_valid)
    {
      // finish
      LatLon2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, Task[i].End);
      LatLon2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, Task[i].Start);

   	  // No need to continue.
      break;
    }
    if(this_valid && next_valid)
    {
      LatLon2Screen(Task[i].SectorEndLon, Task[i].SectorEndLat, Task[i].End);
      LatLon2Screen(Task[i].SectorStartLon, Task[i].SectorStartLat, Task[i].Start);

      if((AATEnabled) && (Task[i].AATType == SECTOR))
      {
        LatLon2Screen(Task[i].AATStartLon, Task[i].AATStartLat, Task[i].AATStart);
        LatLon2Screen(Task[i].AATFinishLon, Task[i].AATFinishLat, Task[i].AATFinish);
      }
      if (AATEnabled && (((int)i==ActiveWayPoint) || 
			 (mode.Is(Mode::MODE_TARGET_PAN) && ((int)i==TargetPanIndex)))) {

	for (int j=0; j<MAXISOLINES; j++) {
	  if (TaskStats[i].IsoLine_valid[j]) {
	    LatLon2Screen(TaskStats[i].IsoLine_Longitude[j], 
			  TaskStats[i].IsoLine_Latitude[j], 
			  TaskStats[i].IsoLine_Screen[j]);
	  }
	}
      }
    }
  }

  UnlockTaskData();

}




void MapWindow::CalculateScreenPositionsGroundline(void) {
  bool mm=IsMultiMapSharedNoMain();
  if (FinalGlideTerrain) {
	if (mm) {
		LatLon2ScreenMultimap(DerivedDrawInfo.GlideFootPrint, Groundline, NUMTERRAINSWEEPS+1, 1);
	} else {
		LatLon2Screen(DerivedDrawInfo.GlideFootPrint, Groundline, NUMTERRAINSWEEPS+1, 1);
	}
	#ifdef GTL2
	if (FinalGlideTerrain > 2) {// show next-WP line
		if (mm) {
			LatLon2ScreenMultimap(GlideFootPrint2, Groundline2, NUMTERRAINSWEEPS+1, 1);
		} else {
			LatLon2Screen(GlideFootPrint2, Groundline2, NUMTERRAINSWEEPS+1, 1);
		}
	}
	#endif
  }
}


