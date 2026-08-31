
// CheckersDoc.h : interface of the CCheckersDoc class
//


#pragma once
#include "defines.h"
#include "mcts.h"

class CCheckersDoc : public CDocument
{
	using Moves = std::vector<game::Move>;
	
	struct Hist { id::zip64 state; game::Move m; };

	struct Sample
	{
		game::Color mover;          // needed to assign correct-signed outcome later
		std::vector<double> board;                  // state.encode_board(), from mover's perspective
		std::vector<size_t> legal_indices;
		std::vector<double> target_policy;          // size 896, visit_distribution: N_i / ΣN over root edges, 0 elsewhere
		double real_value = .0;
	};

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
	auto& GetTree() { return m_Tree; }
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
	afx_msg void OnUpdateIdsIndicatorAdjusted(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIdsIndicatorLearns(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS

private:
	BOOL IsHuman(game::Color)const;
	void EndGame(std::optional<game::Color>);
	void UpdatePicture(BOOL doRedraw);
	BOOL Test4Stale();
	auto const& GetGame()const { return m_Tree.current_state(); }
	Sample MakeSample()const;
	void TrainOnSamples(std::optional<game::Color>);

	BOOL m_isWhiteHuman, m_isBlackHuman;
	Moves m_PossibleMoves;
	std::map<id::zip64, size_t> m_idCount;
	UINT_PTR m_idTimer;
	SIZE_T m_uGameCount, m_uMoveCount, m_winWhite, m_winBlack;
	mcts::MCTS m_Tree;
	NNet m_Net;
	std::vector<Sample> m_Samples;
	double m_FirstValue;
public:
	afx_msg void OnWhiteHuman();
	afx_msg void OnUpdateWhiteHuman(CCmdUI* pCmdUI);
	virtual void OnCloseDocument();
};
