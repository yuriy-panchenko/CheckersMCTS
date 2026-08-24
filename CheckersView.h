
// CheckersView.h : interface of the CCheckersView class
//

#pragma once


class CCheckersView : public CView
{
protected: // create from serialization only
	CCheckersView() noexcept;
	DECLARE_DYNCREATE(CCheckersView)

	// Attributes
public:
	CCheckersDoc* GetDocument() const;

	// Operations
public:

	// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

	// Implementation
public:
	virtual ~CCheckersView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	void DrawCanvas(CDC&);
	game::Position HitTest(CPoint)const;
	void UpdatePicture();
	void UpdatePicture(CDC *pDC);

private:
	CBitmap m_bmpCanvas;
	CRect m_rCanvas, m_rBoard;
	CBrush m_brushWhite, m_brushBlack;
	CPen m_penSelect,m_penPossible;
	game::Position m_Selected;
};

#ifndef _DEBUG  // debug version in CheckersView.cpp
inline CCheckersDoc* CCheckersView::GetDocument() const
{
	return reinterpret_cast<CCheckersDoc*>(m_pDocument);
}
#endif

