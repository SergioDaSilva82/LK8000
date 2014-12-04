/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: dlgStartTask.cpp,v 1.1 2011/12/21 10:29:29 root Exp root $
*/


#include "externs.h"
#include "LKProcess.h"
#include "Dialogs.h"
#include "dlgTools.h"
#include "WindowControls.h"


static WndForm *wf=NULL;

bool startIsValid = false;

static void OnCloseClicked(Window* pWnd) {
    wf->SetModalResult(mrOK);
}

static void StartTaskAnyway(bool valid) {
  startIsValid = valid;
}

static void OnStartTaskAnywayClicked(Window* pWnd) {
    StartTaskAnyway(true);
    wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  ClickNotifyCallbackEntry(OnStartTaskAnywayClicked),
  ClickNotifyCallbackEntry(OnCloseClicked),
  EndCallBackEntry()
};

void dlgStartTaskShowModal(bool *validStart, double Time, double Speed, double Altitude){

  TCHAR filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgStartTask.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
                      filename, 
		      TEXT("IDR_XML_STARTTASK"));

  if (wf) {
    WndProperty* wp;

    TCHAR Temp[80];

    wp = (WndProperty*)wf->FindByName(TEXT("prpTime"));
    if (wp) {
      Units::TimeToText(Temp, (int)TimeLocal((int)Time));
      wp->SetText(Temp);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpSpeed"));
    if (wp) {
      _stprintf(Temp, TEXT("%.0f %s"),
                (double) TASKSPEEDMODIFY * Speed, Units::GetTaskSpeedName());
      wp->SetText(Temp);
    }

    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      _stprintf(Temp, TEXT("%.0f %s"),
                (double) Altitude*ALTITUDEMODIFY, Units::GetAltitudeName());
      wp->SetText(Temp);
    }

    wf->ShowModal();
    
    delete wf;
  }
  wf = NULL;

  *validStart = startIsValid;
}

