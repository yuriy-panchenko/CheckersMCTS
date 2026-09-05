
// Checkers.h : main header file for the Checkers application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "defines.h"

// CCheckersApp:
// See Checkers.cpp for the implementation of this class
// 
//
struct Sample
{
	game::Color mover;          // needed to assign correct-signed outcome later
	std::vector<double> board;                  // state.encode_board(), from mover's perspective
	std::unordered_set<size_t> legal_indices;
	std::vector<double> target_policy;          // size 896, visit_distribution: N_i / ΣN over root edges, 0 elsewhere
	double real_value = .0;
};

class CCheckersApp : public CWinAppEx
{
public:
	CCheckersApp() noexcept;


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	
	CDocument* GetDocument();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

};

extern CCheckersApp theApp;
