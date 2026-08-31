// CChatWnd.cpp : implementation file
//

#include "pch.h"
#include "Checkers.h"
#include "CChatWnd.h"


// CChatWnd

IMPLEMENT_DYNAMIC(CChatWnd, CWnd)

CChatWnd::CChatWnd()
	:m_penLine{ PS_SOLID, 1, RGB(220, 220, 20) }
	, m_colMajor{ RGB(220, 180, 70) }
	, m_colMinor{ RGB(125, 135, 150) }
	, m_penScaleMajor{ PS_SOLID, 1, RGB(110, 110, 110) }
	, m_penScaleMinor{ PS_DOT, 1, RGB(80, 80, 80) }
	, m_Min{}
	, m_Max{}
	, m_bInitial{ TRUE }
{
	m_fontMinor.CreateFont(
		14,                    // height
		0,                     // width
		0,                     // escapement
		0,                     // orientation
		FW_NORMAL,             // weight
		FALSE,                 // italic
		FALSE,                 // underline
		FALSE,                 // strikeout
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_SWISS,
		L"Segoe UI"
	);

	// Major font
	m_fontMajor.CreateFont(
		16,                    // slightly larger
		0,
		0,
		0,
		FW_SEMIBOLD,           // <-- noticeably stronger
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_SWISS,
		L"Segoe UI"
	);
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

	/*memDC.SelectObject(m_penLine);
	memDC.MoveTo(100, 100);
	memDC.LineTo(100, 120);
	memDC.MoveTo(120, 100);
	memDC.LineTo(120, 200);*/

	dc.BitBlt(canvas.left, canvas.top, canvas.Width(), canvas.Height(), &memDC, 0, 0, SRCCOPY);

	memDC.RestoreDC(iSave);

}

void CChatWnd::Draw(CDC& dc, CSize const canv)
{
	if (m_Max == m_Min)
		return;

	auto const CX{ m_Data.size() - 1ull };
	auto const CY{ m_Max - m_Min };

	int exponent{ 0 };
	auto find_grid_step =
		[&exponent](double range, int pixels, bool bMajor)
		{
			constexpr double min_pixels_between_lines{ 30. };
			double const desired_step{ range / pixels * min_pixels_between_lines };
			exponent = static_cast<int>(std::floor(std::log10(desired_step)));
			double power{ std::pow(10.0, exponent) };
			double const normalized{ desired_step / power };

			double nice;
			if (normalized <= 1.)
				nice = bMajor ? 2. : 1.;
			else if (normalized <= 2.)
				nice = bMajor ? 5. : 2.;
			else if (normalized <= 5.)
				nice = bMajor ? 10. : 5.;
			else
			{
				nice = bMajor ? 2. : 10.;

				if (bMajor)
					power *= 10.;
			}

			return nice * power;
		};

	//	draw horizontal lines
	{
		auto toY = [&](double dbY) {return int(dbY * canv.cy / CY); };

		auto format_value = [](double value)
			{
				CString str;
				str.Format(L"%.10f", value);

				// Remove trailing zeros
				str.TrimRight(L'0');

				// Remove trailing decimal point
				str.TrimRight(L'.');

				return str;
			};

		auto draw_lines = [&](CPen const& pen, CFont const& font, COLORREF color, double step_size, bool isMajor)
			{
				auto const index_start{ static_cast<__int64>(std::ceil(m_Min / step_size)) },
					index_end{ static_cast<__int64>(std::floor(m_Max / step_size)) };
				dc.SelectObject(pen);
				dc.SelectObject(font);
				dc.SetTextColor(color);
				for (auto index{ index_start }; index <= index_end; ++index)
				{
					auto const points_at_line{ index * step_size };
					auto const y{ toY(m_Max - points_at_line) };
					dc.MoveTo(0, y);
					dc.LineTo(canv.cx, y);

					int precision = (std::max)(0, -exponent);

					CRect rect{};
					CString str{ format_value(points_at_line) };
					dc.DrawText(str, rect, DT_CALCRECT);
					if (isMajor)
						rect.InflateRect(3, 2);
					else rect.InflateRect(3, 1);
					CRect r;
					r.left = 0;
					r.right = rect.Width();
					r.top = y - rect.Height() / 2;
					r.bottom = r.top + rect.Height();

					dc.Rectangle(r);
					dc.DrawText(str, r, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

					rect = r;
					r.right = canv.cx;
					r.left = r.right - rect.Width();
					dc.Rectangle(r);
					dc.DrawText(str, r, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
				}
			};

		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(150, 150, 40));
		dc.SelectObject(::GetStockObject(BLACK_BRUSH));

		draw_lines(m_penScaleMinor, m_fontMinor, m_colMinor, find_grid_step(CY, canv.cy, false), false);
		draw_lines(m_penScaleMajor, m_fontMajor, m_colMajor, find_grid_step(CY, canv.cy, true), true);
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
