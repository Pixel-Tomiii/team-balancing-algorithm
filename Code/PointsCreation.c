#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Constants.
#define MAX_TEAM_SIZE 8
#define MAX_PLAYERS (MAX_TEAM_SIZE * 2)
#define MAX_SUBSETS 65536
#define MAX_PLAYERS_IN_SUM 4

// Structs.
typedef struct player Player; // Stores player data.
typedef struct team Team; // Stores team data.
typedef struct controller Controller; // Tracks all the players and the teams.
typedef struct sum Sum; // Stores a sum of a subset and stores which values add to that sum.

// Globals.
Controller controller;

// Tracks the player's who's elo adds to the sum value.
struct sum {
    int sum;
    Player *players[MAX_PLAYERS_IN_SUM];
    int numPlayers;
};


// The player struct stores their id, elo and the team which they are on.
// It also stores their score, kills and deaths (and kd).
struct player {
    int id; // Stores the player id.
    int elo; // Stores the player's elo.

    int kills; // Stores the player's amount of kills.
    int deaths; // Stores the player's amount of deaths.

    Team *team; // Stores which team the player is on.
};



// The team struct stores the players on the team, the expected percentage of a win
// And the total and average Elo across the whole team.
struct team {
    char *id; // Stores the name of the team.
    Player *players[MAX_TEAM_SIZE]; // Stores the players in the team.
    int playerCount; // Stores how many players are in the team.

    int totalElo; // Stores the total elo of the team.
    int expected; // Stores the win chance of the team (%).

    Sum subsets[MAX_SUBSETS]; // Stores the subsets of the team. Will be used with team balancing.
    int totalSums; // Stores how many sums have been calculated.
};



// The controller struct tracks the player's in the and also stores the 2 teams.
struct controller {
    Team red; // Stores the red team.
    Team blue; // Stores the blue team.

    Player players[MAX_PLAYERS]; // Stores all the players.
    int playerCount; // Stores how many players are in the array.

    int totalElo; // The sum of elo of every player.
};



// Resets everything in the controller ready for a new game.
void reset() {
    memset(&controller, 0, sizeof(Controller));
}


// Adds a player to the given team.
void addPlayer(Team *team, Player *player) {
    if (MAX_TEAM_SIZE > team->playerCount) {
        team->players[team->playerCount++] = player;
        team->totalElo += player->elo;
        player->team = team;
    }

    else {
        printf("ERROR: Team %s is full!", team->id);
    }
}


// Removes a player from a given team. If the player is at the end of the array
// of which values are not null it just updates the playerCount. If it is in the
// middle it moves the last element to the empty slot.
void removePlayer(Team *team, int id) {
    // Looping through the players (linear search).
    for (int i = 0; i < team->playerCount; i++) {

        if (team->players[i]->id == id) {

            // Update the team elo.
            team->totalElo -= team->players[i]->elo;

            // The the player is in the middle/start of the array, replace it
            // with the last player in the array.
            if (i + 1 != team->playerCount) {
                team->players[i] = team->players[team->playerCount-- - 1];
                team->players[team->playerCount] = 0;
                break;
            }

            // Else remove the last player.
            else {
                team->players[--team->playerCount] = 0;
            }
        }
    }
}


// Calculates the number of ones in the binary representation of the given number.
int numberOfOnes(int num, int numberOfBits) {

    int numberOfSetBits = 0;
    for (int i = 0; i <= numberOfBits; i++) {
        if ((num >> i) & 1) {
            numberOfSetBits++;
        }
    }

    return numberOfSetBits;
}


// Fetches all the subsets and their sums of elo for the given team.
void getSubsets(Team *team) {

    int bitsForNumber;
    int setBits;

    for (int number = 1; number < (1 << team->playerCount); number++) {

        Sum sum = {0};

        bitsForNumber = (int) ceil(log2(number));
        setBits = numberOfOnes(number, bitsForNumber);

        // 4 = maximum number of players to stop (the average is less than 3 swapped players).
        if (setBits <= 4 && setBits > 0) {

            // Looping through all the bits.
            for (int bit = 0; bit <= bitsForNumber; bit++) {
                // Only adding the player's elo if the current bit is set.
                if ((number >> bit) & 1) {
                    Player *player = team->players[team->playerCount - bit - 1];
                    sum.sum += player->elo;
                    sum.players[sum.numPlayers++] = player;
                }
            }

            team->subsets[team->totalSums++] = sum;
        }
    }
}


// Method which sets up the players
// Creates a new player and gives them a random elo score (1000 - 3000).
// Adds the same number of players as the max players allowed.
void setupPlayers() {

    #define MAX_ELO 3000
    #define MIN_ELO 1000

    controller.playerCount = MAX_PLAYERS;

    for (int i = 0; i < controller.playerCount; i ++) {
        Player p = {i, (rand() % (MAX_ELO - MIN_ELO + 1)) + MIN_ELO};
        controller.totalElo += p.elo;
        controller.players[i] = p;
    }
}


// Method which adds the players to the teams.
void setupTeams() {
    int switcher = 1; // Alternates which team is added to.

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (switcher) {
            addPlayer(&controller.red, &controller.players[i]);
        }

        else {
            addPlayer(&controller.blue, &controller.players[i]);
        }

        switcher = !switcher;
    }

    // Uses the expected win formula for Elo.
    controller.red.expected = round(100 * (1 / (1 + pow(10, (controller.blue.totalElo - controller.red.totalElo) / 400.0))));
    controller.blue.expected = 100 - controller.red.expected;
}


// Outputs the players and their elos and the total elo for the given team.
void outputTeam(Team *team) {
    printf("====================\nTeam: %s->Expected = %d%%\n", team->id, team->expected);
    for (int i = 0; i < team->playerCount; i++) {
        printf("Player %d: %d\n", team->players[i]->id, team->players[i]->elo);
    }

    printf("\nTotal Elo: %d\n\n", team->totalElo);
}




// Swaps the players around (team by team basis).
void swapPlayers(Sum *removeFromTeam1, Sum *removeFromTeam2, Team *team1, Team *team2) {

    // Fetches the number of players to swap (total is equal for but 'Sums').
    int numPlayersToSwap = removeFromTeam1->numPlayers;

    // Swapping the players.
    for (int i = 0; i < numPlayersToSwap; i++) {

        // Removing BEFORE adding.
        removePlayer(team1, removeFromTeam1->players[i]->id);
        removePlayer(team2, removeFromTeam2->players[i]->id);

        addPlayer(team1, removeFromTeam2->players[i]);
        addPlayer(team2, removeFromTeam1->players[i]);
    }
}

// Balances the teams.
void balanceTeams() {

    Team *team1;
    Team *team2;

    // Finds which team has the most total elo and sets that team as team 1.
    if (controller.red.totalElo > controller.blue.totalElo) {
        team1 = &controller.red;
        team2 = &controller.blue;
    }

    else {
        team1 = &controller.blue;
        team2 = &controller.red;
    }


    Sum team1Swap;
    Sum team2Swap;

    int difference = (team1->totalElo - team2->totalElo) / 2;
    int smallest = 0;

    // Looping through the subsets for both teams to determine which players to swap to balance the teams.
    for (int team1Subset = 0; team1Subset < team1->totalSums; team1Subset ++) {
        for (int team2Subset = 0; team2Subset < team2->totalSums; team2Subset++) {

            Sum *team1CurrentSubset = &(team1->subsets[team1Subset]);
            Sum *team2CurrentSubset = &(team2->subsets[team2Subset]);

            // Only comparing if the number of players in each subset is equal.
            if (team1CurrentSubset->numPlayers == team2CurrentSubset->numPlayers) {
                int diff = (team1CurrentSubset->sum) - (team2CurrentSubset->sum);

                // If the 'diff', is closer to the difference than the previous closest.
                if (diff <= difference && diff > smallest) {
                    smallest = diff;
                    team1Swap = *team1CurrentSubset;
                    team2Swap = *team2CurrentSubset;
                }
            }
        }
    }

    swapPlayers(&team1Swap, &team2Swap, team1, team2);

    // Uses the expected win formula for Elo.
    team1->expected = (int) round(100 * (1 / (1 + pow(10, (team2->totalElo - team1->totalElo) / 400.0))));
    team2->expected = 100 - team1->expected;

}



// Starts a new game every loop.
// Steps:
//  - Adds the players to the teams, trying to keep them as balanced as possible.
//  - Simulates a game.
//  - Updates player stats.
//  - Resets the controller.

void run() {

    setupTeams();

    getSubsets(&controller.red);
    getSubsets(&controller.blue);

    clock_t start = clock();
    balanceTeams();
    clock_t end = clock();

    double timeTaken = (double) 1000.0 * (end - start) / (double) CLOCKS_PER_SEC;

    outputTeam(&controller.red);
    outputTeam(&controller.blue);

    printf("\n\nTime taken: %fms", timeTaken);
}


int main() {

    setupPlayers();

    controller.red.id = "red";
    controller.blue.id = "blue";

    run();

}
