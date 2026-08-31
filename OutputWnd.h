
#pragma once

/////////////////////////////////////////////////////////////////////////////
// COutputList window
#include "CChatWnd.h"
class COutputList : public CListBox
{
	// Construction
public:
	COutputList() noexcept;

	// Implementation
public:
	virtual ~COutputList();

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();

	DECLARE_MESSAGE_MAP()
	void CopyToClipboard();
};

class COutputWnd : public CDockablePane
{
	// Construction
public:
	COutputWnd() noexcept;

	void UpdateFonts();
	void AddBuildString(CString const&);
	void AddDebugString(CString const&);
	void AddChartData(double);
	void RemoveBuildString();
	void ClearBuild();
	void SaveChartData();

	// Attributes
protected:
	CMFCTabCtrl	m_wndTabs;

	COutputList m_wndOutputBuild;
	COutputList m_wndOutputDebug;
	CChatWnd m_wndChart;
	//COutputList m_wndOutputFind;

protected:
	void FillBuildWindow();
	void FillDebugWindow();
	void FillFindWindow();

	void AdjustHorzScroll(CListBox& wndListBox);

	// Implementation
public:
	virtual ~COutputWnd();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

