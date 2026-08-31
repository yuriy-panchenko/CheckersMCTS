#pragma once



// CLearningThread

class CLearningThread : public CWinThread
{
	DECLARE_DYNCREATE(CLearningThread)

protected:
	CLearningThread();           // protected constructor used by dynamic creation
	virtual ~CLearningThread();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
	DECLARE_MESSAGE_MAP()
};


