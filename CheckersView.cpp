
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
using namespace game;
constexpr auto
min_text_height{ 18 },
min_text_width{ 20 };

// CCheckersView

IMPLEMENT_DYNCREATE(CCheckersView, CView)

BEGIN_MESSAGE_MAP(CCheckersView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

// CCheckersView construction/destruction

CCheckersView::CCheckersView() noexcept
	:m_brushWhite{ RGB(180,20,20) }
	, m_brushBlack{ RGB(20,180,20) }
	, m_penSelect{ PS_SOLID,3,RGB(220,220,20) }
	, m_penPossible{ PS_SOLID,3,RGB(20,220,20) }
{
	// TODO: add construction code here

}

CCheckersView::~CCheckersView()
{}

BOOL CCheckersView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CCheckersView drawing

void CCheckersView::OnDraw(CDC* pDC)
{
	CCheckersDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	auto const iSave{ memDC.SaveDC() };
	memDC.SelectObject(m_bmpCanvas);

	pDC->BitBlt(m_rCanvas.left, m_rCanvas.top, m_rCanvas.Width(), m_rCanvas.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.RestoreDC(iSave);
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

BOOL CCheckersView::OnEraseBkgnd(CDC* pDC)
{
	m_bmpCanvas.DeleteObject();
	m_bmpCanvas.CreateCompatibleBitmap(pDC, m_rCanvas.Width(), m_rCanvas.Height());

	UpdatePicture(pDC);

	//return CView::OnEraseBkgnd(pDC);
	return TRUE;
}

void CCheckersView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	m_rCanvas = { 0, 0, cx, cy };

	/*m_bmpCanvas.DeleteObject();
	CClientDC dc{ this };
	m_bmpCanvas.CreateCompatibleBitmap(&dc, cx, cy);
	DrawCanvas(dc);*/
}

void CCheckersView::DrawCanvas(CDC& dc)
{
	//	calculate layout
	auto rect{ m_rCanvas };
	rect.OffsetRect(-rect.left, -rect.top);
	int minSide{ min(rect.Width() - 2 * min_text_width,rect.Height() - 2 * min_text_height) };
	if (minSide < 1)
		return;

	rect.right = rect.left + minSide + 2 * min_text_width,
		rect.bottom = rect.top + minSide + 2 * min_text_height;


	m_rBoard = rect;
	m_rBoard.left += min_text_width;
	m_rBoard.right -= min_text_width;
	m_rBoard.top += min_text_height;
	m_rBoard.bottom -= min_text_height;

	if (m_rBoard.IsRectEmpty())
		return;

	dc.SetTextColor(RGB(180, 180, 50));
	dc.SetBkMode(TRANSPARENT);

	//	draw letters
	auto draw_letters = [this, &dc](CRect r)
		{
			CString str;
			for (size_t i = 0; i < 8; ++i)
			{
				r.left = r.right;
				r.right = m_rBoard.left + (int)((i + 1) * m_rBoard.Width() / 8.);
				str.Empty();
				str.AppendChar(L'A' + wchar_t(i));
				dc.DrawText(str, r, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			}
		};

	rect = m_rBoard;
	rect.bottom = rect.top;
	rect.top -= min_text_height;
	rect.right = rect.left;
	draw_letters(rect);
	rect.top = m_rBoard.bottom;
	rect.bottom = rect.top + min_text_height;
	draw_letters(rect);

	//	draw numbers
	auto draw_numbers = [this, &dc](CRect r)
		{
			CString str;
			for (size_t i = 0; i < 8; ++i)
			{
				r.top = r.bottom;
				r.bottom = m_rBoard.top + (int)((i + 1) * m_rBoard.Width() / 8.);
				str.Empty();
				str.AppendChar(L'8' - wchar_t(i));
				dc.DrawText(str, r, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			}
		};
	rect = m_rBoard;
	rect.right = rect.left;
	rect.left -= min_text_width;
	rect.bottom = rect.top;
	draw_numbers(rect);
	rect.left = m_rBoard.right;
	rect.right = rect.left + min_text_width;
	draw_numbers(rect);

	//	draw board
	rect = m_rBoard;
	rect.InflateRect(1, 1);
	dc.Draw3dRect(rect, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DSHADOW));
	auto& brd{ GetDocument()->GetBoard() };

	auto draw_cell = [&](Position pos)
		{
			CRect r;
			r.left = m_rBoard.left + int(pos.col * m_rBoard.Width() / 8.);
			r.right = m_rBoard.left + int((pos.col + 1) * m_rBoard.Width() / 8.);
			r.top = m_rBoard.top + int(pos.row * m_rBoard.Width() / 8.);
			r.bottom = m_rBoard.top + int((pos.row + 1) * m_rBoard.Width() / 8.);
			r.DeflateRect(1, 1);
			dc.FillSolidRect(r, pos.is_white() ? RGB(255, 255, 255) : RGB(0, 0, 0));
			if (pos == m_Selected)
			{
				auto oldPen{ dc.SelectObject(m_penSelect) };
				auto oldBrush{ dc.SelectObject(::GetStockObject(NULL_BRUSH)) };
				dc.Rectangle(r);
				dc.SelectObject(oldPen);
				//dc.SelectObject(oldBrush);
			}
			else if (GetDocument()->IsPossible2Move2(m_Selected, pos))
			{
				auto oldPen{ dc.SelectObject(m_penPossible) };
				auto oldBrush{ dc.SelectObject(::GetStockObject(NULL_BRUSH)) };
				dc.Rectangle(r);
				dc.SelectObject(oldPen);
			}

			if (auto p{ brd[pos] })
			{
				dc.SelectObject(p->color == Color::White ? m_brushWhite : m_brushBlack);
				//auto oldPen{ dc.SelectStockObject(NULL_PEN) };
				dc.Ellipse(r);
				if (p->rank == Rank::Queen)
				{
				}
				//dc.SelectObject(oldPen);
			}
		};

	//	draw each cell
	for (Row r{ 0 }; r.valid(); ++r)
		for (Column c{ 0 }; c.valid(); ++c)
			draw_cell({ r, c });
}

game::Position CCheckersView::HitTest(CPoint pnt) const
{
	pnt.x -= min_text_width,
		pnt.y -= min_text_height;
	if (m_rBoard.PtInRect(pnt))
		return { Row{int(pnt.y * 8. / m_rBoard.Height())},Column{int(pnt.x * 8. / m_rBoard.Width())} };
	else return {};
}

void CCheckersView::UpdatePicture()
{
	CClientDC dc{ this };
	UpdatePicture(&dc);
	Invalidate();
}

void CCheckersView::UpdatePicture(CDC* pDC)
{
	CDC memDC;
	memDC.CreateCompatibleDC(pDC);
	int iSave{ memDC.SaveDC() };
	memDC.SelectObject(m_bmpCanvas);
	DrawCanvas(memDC);
	memDC.RestoreDC(iSave);
}

void CCheckersView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	auto const oldVal{ m_Selected };
	auto const pos{ HitTest(point) };
	if (!pos)
		return;

	if (pos.is_white())
		m_Selected = {};
	else
	{
		if (m_Selected == Position{})
		{
			if (GetDocument()->IsMoveable(pos))
				m_Selected = pos;
		}
		else
		{
			auto m{ GetDocument()->FindMove(m_Selected,pos) };
			if (m.empty())
			{
				if (GetDocument()->IsMoveable(pos))
					m_Selected = pos;
				else m_Selected = {};
			}
			else
			{
				GetDocument()->MakeMove(m);
				m_Selected = {};
			}
		}
	}

	if (m_Selected != oldVal)
		UpdatePicture();

	//CView::OnLButtonDown(nFlags, point);
}
