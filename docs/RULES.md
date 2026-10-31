# GoGame Rules

These rules describe the current implementation. GoGame uses territory and captured stones for scoring, with simplifications to life-and-death adjudication and game completion. It does not implement the complete Japanese Rules of Go.

Online and AI matches use the board rules below. Sandbox uses the same move validation and scoring while allowing one user to control both colors and edit the game through Reset, Undo, and Redo.

## Article 1. Objective

The objective is to finish with more points than the opponent. Points come from empty territory and captured opposing stones. White also receives komi as described in Article 10. A match can also be won by resignation, an online time forfeit, or an opponent's online disconnection.

## Article 2. Turns

Black moves first. Black and White then alternate turns. On a turn, the player may place one stone or pass. A player may resign from an online or AI match, including outside their own turn.

## Article 3. Board and Placement

The board is a grid of 19 horizontal and 19 vertical lines forming 361 intersections. A stone can be played on any unoccupied intersection (called an "empty point") on which Article 4 permits it to exist. The point on which a stone is played is called its "point of play."

## Article 4. Groups, Liberties, and Suicide

After a move is completed, a group of one or more stones belonging to one player exists on its points of play on the board as long as it has a horizontally or vertically adjacent empty point, called a "liberty." No group of stones without a liberty can exist on the board.

## Article 5. Capture

After a stone is placed, adjacent opposing groups whose last liberty was filled are removed. Each removed stone becomes a prisoner credited to the capturing player. Multiple groups may be captured in one move.

## Article 6. Ko

A shape in which the players can alternately capture and recapture one opposing stone is called a "ko." A player whose stone has been captured in a ko cannot recapture in that ko on the next move.

## Article 7. Life and Death in This Implementation

There is no separate life-and-death adjudication phase. Stones still on the board when the game ends are not automatically removed as dead stones and are not added to either player's prisoners.

Players must make any necessary captures during play before both sides pass. The program does not provide an agreement screen for marking dead groups.

## Article 8. Territory

Each connected region of empty intersections:

- If its bordering stones are all Black, every intersection in the region counts as Black territory.
- If its bordering stones are all White, every intersection in the region counts as White territory.
- If it borders both colors, or no stones, it gives neither player territory points.

Occupied intersections do not give territory points.

## Article 9. Passing and Ending by Score

When a player passes his move and his opponent passes in succession, the game stops.

## Article 10. Determining the Score

The final scores are:

```text
Black score = Black territory + White stones captured by Black
White score = White territory + Black stones captured by White + 6.5 komi
```

The player with the larger score wins. Because territory and prisoner counts are integers and komi is fixed at 6.5, a scored game cannot end in an equal score under the current settings.

Points displayed during play are provisional estimates of secured territory and may differ from the final territory calculation. Captures, an unfinished position, and the life-and-death/seki limitations above can affect the result.

## Article 11. Resignation

Resigning immediately ends an online or AI match as a loss for the resigning player and a win for the opponent. Territory points do not determine a resignation result.

## Article 12. No result

When the same whole-board position is repeated during a game, if the players agree(offer draw and oppenent accepted), the game ends without result(draw).

## Article 13. Both Players Lose

The application has no result in which both players lose the same match.

## Article 14. Online game

Online thinking time continues while the active player attempts an invalid move.

In an active online match:

- A player who exhausts all available time loses.
- A player who disconnects loses, and the remaining opponent wins.

## Article 15. Time control in online game

Each player starts with 10 minutes of main time by default. Main time runs only during that player's turn.

When main time reaches zero, the player enters Byo-yomi with three periods of 20 seconds each:

- Completing a legal move or pass before the current period expires preserves that period. The next turn starts with a full 20 seconds.
- Letting a period expire consumes one period. If any remain, another 20-second period begins for the same turn.
- Exhausting the final period immediately loses the match.

Completing a move ends that player's running turn and starts the opponent's clock. The server determines online clock results.

## Reference

The [Japanese Rules of Go](https://www.cs.cmu.edu/~wjh/go/tmp/rules/NihonKiin.html) are the reference for the terminology. Their full life-and-death, seki, agreement, and No Result procedures are not all implemented by GoGame.
