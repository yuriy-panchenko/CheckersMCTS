#pragma once
#include <deque>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <optional>
#include <string>
#include "defines.h"

class GameStats
{
public:
	enum class Reason { NoMoves, Stale, NoProgress, KvK };   // Stale = old repetition rule, kept so old CSV rows load

	static char const* name(Reason r)
	{
		return r == Reason::Stale ? "Stale" : r == Reason::NoProgress ? "NoProgress" : r == Reason::KvK ? "KvK" : "NoMoves";
	}
	static Reason parse(std::string const& s)
	{
		return s == "Stale" ? Reason::Stale : s == "NoProgress" ? Reason::NoProgress : s == "KvK" ? Reason::KvK : Reason::NoMoves;
	}

	struct Record
	{
		size_t game;
		std::optional<game::Color> winner;   // nullopt = draw
		Reason reason;
		size_t plies;                        // both sides' turns
		double policy_loss, value_loss;
		int max_quiet{};                     // highest no-progress counter reached
		size_t max_repeats{};                // highest occurrence count of any position (3 = repetition draw)
		double q_absmax{};                   // largest |root value| over the game
		double q_last{};                     // root value at the final move (mover = the player who made it)
		int men_w{}, kings_w{}, men_b{}, kings_b{};   // pieces on the board when the game ended
	};

	struct Tally
	{
		size_t games{}, white{}, black{}, draws{}, stale{}, plies_sum{}, plies_max{};

		void add(Record const& r)
		{
			++games;
			if (!r.winner) ++draws;
			else if (*r.winner == game::Color::White) ++white;
			else ++black;
			if (r.reason != Reason::NoMoves) ++stale;   // any draw ending (repetition, no progress or king vs king)
			plies_sum += r.plies;
			plies_max = (std::max)(plies_max, r.plies);
		}
		double avg_plies() const { return games ? double(plies_sum) / games : 0.; }
	};

	explicit GameStats(size_t window = 50) : m_Window{ window } {}

	size_t games() const { return m_All.games; }

	void add(Record const& r)
	{
		m_All.add(r);
		m_Recent.push_back(r);
		if (m_Recent.size() > m_Window)
			m_Recent.pop_front();
	}

	CString summary() const
	{
		Tally recent;
		for (auto const& r : m_Recent)
			recent.add(r);

		CString str;
		str.Format(_T("G %zu | W %zu  B %zu  D %zu | last%zu: %zu/%zu/%zu | plies avg %.0f max %zu"),
			m_All.games, m_All.white, m_All.black, m_All.draws,
			m_Window, recent.white, recent.black, recent.draws,
			m_All.avg_plies(), m_All.plies_max);
		return str;
	}

	// Same columns as the Game Log sheet in game_stats_plan.xlsx
	static void append_csv(std::filesystem::path const& p, Record const& r)
	{
		bool const fresh{ !std::filesystem::exists(p) };
		std::ofstream s{ p, std::ios::app };
		if (!s)
			return;
		if (fresh)
			s << "game,winner,reason,plies,policy_loss,value_loss,max_quiet,max_repeats,q_absmax,q_last,men_w,kings_w,men_b,kings_b\n";
		s << r.game << ','
			<< (!r.winner ? 'D' : *r.winner == game::Color::White ? 'W' : 'B') << ','
			<< name(r.reason) << ','
			<< r.plies << ',' << r.policy_loss << ',' << r.value_loss << ','
			<< r.max_quiet << ',' << r.max_repeats << ',' << r.q_absmax << ',' << r.q_last << ','
			<< r.men_w << ',' << r.kings_w << ',' << r.men_b << ',' << r.kings_b << '\n';
	}

	// Rebuild counters after a restart (net.bin persists, so the stats should too)
	void load_csv(std::filesystem::path const& p)
	{
		std::ifstream s{ p };
		std::string line;
		std::getline(s, line);   // header
		while (std::getline(s, line))
		{
			std::istringstream ls{ line };
			std::string f[14];
			for (auto& x : f)
				std::getline(ls, x, ',');
			try
			{
				std::optional<game::Color> w;
				if (f[1] == "W") w = game::Color::White;
				else if (f[1] == "B") w = game::Color::Black;

				// the columns after value_loss are missing in rows written by older versions
				auto const opt{ [](std::string const& x) { return x.empty() ? 0. : std::stod(x); } };
				add({ std::stoull(f[0]), w, parse(f[2]),
					std::stoull(f[3]), std::stod(f[4]), std::stod(f[5]),
					int(opt(f[6])), size_t(opt(f[7])), opt(f[8]), opt(f[9]),
					int(opt(f[10])), int(opt(f[11])), int(opt(f[12])), int(opt(f[13])) });
			}
			catch (...) {}   // skip a damaged line
		}
	}

private:
	size_t m_Window;
	Tally m_All;
	std::deque<Record> m_Recent;
};