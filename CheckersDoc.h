
// CheckersDoc.h : interface of the CCheckersDoc class
//


#pragma once
#include "defines.h"
#include "CLearningThread.h"

class CCheckersDoc : public CDocument
{
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
	void HumanMove(game::Move const&);		//	view -> thread
	void OnMoveFinished();					//	thread -> UI, called from the view on WM_MOVE_FINISHED
	// Operations
public:

	// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
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
	// Generated message map functions
protected:
	afx_msg void OnUpdateIdsIndicatorWhite(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorPossibleMoves(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorBlack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorGameCount(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorMoveCount(CCmdUI* pCmdUI);
	afx_msg void OnStartPause();
	afx_msg void OnUpdateStartPause(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorAdjusted(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorLearns(CCmdUI* pCmdUI);
	afx_msg void OnWhiteHuman();
	afx_msg void OnUpdateWhiteHuman(CCmdUI* pCmdUI);
	afx_msg void OnNewGame();
	afx_msg void OnUpdateNewGame(CCmdUI* pCmdUI);
	afx_msg void OnNextStep();
	afx_msg void OnUpdateNextStep(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

private:
	BOOL IsHuman(game::Color)const;
	void UpdatePicture(BOOL doRedraw);
	void KillLearner();
	void Kick();		//	ask the thread for the next AI move if that is what should happen now

	BOOL m_isWhiteHuman, m_isBlackHuman;
	BOOL m_bRunning;	//	AI moves allowed (Start/Pause)
	BOOL m_bBusy;		//	a message to the thread is in flight; cleared by WM_MOVE_FINISHED
	BOOL m_bReady;		//	the thread has published its first position (its queue exists)
	GameView m_View;	//	UI-thread copy of the thread's state; the only game data the UI reads
	CLearningThread* m_pLearnTh;
};
