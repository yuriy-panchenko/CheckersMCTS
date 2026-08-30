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

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
private:
	void Draw(CDC& dc, CSize const canv);

	std::vector<double> m_Data;
	CPen m_penLine;
	double m_Min, m_Max;
	BOOL m_bInitial;
};


