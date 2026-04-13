# Chaos-Poker
Hi, I'm Ananthram Vijayaraj from IIT Madras and this is my submission for the chaos poker bot for the jump challenge
Compiled main.cpp by running
```g++ main.cpp -o ./bots/main```
then run as usual by passing to the chaos poker function

## Strategy

The bot uses an evaluate7 function, that takes 7 cards as input and returns a single score in which the top 4 bits encode the hand category (0 = high card,8 = straight flush) and the lower bits encode tiebreakers.

In order to estimate equity, the bot uses fast approximations instead of a full Monte Carlo simulation, the strategies across rounds are as follows
Pre flop: Uses a formula based on the Chen scoring system, it looks at the ranks, whether the cards are suited, and the gap between them to produce a baseline win probability against one opponent and then compounds it for multiple opponents.
Post flop: Maps the current hand category to a hardcoded win probability table,then compounds for multiple opponents. The table ranges from 0.17 for high card to 0.98 for straight flush.

### Betting strategy

The bot computes the ratio of the current equity and the fair equity and decides how much to raise/whether to call based on this ratio as well as considering the potOdds, there are multiple tiers of bets which are essentially just percentages of the current pot, these tiers are based on which street we are currently in

### Swapping strategy

The bot swaps extenstively in the preflop stage, due to the low price of swaps as well as the fact that the state of the community cards are random regardless of future swaps/votes.
In the flop stage it abstains from swapping, due to the fact that the commuinty cards could change wildly in the redraw.
During the last two stages it uses a safer swap system where it swaps only if the expected increase in equity is atleast 5% more than the current equity, or if we are at a proper disadvantage, in which case we take the risk

### Voting strategy
The direction of the vote (YES vs NO) is proportional to how much the equity beats/trails the fair equity determined

### Tradeoffs
- The fair equity function cannot clearly determine the stack sizes of the opponents as the change in chips during the swapping and voting changes are kept private.
- The bot plays very passively on multitables due to the scaling of the equity function.
- The betting does not consider the stack sizes of the opponents (due to the fact that they are not reliable) but it is something that you would consider in a general poker scenario.
- The wager used in the voting is also very passive, large wagers are not placed.
