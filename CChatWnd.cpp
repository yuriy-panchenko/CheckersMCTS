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
	, m_penMA{ PS_SOLID, 3, RGB(20,20,180) }
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
		m_bInitial = FALSE;
		for (auto db : m_Data)
			m_Max = max(m_Max, db), m_Min = min(m_Min, db);
	}
	else m_bInitial = TRUE;
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
	DrawLegend(memDC, { canvas.Width(), canvas.Height() });

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
	if (m_Data.size() < 2)
		return;
	if (m_bFixed)		//	scores live in 0..1; a fixed scale keeps the picture comparable between matches
		m_Min = m_FixedMin, m_Max = m_FixedMax;
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

	dc.SelectObject(m_penMA);
	for (int i = 0; i < m_Data.size(); ++i)
	{
		double sum{};
		auto const from{ (std::max)(0, i - m_nMa) };
		auto u{ from };
		for (; u <= i; ++u)
			sum += m_Data[u];
		sum /= u - from;

		auto const pnt{ to_scr(i, sum) };
		if (i)
			dc.LineTo(pnt);
		else dc.MoveTo(pnt);
	}
}

void CChatWnd::DrawLegend(CDC& dc, CSize const canv)
{
	if (m_strTitle.IsEmpty())
		return;

	constexpr int pad{ 8 }, key_len{ 28 };
	int const max_w{ max(200, canv.cx / 2) };

	dc.SetBkMode(TRANSPARENT);
	auto const oldFont{ dc.SelectObject(&m_fontMajor) };

	CRect rcTitle{ 0, 0, max_w, 0 };
	dc.DrawText(m_strTitle, rcTitle, DT_CALCRECT | DT_WORDBREAK);

	dc.SelectObject(&m_fontMinor);
	CString key1, key2;
	key1 = _T("raw value (one point per entry)");
	key2.Format(_T("moving average of the last %d points"), m_nMa + 1);
	CRect rcKey{ 0, 0, max_w - key_len - 6, 0 };
	dc.DrawText(key1, rcKey, DT_CALCRECT | DT_SINGLELINE);
	CRect rcKey2{ 0, 0, max_w - key_len - 6, 0 };
	dc.DrawText(key2, rcKey2, DT_CALCRECT | DT_SINGLELINE);
	int const line_h{ (std::max)(rcKey.Height(), 14) };

	CRect rcHint{ 0, 0, max_w, 0 };
	if (!m_strHint.IsEmpty())
		dc.DrawText(m_strHint, rcHint, DT_CALCRECT | DT_WORDBREAK);

	int const box_w{ (std::max)({ (int)rcTitle.Width(), key_len + 6 + (std::max)(rcKey.Width(), rcKey2.Width()), (int)rcHint.Width() }) + 2 * pad };
	int const box_h{ pad + rcTitle.Height() + 4 + 2 * (line_h + 2) + (m_strHint.IsEmpty() ? 0 : 6 + rcHint.Height()) + pad };
	CRect box{ pad, pad, pad + box_w, pad + box_h };

	dc.FillSolidRect(box, RGB(18, 18, 24));
	CBrush brFrame{ RGB(90, 90, 100) };
	dc.FrameRect(box, &brFrame);

	int y{ box.top + pad };
	dc.SelectObject(&m_fontMajor);
	dc.SetTextColor(m_colMajor);
	CRect r{ box.left + pad, y, box.right - pad, y + rcTitle.Height() };
	dc.DrawText(m_strTitle, r, DT_WORDBREAK);
	y += rcTitle.Height() + 4;

	dc.SelectObject(&m_fontMinor);
	dc.SetTextColor(RGB(190, 190, 200));

	auto const x0{ box.left + pad };
	auto const key_row = [&](CPen& pen, CString const& text)
		{
			auto const old{ dc.SelectObject(&pen) };
			dc.MoveTo(x0, y + line_h / 2);
			dc.LineTo(x0 + key_len, y + line_h / 2);
			dc.SelectObject(old);
			CRect rt{ x0 + key_len + 6, y, box.right - pad, y + line_h };
			dc.DrawText(text, rt, DT_SINGLELINE | DT_VCENTER);
			y += line_h + 2;
		};
	key_row(m_penLine, key1);
	key_row(m_penMA, key2);

	if (!m_strHint.IsEmpty())
	{
		y += 6 - 2;
		dc.SetTextColor(m_colMinor);
		CRect rh{ x0, y, box.right - pad, y + rcHint.Height() };
		dc.DrawText(m_strHint, rh, DT_WORDBREAK);
	}

	dc.SelectObject(oldFont);
}
