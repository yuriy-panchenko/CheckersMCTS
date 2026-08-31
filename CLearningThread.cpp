// CLearningThread.cpp : implementation file
//

#include "pch.h"
#include "Checkers.h"
#include "CLearningThread.h"


// CLearningThread

IMPLEMENT_DYNCREATE(CLearningThread, CWinThread)

CLearningThread::CLearningThread()
{
}

CLearningThread::~CLearningThread()
{
}

BOOL CLearningThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	return TRUE;
}

int CLearningThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CLearningThread, CWinThread)
END_MESSAGE_MAP()


// CLearningThread message handlers
