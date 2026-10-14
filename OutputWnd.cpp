
#include "pch.h"
#include "framework.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CHART_DATA_FILENAME	_T("chart.bin")
#define CHAMP_CHART_FILENAME	_T("champion_chart.bin")
#define ANCHOR_CHART_FILENAME	_T("anchor_chart.bin")

/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputWnd::COutputWnd() noexcept
{}

COutputWnd::~COutputWnd()
{}

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create tabs window:
	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, ID_OUTPUT_TABS))
	{
		TRACE0("Failed to create output tab window\n");
		return -1;      // fail to create
	}

	// Create output panes:
	const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

	if (!m_wndOutputBuild.Create(dwStyle, rectDummy, &m_wndTabs, ID_OUTPUT_BUILD)
		|| !m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, ID_OUTPUT_DEBUG)
		|| !m_wndChart.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect{}, &m_wndTabs, ID_CHART_WND)
		|| !m_wndChampChart.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect{}, &m_wndTabs, ID_CHAMP_CHART_WND)
		|| !m_wndAnchorChart.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect{}, &m_wndTabs, ID_ANCHOR_CHART_WND)
		|| !m_wndMatch.Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect{}, &m_wndTabs, ID_MATCH_WND)
		/*
		||!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4)*/)
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}

	// OnCreate: before UpdateFonts()
	if (!m_wndSummary.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_ENDELLIPSIS, rectDummy, this))
	{
		TRACE0("Failed to create summary\n");
		return -1;
	}

	// UpdateFonts
	if (m_wndSummary.GetSafeHwnd())
		m_wndSummary.SetFont(&afxGlobalData.fontRegular);

	UpdateFonts();

	CString strTabName;
	BOOL bNameValid;

	// Attach list windows to tab:
	bNameValid = strTabName.LoadString(IDS_BUILD_TAB);
	ASSERT(bNameValid);
	UINT u{};
	m_wndTabs.AddTab(&m_wndOutputBuild, strTabName, u++);

	bNameValid = strTabName.LoadString(IDS_DEBUG_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, u++);

	m_wndTabs.AddTab(&m_wndChart, _T("Chart"), 2u);
	m_wndTabs.AddTab(&m_wndChampChart, _T("Vs champion"), u++);
	m_wndTabs.AddTab(&m_wndAnchorChart, _T("Vs random"), u++);
	m_wndTabs.AddTab(&m_wndMatch, _T("Match"), u++);
	/*
	bNameValid = strTabName.LoadString(IDS_FIND_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)2);*/

	// Fill output tabs with some dummy text (nothing magic here)
	FillBuildWindow();
	//FillDebugWindow();
	//FillFindWindow();
	std::ifstream s{ CHART_DATA_FILENAME, std::ios::binary };
	if (s)
		m_wndChart.Load(s, std::filesystem::file_size(CHART_DATA_FILENAME) / sizeof(double));
	std::ifstream sc{ CHAMP_CHART_FILENAME, std::ios::binary };
	if (sc)
		m_wndChampChart.Load(sc, std::filesystem::file_size(CHAMP_CHART_FILENAME) / sizeof(double));
	std::ifstream sa{ ANCHOR_CHART_FILENAME, std::ios::binary };
	if (sa)
		m_wndAnchorChart.Load(sa, std::filesystem::file_size(ANCHOR_CHART_FILENAME) / sizeof(double));
	//m_wndChart.SetFocus();
	return 0;
}

void COutputWnd::SetSummary(CString const& text)
{
	m_wndSummary.SetWindowText(text);
}

void COutputWnd::SetMatchScore(MatchScore const& score)
{
	m_wndMatch.m_Score = score;
	m_wndMatch.Invalidate();
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	// OnSize: replace the body
	CDockablePane::OnSize(nType, cx, cy);
	int const h{ afxGlobalData.GetTextHeight() + 6 };
	if (m_wndSummary.GetSafeHwnd())
		m_wndSummary.SetWindowPos(nullptr, 0, 0, cx, h, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndTabs.SetWindowPos(nullptr, 0, h, cx, (std::max)(0, cy - h), SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void COutputWnd::FillBuildWindow()
{
	/*m_wndOutputBuild.AddString(_T("Build output is being displayed here."));
	m_wndOutputBuild.AddString(_T("The output is being displayed in rows of a list view"));
	m_wndOutputBuild.AddString(_T("but you can change the way it is displayed as you wish..."));*/
}

//void COutputWnd::FillDebugWindow()
//{
//	m_wndOutputDebug.AddString(_T("Debug output is being displayed here."));
//	m_wndOutputDebug.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputDebug.AddString(_T("but you can change the way it is displayed as you wish..."));
//}
//
//void COutputWnd::FillFindWindow()
//{
//	m_wndOutputFind.AddString(_T("Find output is being displayed here."));
//	m_wndOutputFind.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputFind.AddString(_T("but you can change the way it is displayed as you wish..."));
//}

void COutputWnd::UpdateFonts()
{
	m_wndOutputBuild.SetFont(&afxGlobalData.fontRegular);
	m_wndOutputDebug.SetFont(&afxGlobalData.fontRegular);
	m_wndChart.SetFont(&afxGlobalData.fontRegular);
	//m_wndOutputFind.SetFont(&afxGlobalData.fontRegular);
}

void COutputWnd::AddBuildString(CString const& text)
{
	m_wndOutputBuild.SetTopIndex(m_wndOutputBuild.AddString(text));
}

void COutputWnd::AddDebugString(CString const& text)
{
	m_wndOutputDebug.SetTopIndex(m_wndOutputDebug.AddString(text));
}

void COutputWnd::AddChartData(double db)
{
	m_wndChart.Add(db);
}

void COutputWnd::AddMatchData(MatchResult const& r)
{
	double vsChampion = r.hadChampion ? r.vsChampion.value() : std::numeric_limits<double>::quiet_NaN();
	double vsRandom = r.vsRandom.value();
	if (!std::isnan(vsChampion))
	{
		m_wndChampChart.Add(vsChampion);
		std::ofstream s{ CHAMP_CHART_FILENAME, std::ios::binary };
		if (s)
			m_wndChampChart.Save(s);
	}
	m_wndAnchorChart.Add(vsRandom);

	m_wndMatch.m_Result = r;
	m_wndMatch.Invalidate();

	std::ofstream s{ ANCHOR_CHART_FILENAME, std::ios::binary };
	if (s)
		m_wndAnchorChart.Save(s);
}

void COutputWnd::RemoveBuildString()
{
	int index = m_wndOutputBuild.GetCount();
	ASSERT(index > 0);
	index = m_wndOutputBuild.DeleteString(index - 1);
	if (index > 0)
		m_wndOutputBuild.SetTopIndex(index - 1);
}

void COutputWnd::ClearBuild()
{
	m_wndOutputBuild.ResetContent();
}

void COutputWnd::SaveChartData()
{
	if (m_wndChart.HasData())
	{
		std::ofstream s{ CHART_DATA_FILENAME, std::ios::binary };
		if (s)
			m_wndChart.Save(s);
	}
}

/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList() noexcept
{}

COutputList::~COutputList()
{}

BEGIN_MESSAGE_MAP(COutputList, CListBox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// COutputList message handlers

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnEditCopy()
{
	CopyToClipboard();
}

void COutputList::OnEditClear()
{
	ResetContent();
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != nullptr && pParentBar != nullptr)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}

void COutputList::CopyToClipboard()
{
	CString text;

	const int count = GetCount();

	for (int i = 0; i < count; ++i)
	{
		CString item;
		GetText(i, item);

		text += item;

		if (i < count - 1)
			text += _T("\r\n");
	}

	if (!OpenClipboard())
		return;

	EmptyClipboard();

#ifdef UNICODE
	const SIZE_T size = (text.GetLength() + 1) * sizeof(wchar_t);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (hMem)
	{
		void* pMem = GlobalLock(hMem);

		if (pMem)
		{
			memcpy(pMem, text.GetString(), size);
			GlobalUnlock(hMem);

			SetClipboardData(CF_UNICODETEXT, hMem);
			hMem = nullptr; // clipboard owns it now
		}

		if (hMem)
			GlobalFree(hMem);
	}
#else
	// ANSI version if needed
#endif

	CloseClipboard();
}
