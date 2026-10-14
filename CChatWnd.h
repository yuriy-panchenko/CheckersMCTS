#pragma once


// CChatWnd

class CChatWnd : public CWnd
{
	DECLARE_DYNAMIC(CChatWnd)

public:
	CChatWnd();
	virtual ~CChatWnd();

	void Add(double db);

	void Save(std::ofstream&);
	void Load(std::ifstream&, size_t element_count);
	BOOL HasData()const { return !m_Data.empty(); }

	//	title + a few lines about how to read the chart are drawn in a legend box in the corner, next to the key for the two lines
	void SetInfo(LPCTSTR title, LPCTSTR hint) { m_strTitle = title, m_strHint = hint; }
	void SetMaWindow(int n) { m_nMa = n; }						//	the blue line averages this point and the n before it
	void SetFixedRange(double lo, double hi) { m_bFixed = TRUE, m_FixedMin = lo, m_FixedMax = hi; }

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
private:
	void Draw(CDC& dc, CSize const canv);
	void DrawLegend(CDC& dc, CSize const canv);

	std::vector<double> m_Data;
	COLORREF m_colMajor, m_colMinor;
	CPen m_penLine, m_penScaleMajor,m_penScaleMinor,m_penMA;
	CFont m_fontMajor, m_fontMinor;
	double m_Min, m_Max;
	BOOL m_bInitial;
	CString m_strTitle, m_strHint;
	int m_nMa{ 20 };
	BOOL m_bFixed{ FALSE };
	double m_FixedMin{}, m_FixedMax{ 1. };
};


