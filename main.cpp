#include <cstdio>
#include <cstring>
using namespace std;

static inline int card_rank(int card){return card >> 2;}
static inline int card_suit(int card){return card & 3;}
static inline int make_card(int rank, int suit){return (rank << 2) | suit;}

static int parse_card(const char* s) {
    int rank;
    switch (s[0]) {
        case '2': rank = 0;  break; case '3': rank = 1;  break;
        case '4': rank = 2;  break; case '5': rank = 3;  break;
        case '6': rank = 4;  break; case '7': rank = 5;  break;
        case '8': rank = 6;  break; case '9': rank = 7;  break;
        case 'T': rank = 8;  break; case 'J': rank = 9;  break;
        case 'Q': rank = 10; break; case 'K': rank = 11; break;
        default:  rank = 12; break;
    }
    int suit;
    switch (s[1]) {
        case 'c': suit = 0; break; case 'd': suit = 1; break;
        case 'h': suit = 2; break; default:  suit = 3; break;
    }
    return make_card(rank, suit);
}

static int eval7(const int* cards) {
    int rank_freq[13] = {};
    int suit_count[4] = {};
    int ranks[7], suits[7];

    for (int i = 0; i < 7; i++) {
        ranks[i] = card_rank(cards[i]);
        suits[i] = card_suit(cards[i]);
        rank_freq[ranks[i]]++;
        suit_count[suits[i]]++;
    }

    int flush_suit = -1;
    for (int s = 0; s < 4; s++) {
        if (suit_count[s] >= 5) { flush_suit = s; break; }
    }

    int flush_ranks[7], flush_count = 0;
    if (flush_suit >= 0) {
        for (int i = 0; i < 7; i++) {
            if (suits[i] == flush_suit) flush_ranks[flush_count++] = ranks[i];
        }
    }

    auto find_straight = [](int* arr, int n) -> int {
        for (int i = 0; i < n - 1; i++)
            for (int j = i + 1; j < n; j++)
                if (arr[j] > arr[i]) { int t = arr[i]; arr[i] = arr[j]; arr[j] = t; }
        int unique[14], unique_count = 0;
        for (int i = 0; i < n; i++)
            if (!unique_count || unique[unique_count - 1] != arr[i])
                unique[unique_count++] = arr[i];
        if (unique_count && unique[0] == 12) unique[unique_count++] = -1;
        for (int i = 0; i + 4 < unique_count; i++)
            if (unique[i] - unique[i + 4] == 4) return unique[i];
        return -1;
    };

    if (flush_suit >= 0) {
        int flush_copy[7];
        memcpy(flush_copy, flush_ranks, flush_count * 4);
        int straight_top = find_straight(flush_copy, flush_count);
        if (straight_top >= 0) return (8 << 28) | straight_top;
    }

    int group_rank[13], group_freq[13], group_count = 0;
    for (int r = 12; r >= 0; r--) {
        if (rank_freq[r]) { group_freq[group_count] = rank_freq[r]; group_rank[group_count] = r; group_count++; }
    }
    for (int i = 0; i < group_count - 1; i++)
        for (int j = i + 1; j < group_count; j++)
            if (group_freq[j] > group_freq[i] || (group_freq[j] == group_freq[i] && group_rank[j] > group_rank[i])) {
                int t = group_freq[i]; group_freq[i] = group_freq[j]; group_freq[j] = t;
                t = group_rank[i]; group_rank[i] = group_rank[j]; group_rank[j] = t;
            }

    int top_freq  = group_freq[0], top_rank  = group_rank[0];
    int sec_freq  = group_count > 1 ? group_freq[1] : 0;
    int sec_rank  = group_count > 1 ? group_rank[1] : 0;

    if (top_freq == 4) {
        int kicker = -1;
        for (int i = 0; i < group_count; i++) if (group_rank[i] != top_rank) { kicker = group_rank[i]; break; }
        return (7 << 28) | (top_rank << 4) | (kicker & 0xF);
    }
    if (top_freq == 3 && sec_freq >= 2) return (6 << 28) | (top_rank << 4) | sec_rank;
    if (flush_suit >= 0) {
        int score = 5 << 28;
        for (int i = 0; i < 5; i++) score |= flush_ranks[i] << (16 - 4 * i);
        return score;
    }
    {
        int ranks_copy[7];
        memcpy(ranks_copy, ranks, 28);
        int straight_top = find_straight(ranks_copy, 7);
        if (straight_top >= 0) return (4 << 28) | straight_top;
    }
    if (top_freq == 3) {
        int score = (3 << 28) | (top_rank << 8);
        int filled = 0;
        for (int i = 0; i < group_count; i++) {
            if (group_rank[i] == top_rank) continue;
            score |= group_rank[i] << (4 - 4 * filled);
            if (++filled == 2) break;
        }
        return score;
    }
    if (top_freq == 2 && sec_freq == 2) {
        int hi = top_rank > sec_rank ? top_rank : sec_rank;
        int lo = top_rank < sec_rank ? top_rank : sec_rank;
        int kicker = -1;
        for (int i = 0; i < group_count; i++)
            if (group_rank[i] != top_rank && group_rank[i] != sec_rank) { kicker = group_rank[i]; break; }
        return (2 << 28) | (hi << 8) | (lo << 4) | (kicker & 0xF);
    }
    if (top_freq == 2) {
        int score = (1 << 28) | (top_rank << 12);
        int filled = 0;
        for (int i = 0; i < group_count; i++) {
            if (group_rank[i] == top_rank) continue;
            score |= group_rank[i] << (8 - 4 * filled);
            if (++filled == 3) break;
        }
        return score;
    }
    {
        int score = 0;
        for (int i = 0; i < 5; i++) score |= group_rank[i] << (16 - 4 * i);
        return score;
    }
}

static const float CATEGORY_EQUITY_VS_ONE[9] = { 0.17f, 0.44f, 0.63f, 0.72f, 0.78f, 0.83f, 0.90f, 0.95f, 0.98f };

static float category_equity(int category, int num_opponents) {
    float per_opp = CATEGORY_EQUITY_VS_ONE[category];
    float equity = per_opp;
    for (int i = 1; i < num_opponents; i++) equity *= per_opp;
    return equity;
}

static float preflop_equity(int hole_0, int hole_1, int num_opponents) {
    int rank_hi = card_rank(hole_0), rank_lo = card_rank(hole_1);
    if (rank_hi < rank_lo) { int t = rank_hi; rank_hi = rank_lo; rank_lo = t; }
    int suited = (card_suit(hole_0) == card_suit(hole_1)) ? 1 : 0;

    float base;
    if (rank_hi == rank_lo) {
        base = 0.50f + (rank_hi / 12.0f) * 0.35f;
    } else {
        float hi_contrib  = 0.30f + (rank_hi / 12.0f) * 0.30f;
        float lo_contrib  = (rank_lo / 12.0f) * 0.10f;
        float gap_penalty = (rank_hi - rank_lo - 1) * 0.02f;
        float suit_bonus  = suited ? 0.03f : 0.0f;
        base = hi_contrib + lo_contrib - gap_penalty + suit_bonus;
        if (base < 0.28f) base = 0.28f;
        if (base > 0.72f) base = 0.72f;
    }

    float equity = base;
    for (int i = 1; i < num_opponents; i++) equity *= base;
    return equity;
}

struct GameState {
    int  my_seat, num_players, my_chips, pot, small_blind;
    int  hole[2];
    int  community[5];
    int  num_community;
    int  chip_count[9];
    int  bet_this_round[9];
    bool folded[9], card_dead[52];
    int  swap_cost_mult[4];
    int  players_alive, players_in_hand;
    int  swap_count, last_swap_idx;
    int  chips_at_hand_start;
    int  pot_at_street_start;
    bool preflop_strong;
} G;

static void reset_hand() {
    G.num_community      = 0;
    G.pot                = 0;
    G.swap_count         = 0;
    G.last_swap_idx      = -1;
    G.players_in_hand    = G.players_alive;
    G.chips_at_hand_start = G.my_chips;
    G.pot_at_street_start = 0;
    G.preflop_strong     = false;
    memset(G.card_dead,      0, sizeof(G.card_dead));
    memset(G.folded,         0, sizeof(G.folded));
    memset(G.bet_this_round, 0, sizeof(G.bet_this_round));
}

static int num_opponents() {
    return G.players_in_hand > 1 ? G.players_in_hand - 1 : 1;
}

static int hand_score_7() {
    int cards[7] = { G.hole[0], G.hole[1],
                     G.community[0], G.community[1], G.community[2],
                     G.community[3], G.community[4] };
    return eval7(cards);
}

static int hand_score_6() {
    int cards[7] = { G.hole[0], G.hole[1],
                     G.community[0], G.community[1], G.community[2],
                     G.community[3], G.community[3] };
    return eval7(cards);
}

static float my_equity() {
    if (G.num_community < 3) {
        return preflop_equity(G.hole[0], G.hole[1], num_opponents());
    }
    if (G.num_community == 3) {
        int cards[7] = { G.hole[0], G.hole[1],
                         G.community[0], G.community[1], G.community[2],
                         G.hole[0], G.hole[1] };
        return category_equity(eval7(cards) >> 28, num_opponents());
    }
    if (G.num_community == 4) return category_equity(hand_score_6() >> 28, num_opponents());
    return category_equity(hand_score_7() >> 28, num_opponents());
}

static float fair_equity(int pot_size) {
    if (pot_size <= 0) return 1.0f / G.players_alive;
    int chips_committed = G.chips_at_hand_start - G.my_chips;
    if (chips_committed < 0) chips_committed = 0;
    float fe = (float)chips_committed / (float)pot_size;
    float min_fe = 1.0f / G.players_alive;
    if (fe < min_fe) fe = min_fe;
    if (fe > 0.95f)  fe = 0.95f;
    return fe;
}

static float vote_fair_equity() {
    int estimated_pot = G.pot_at_street_start;
    if (estimated_pot < G.small_blind * 2) estimated_pot = G.small_blind * 2;
    return fair_equity(estimated_pot);
}

static void do_bet(int my_chips, int current_bet, int my_bet, int min_raise, int pot_size) {
    int   to_call   = current_bet - my_bet;
    float equity    = my_equity();
    float fair_eq   = fair_equity(pot_size);
    float pot_odds  = pot_size > 0 ? (float)to_call / (pot_size + to_call) : 0.0f;
    float strength  = fair_eq > 0 ? equity / fair_eq : 1.0f;

    float tier_1, tier_2, tier_3, max_fraction;
    switch (G.num_community) {
        case 0:  tier_1 = 0.60f; tier_2 = 0.40f; tier_3 = 0.25f; max_fraction = 0.75f; break;
        case 3:  tier_1 = 0.75f; tier_2 = 0.50f; tier_3 = 0.33f; max_fraction = 1.00f; break;
        case 4:  tier_1 = 0.90f; tier_2 = 0.67f; tier_3 = 0.45f; max_fraction = 1.25f; break;
        default: tier_1 = 1.10f; tier_2 = 0.85f; tier_3 = 0.55f; max_fraction = 1.50f; break;
    }

    auto make_raise_amount = [&](float fraction) -> int {
        int amount     = (int)(pot_size * fraction);
        int cap_pot    = (int)(pot_size * max_fraction);
        int cap_stack  = (int)(my_chips * 0.60f);
        if (amount > cap_pot)   amount = cap_pot;
        if (amount > cap_stack) amount = cap_stack;
        if (amount < min_raise) amount = min_raise;
        if (amount > my_chips + my_bet) amount = my_chips + my_bet;
        return amount;
    };

    auto try_raise = [&](float fraction) -> bool {
        int amount = make_raise_amount(fraction);
        if (amount >= my_chips) return false;
        if (amount < min_raise) return false;
        printf("RAISE %d\n", amount);
        return true;
    };

    if (to_call <= 0) {
        if      (strength >= 1.4f)  { if (!try_raise(tier_1)) printf("CHECK\n"); }
        else if (strength >= 1.2f)  { if (!try_raise(tier_2)) printf("CHECK\n"); }
        else if (strength >= 1.05f) { if (!try_raise(tier_3)) printf("CHECK\n"); }
        else                          printf("CHECK\n");
    } else {
        if (equity <= pot_odds + 0.04f) { printf("FOLD\n"); return; }
        if      (strength >= 1.4f) { if (!try_raise(tier_1)) printf("CALL\n"); }
        else if (strength >= 1.2f) { if (!try_raise(tier_2)) printf("CALL\n"); }
        else {
            if (to_call <= my_chips) printf("CALL\n");
            else                     printf("FOLD\n");
        }
    }
}

static float chen_score(int card_0, int card_1) {
    int rank_hi = card_rank(card_0), rank_lo = card_rank(card_1);
    if (rank_hi < rank_lo) { int t = rank_hi; rank_hi = rank_lo; rank_lo = t; }

    float score;
    if      (rank_hi == 12) score = 10.0f;
    else if (rank_hi == 11) score = 8.0f;
    else if (rank_hi == 10) score = 7.0f;
    else if (rank_hi == 9)  score = 6.0f;
    else                    score = (rank_hi + 2) / 2.0f;

    if (rank_hi == rank_lo) {
        score = score * 2;
        if (score < 5) score = 5;
        return score;
    }

    if (card_suit(card_0) == card_suit(card_1)) score += 2;

    int gap = rank_hi - rank_lo - 1;
    if      (gap == 0) score += 1;
    else if (gap == 1) score -= 1;
    else if (gap == 2) score -= 2;
    else if (gap == 3) score -= 4;
    else               score -= 5;

    if (rank_hi < 4 && gap <= 1) score -= 1;
    return score;
}

static void do_swap() {
    if (G.num_community == 3) { printf("STAY\n"); return; }

    if (G.num_community == 0 && (G.preflop_strong || G.swap_count >= 1)) {
        printf("STAY\n"); return;
    }

    if (G.swap_count >= 2) { printf("STAY\n"); return; }

    int swap_cost;
    switch (G.num_community) {
        case 0:  swap_cost = G.swap_cost_mult[0] * G.small_blind; break;
        case 4:  swap_cost = G.swap_cost_mult[2] * G.small_blind; break;
        default: swap_cost = G.swap_cost_mult[3] * G.small_blind; break;
    }
    if (swap_cost > G.my_chips) { printf("STAY\n"); return; }

    float current_equity = my_equity();
    float fair_eq        = fair_equity(G.pot);
    if (current_equity > fair_eq * 1.12f) { printf("STAY\n"); return; }

    auto equity_without_card = [&](int card_idx) -> float {
        int other = G.hole[1 - card_idx];
        int cards[7];
        cards[0] = other; cards[1] = other;
        for (int i = 0; i < G.num_community; i++) cards[2 + i] = G.community[i];
        for (int i = G.num_community; i < 5; i++) cards[2 + i] = other;
        return category_equity(eval7(cards) >> 28, num_opponents()) * 0.85f;
    };

    float equity_drop_0 = equity_without_card(0);
    float equity_drop_1 = equity_without_card(1);
    int   swap_idx      = (equity_drop_0 < equity_drop_1) ? 0 : 1;
    float best_kept_eq  = (equity_drop_0 > equity_drop_1) ? equity_drop_0 : equity_drop_1;

    if (best_kept_eq > current_equity * 1.05f || current_equity < fair_eq * 0.85f) {
        G.card_dead[G.hole[swap_idx]] = true;
        G.swap_count++;
        G.last_swap_idx = swap_idx;
        printf("SWAP %d\n", swap_idx);
    } else {
        printf("STAY\n");
    }
}

static void do_vote(int my_chips) {
    float equity  = my_equity();
    float fair_eq = vote_fair_equity();

    if (equity >= fair_eq) {
        int wager = (int)(my_chips * (equity - fair_eq) * 0.05f);
        if (wager < 0) wager = 0;
        printf("VOTE YES %d\n", wager);
    } else {
        int wager = (int)(my_chips * (fair_eq - equity) * 0.05f);
        if (wager < 0) wager = 0;
        printf("VOTE NO %d\n", wager);
    }
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin,  NULL, _IONBF, 0);

    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        char cmd[32];
        if (sscanf(line, "%31s", cmd) != 1) continue;

        if (!strcmp(cmd, "GAME_START")) {
            int num_players, seat, starting_chips, pm, fm, tm, rm;
            sscanf(line, "%*s %d %d %d %d %d %d %d",
                   &num_players, &seat, &starting_chips, &pm, &fm, &tm, &rm);
            G.my_seat       = seat;
            G.num_players   = num_players;
            G.my_chips      = starting_chips;
            G.players_alive = num_players;
            G.small_blind   = 1;
            G.swap_cost_mult[0] = pm; G.swap_cost_mult[1] = fm;
            G.swap_cost_mult[2] = tm; G.swap_cost_mult[3] = rm;
            for (int i = 0; i < num_players; i++) G.chip_count[i] = starting_chips;
            memset(G.folded, 0, sizeof(G.folded));

        } else if (!strcmp(cmd, "HAND_START")) {
            int hand_num, dealer_seat, sb_seat, bb_seat, sb_amount, bb_amount;
            sscanf(line, "%*s %d %d %d %d %d %d",
                   &hand_num, &dealer_seat, &sb_seat, &bb_seat, &sb_amount, &bb_amount);
            G.small_blind = sb_amount;
            reset_hand();
            if (sb_seat >= 0 && sb_seat < G.num_players) {
                int amt = sb_amount < G.chip_count[sb_seat] ? sb_amount : G.chip_count[sb_seat];
                G.chip_count[sb_seat]    -= amt;
                G.bet_this_round[sb_seat] = amt;
                G.pot                    += amt;
            }
            if (bb_seat >= 0 && bb_seat < G.num_players) {
                int amt = sb_amount * 2;
                if (amt > G.chip_count[bb_seat]) amt = G.chip_count[bb_seat];
                G.chip_count[bb_seat]    -= amt;
                G.bet_this_round[bb_seat] = amt;
                G.pot                    += amt;
            }
            if (G.my_seat == sb_seat) G.my_chips = G.chip_count[G.my_seat];
            if (G.my_seat == bb_seat) G.my_chips = G.chip_count[G.my_seat];
            G.chips_at_hand_start  = G.my_chips;
            G.pot_at_street_start  = G.pot;

        } else if (!strcmp(cmd, "CHIPS")) {
            int players_alive = 0;
            char* ptr = line + 5;
            for (int i = 0; i < G.num_players; i++) {
                while (*ptr == ' ') ptr++;
                int val = 0;
                while (*ptr >= '0' && *ptr <= '9') val = val * 10 + (*ptr++) - '0';
                G.chip_count[i] = val;
                if (val > 0) players_alive++;
                if (i == G.my_seat) { G.my_chips = val; G.chips_at_hand_start = val; }
            }
            G.players_alive   = players_alive;
            G.players_in_hand = players_alive;

        } else if (!strcmp(cmd, "DEAL_HOLE")) {
            char c1[4], c2[4];
            sscanf(line, "%*s %3s %3s", c1, c2);
            G.hole[0] = parse_card(c1);
            G.hole[1] = parse_card(c2);
            G.preflop_strong = (chen_score(G.hole[0], G.hole[1]) >= 7.0f);

        } else if (!strcmp(cmd, "DEAL_FLOP")) {
            char c1[4], c2[4], c3[4];
            sscanf(line, "%*s %3s %3s %3s", c1, c2, c3);
            G.community[0] = parse_card(c1);
            G.community[1] = parse_card(c2);
            G.community[2] = parse_card(c3);
            G.num_community = 3;
            G.swap_count    = 0;
            G.pot_at_street_start = G.pot;
            memset(G.bet_this_round, 0, sizeof(G.bet_this_round));

        } else if (!strcmp(cmd, "DEAL_TURN")) {
            char c1[4]; sscanf(line, "%*s %3s", c1);
            G.community[G.num_community++] = parse_card(c1);
            G.swap_count = 0;
            G.pot_at_street_start = G.pot;
            memset(G.bet_this_round, 0, sizeof(G.bet_this_round));

        } else if (!strcmp(cmd, "DEAL_RIVER")) {
            char c1[4]; sscanf(line, "%*s %3s", c1);
            G.community[G.num_community++] = parse_card(c1);
            G.swap_count = 0;
            G.pot_at_street_start = G.pot;
            memset(G.bet_this_round, 0, sizeof(G.bet_this_round));

        } else if (!strcmp(cmd, "REDRAW_FLOP")) {
            char c1[4], c2[4], c3[4];
            sscanf(line, "%*s %3s %3s %3s", c1, c2, c3);
            G.community[0] = parse_card(c1);
            G.community[1] = parse_card(c2);
            G.community[2] = parse_card(c3);
            G.num_community = 3;

        } else if (!strcmp(cmd, "REDRAW_TURN")) {
            char c1[4]; sscanf(line, "%*s %3s", c1);
            G.community[G.num_community - 1] = parse_card(c1);

        } else if (!strcmp(cmd, "REDRAW_RIVER")) {
            char c1[4]; sscanf(line, "%*s %3s", c1);
            G.community[G.num_community - 1] = parse_card(c1);

        } else if (!strcmp(cmd, "SWAP_RESULT")) {
            char c1[4]; sscanf(line, "%*s %3s", c1);
            if (G.last_swap_idx >= 0) G.hole[G.last_swap_idx] = parse_card(c1);

        } else if (!strcmp(cmd, "ACTION")) {
            int acting_seat; char action[16];
            sscanf(line, "%*s %d %15s", &acting_seat, action);
            if (acting_seat < 0 || acting_seat >= G.num_players) continue;

            if (!strcmp(action, "FOLD")) {
                if (acting_seat != G.my_seat && !G.folded[acting_seat]) {
                    G.folded[acting_seat] = true;
                    G.players_in_hand--;
                }
            } else if (!strcmp(action, "CALL") || !strcmp(action, "RAISE") || !strcmp(action, "ALLIN")) {
                int total_bet = 0;
                sscanf(line, "%*s %*d %*s %d", &total_bet);
                int added = total_bet - G.bet_this_round[acting_seat];
                if (added < 0) added = 0;
                G.chip_count[acting_seat] -= added;
                if (G.chip_count[acting_seat] < 0) G.chip_count[acting_seat] = 0;
                G.bet_this_round[acting_seat] = total_bet;
                G.pot += added;
                if (acting_seat == G.my_seat) G.my_chips = G.chip_count[acting_seat];
            }

        } else if (!strcmp(cmd, "VOTE_RESULT")) {
            int yes_total, no_total;
            sscanf(line, "%*s %d %d", &yes_total, &no_total);
            G.pot += yes_total + no_total;

        } else if (!strcmp(cmd, "SWAP_PROMPT")) {
            int swap_cost, chips_now;
            sscanf(line, "%*s %d %d", &swap_cost, &chips_now);
            G.my_chips = chips_now;
            G.chip_count[G.my_seat] = chips_now;
            do_swap();

        } else if (!strcmp(cmd, "VOTE_PROMPT")) {
            int chips_now; sscanf(line, "%*s %d", &chips_now);
            G.my_chips = chips_now;
            G.chip_count[G.my_seat] = chips_now;
            do_vote(chips_now);

        } else if (!strcmp(cmd, "ACTION_PROMPT")) {
            int chips_now, current_bet, my_bet, min_raise, pot_size;
            sscanf(line, "%*s %d %d %d %d %d",
                   &chips_now, &current_bet, &my_bet, &min_raise, &pot_size);
            G.my_chips = chips_now;
            G.chip_count[G.my_seat] = chips_now;
            G.pot = pot_size;
            do_bet(chips_now, current_bet, my_bet, min_raise, pot_size);
        }
    }
}
