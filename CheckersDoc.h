
// CheckersDoc.h : interface of the CCheckersDoc class
//


#pragma once
#include "defines.h"

class CCheckersDoc : public CDocument
{
	using Moves = std::vector<game::Move>;
protected: // create from serialization only
	CCheckersDoc() noexcept;
	DECLARE_DYNCREATE(CCheckersDoc)

	// Attributes
public:
	game::Board const& GetBoard()const;
	BOOL IsMoveable(game::Position)const;
	BOOL IsPossible2Move2(game::Position root, game::Position testPos)const;
	game::Move FindMove(game::Position from, game::Position to)const;
	void MakeMove(game::Move const&);
	// Operations
public:

	// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

	// Implementation
public:
	virtual ~CCheckersDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	static void AutoMoveProc(HWND, UINT, UINT_PTR, DWORD);
	// Generated message map functions
protected:
	afx_msg void OnUpdateIdsIndicatorWhite(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorPossibleMoves(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorBlack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorGameCount(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorMoveCount(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

private:
	BOOL IsHuman(game::Color)const;
	void EndGame();

	BOOL m_isWhiteHuman, m_isBlackHuman;
	game::Checkers m_Game;
	Moves m_PossibleMoves,m_History;
	UINT_PTR m_idTimer;
	SIZE_T m_uGameCount, m_uMoveCount;
};
