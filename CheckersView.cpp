
// CheckersView.cpp : implementation of the CCheckersView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "Checkers.h"
#endif

#include "CheckersDoc.h"
#include "CheckersView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCheckersView

IMPLEMENT_DYNCREATE(CCheckersView, CView)

BEGIN_MESSAGE_MAP(CCheckersView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CCheckersView construction/destruction

CCheckersView::CCheckersView() noexcept
{
	// TODO: add construction code here

}

CCheckersView::~CCheckersView()
{
}

BOOL CCheckersView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CCheckersView drawing

void CCheckersView::OnDraw(CDC* /*pDC*/)
{
	CCheckersDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}

void CCheckersView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CCheckersView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CCheckersView diagnostics

#ifdef _DEBUG
void CCheckersView::AssertValid() const
{
	CView::AssertValid();
}

void CCheckersView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CCheckersDoc* CCheckersView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCheckersDoc)));
	return (CCheckersDoc*)m_pDocument;
}
#endif //_DEBUG


// CCheckersView message handlers
