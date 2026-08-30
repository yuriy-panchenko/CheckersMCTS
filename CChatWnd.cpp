// CChatWnd.cpp : implementation file
//

#include "pch.h"
#include "Checkers.h"
#include "CChatWnd.h"


// CChatWnd

IMPLEMENT_DYNAMIC(CChatWnd, CWnd)

CChatWnd::CChatWnd()
	:m_penLine{ PS_SOLID, 3, RGB(220, 220, 20) }
	, m_Min{}
	, m_Max{}
	, m_bInitial{ TRUE }
{

}

CChatWnd::~CChatWnd()
{}

void CChatWnd::Add(double db)
{
	m_Data.push_back(db);
	if (m_bInitial)
		m_Max = m_Min = db, m_bInitial = FALSE;
	else
		m_Max = max(m_Max, db),
		m_Min = min(m_Min, db);

	if (IsWindowVisible())
		Invalidate();
}

void CChatWnd::Save(std::ofstream& s)
{
	s.write((char const*)m_Data.data(), m_Data.size() * sizeof(double));
}

void CChatWnd::Load(std::ifstream& s, size_t element_count)
{
	m_Data.resize(element_count);
	s.read((char*)m_Data.data(), element_count * sizeof(double));
	if (!m_Data.empty())
	{
		m_Max = m_Min = m_Data.front();
		for (auto db : m_Data)
			m_Max = max(m_Max, db), m_Min = min(m_Min, db);
	}
}


BEGIN_MESSAGE_MAP(CChatWnd, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CChatWnd message handlers



void CChatWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CWnd::OnPaint() for painting messages

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	CRect canvas;
	GetClientRect(canvas);
	CBitmap bmp;
	bmp.CreateCompatibleBitmap(&dc, canvas.Width(), canvas.Height());
	auto const iSave{ memDC.SaveDC() };
	memDC.SelectObject(bmp);

	Draw(memDC, { canvas.Width(), canvas.Height() });

	dc.BitBlt(canvas.left, canvas.top, canvas.Width(), canvas.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.RestoreDC(iSave);

}

void CChatWnd::Draw(CDC& dc, CSize const canv)
{
	//if (m_Data.empty())
	if (m_Max == m_Min)
		return;

	auto const CX{ m_Data.size() - 1ull };
	auto const CY{ m_Max - m_Min };

	//	draw horizontal lines
	{
		// we want lines to go through
	}

	auto to_scr = [&, this](int x, double y)->CPoint
		{
			return { int(x * canv.cx / CX), int(canv.cy * (m_Max - y) / CY) };
		};

	dc.SelectObject(m_penLine);
	dc.MoveTo(to_scr(0, m_Data.front()));

	for (auto iter{ std::next(m_Data.begin()) }; iter != m_Data.end(); iter++)
		dc.LineTo(to_scr((int)std::distance(m_Data.begin(), iter), *iter));
}
