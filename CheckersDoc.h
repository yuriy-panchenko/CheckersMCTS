
// CheckersDoc.h : interface of the CCheckersDoc class
//


#pragma once
#include "defines.h"
#include <stack>
#include <map>

class CCheckersDoc : public CDocument
{
	using Moves = std::vector<game::Move>;
	struct Hist { id::zip64 state; game::Move m; };
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
	void AutoMove();
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
	afx_msg void OnStartPause();
	afx_msg void OnUpdateStartPause(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

private:
	BOOL IsHuman(game::Color)const;
	void EndGame(game::Color);
	void EndGame();
	void UpdatePicture();
	BOOL Test4Stale();

	BOOL m_isWhiteHuman, m_isBlackHuman;
	game::Checkers m_Game;
	Moves m_PossibleMoves;
	std::vector<Hist> m_Undo;
	std::stack<Hist> m_Redo;
	std::map<id::zip64, size_t> m_idCount;
	UINT_PTR m_idTimer;
	SIZE_T m_uGameCount, m_uMoveCount, m_winWhite, m_winBlack, m_wNoCh, m_bNoCh;
	double m_wPosibleTotal, m_bPosTotal;
	NNet m_Net;
};
