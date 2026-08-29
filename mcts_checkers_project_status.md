# MCTS Checkers Project — Status

Purpose: AlphaZero-style engine for international/Russian draughts (flying kings, backward captures for men). C++, custom NN from scratch (no ML libraries). Language: C++20 (relies on parenthesized aggregate init for `Node`).

Upload this file to the Project's file list so any new thread has full context without re-explaining.

## Encoding

- **Board**: 128 doubles = 32 dark squares × 4 planes (own man / own king / opp man / opp king). Perspective-normalized to side-to-move: `Board::encode_board(bool isWhite)` mirrors square order when `isWhite == false`. Dark-square index = raw 64-cell index ÷ 2.
- **Policy**: 896 slots = 32 squares × 4 directions (Wind: NW/NE/SW/SE) × 7 distances. `Jump::to_policy_index(bool isWhite)` computes `square_id*28 + dir*7 + (distance-1)`, mirroring square_id (`31 - id`) and direction (`3 - dir`) when not White's turn.
- **Move granularity**: MCTS edges = single jumps, NOT full chains. `Board::available_moves()` returns complete multi-jump `Move` chains; MCTS only uses each chain's *first* `Jump` as the edge (deduped by policy index), then re-expands after applying it — multi-jump chains unfold across multiple tree levels where the mover doesn't change.

## Game engine (`game::Checkers`, existing/stable)

- `GetAvailableMoves()`: full legal move chains for current player (or continuation if `forced` set).
- `GetContinuation(Position)`: legal continuations from a forced (mid-chain) position.
- `Do(Jump const&)` → `bool`: applies ONE jump. Returns `true` if still forced/mid-chain (same player continues), `false` if turn passed (auto-switches `next_move` internally).
- `Do(Move const&)`: applies a full chain at once (used for committing final chosen move, not during simulation).
- `WhoMakesTurn()`, `IsWhiteTurn()`: query mover.
- `encode_board()`, `encode_legal_moves()`, `Checkers::mask_and_softmax(logits, legal_indices)`: static helper, masks illegal policy entries and softmaxes the rest.
- No `IsTerminal()` method — terminal = `GetAvailableMoves().empty()` (no legal moves = loss for mover to move).
- No draw/repetition detection yet (known gap).

## Neural net (`chk::net`, existing/stable)

- Dual-head MLP, ReLU trunk (128→256→256→128), identity policy head (896 raw logits, softmax external), ReLU→tanh value head (64→1).
- `net.think(vdb const& input)` — mutates internal state.
- `net.policy_logits()` returns a **reference** into internal buffer — must be copied out immediately, gets overwritten by next `think()` call.
- `net.value()` — scalar in [-1, 1].
- **Important constraint**: `chk::acson` stores `double const& m_Input` (reference into previous layer's buffer, not owned). Consequence: `net` is single-instance/single-threaded only — cannot copy a `net`, cannot run concurrent evaluations, no batching without redesign. Known limitation, not currently blocking anything.

## MCTS (`mcts.h/.cpp`) — IMPLEMENTED AND TESTED

```cpp
struct Edge { int action_index; game::Jump j; double P; int N=0; double W=0.0; double Q=0.0; std::unique_ptr<Node> child; };
struct Node { game::Checkers state; bool expanded=false; bool terminal=false; double terminal_value=0.0; std::vector<Edge> edges; };

class MCTS {
public:
    MCTS(game::Checkers const& initial_state, chk::net& net, double c_puct = 1.5);
    void run_simulation();
    game::Move select_move() const;
    void debug_dump_root(int top_n = 10) const;
private:
    double expand(Node* pNode, std::vector<game::Move> const& legal_moves);
    Edge& select_edge(Node& node);
    double select_and_expand(Node* pNode);
    static std::vector<double> mask_and_softmax(...);  // duplicate of Checkers::mask_and_softmax, could be deduped
    std::unique_ptr<Node> root;
    chk::net& net;
    double c_puct;
};
```

Key logic confirmed correct by review + empirical test:
- `expand()`: runs net once, builds one Edge per unique first-jump (deduped by policy index), applies mask_and_softmax for priors, returns value.
- `select_edge()`: standard PUCT — `argmax(Q + c_puct * P * sqrt(N_parent) / (1+N))`.
- `select_and_expand()`: recursive select→expand→backup. **Value sign flip only happens when `WhoMakesTurn()` actually changes between parent and child** — mid-chain jumps (same player continues) do NOT flip. This was the highest-risk part of the design and is implemented correctly.
- `select_move()`: picks highest-N edge at root (AlphaZero convention — most-visited, not highest-Q); if mid-chain, descends and repeats to build full Move chain; stops when mover changes or child unexpanded.

**Test result** (999 sims from start position, untrained/random-weight net): 7 root edges, N spread evenly (~136-155 each), P all ~0.14 (near-uniform, expected from random policy head), Q all near-zero both signs (expected from random value head). This is correct behavior for an untrained net — no bug. Concentration of visits will only emerge once the net has real signal (trained) or via forced-value testing.

Not yet done: NN evaluator uses `chk::net&` directly (no interface/abstraction — user explicitly rejected an `IEvaluator` abstraction layer).

## Collaboration style

- Yuriy writes the code himself from specs/discussion; I explain design, review/correct his snippets, don't hand him full implementations unprompted.
- Currently: give short answers by default; elaborate only when asked.
- Work proceeds incrementally, one component at a time, with explicit "ready to continue" checkpoints.

## Open decisions / not yet started

1. **Tree reuse across moves** — after `select_move()`, reuse chosen edge's subtree as new root instead of rebuilding from scratch.
2. **Self-play game loop** — loop N simulations → select_move → apply to real game → repeat until terminal.
3. **Training data collection** — record `(encoded_board, visit_distribution, eventual_outcome)` per move for training.
4. **Dirichlet/exploration noise** at root for self-play (AlphaZero adds noise to root priors to ensure exploration diversity).
5. **c_puct = 1.5** is a placeholder, not tuned.
6. Dedupe the two identical `mask_and_softmax` implementations (one in `game::Checkers`, one reimplemented in `MCTS`).

*(Last updated: end of thread covering MCTS run_simulation/select_edge/expand/select_move implementation + testing.)*
