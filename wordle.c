#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "mylist.h"

#define WORD_LEN 5
#define MAX_GUESSES 6

char *NORMAL        = "\033[0m";
char *RED           = "\033[0;31m";
char *BRIGHT_RED    = "\033[1;31m";
char *GREEN         = "\033[0;32m";
char *BRIGHT_GREEN  = "\033[1;32m";
char *YELLOW        = "\033[0;33m";
char *BRIGHT_YELLOW = "\033[1;33m";

enum L_STATUS {
    UNGUESSED,
    UNUSED,
    WRONG,
    CORRECT
};

struct Letter {
    enum L_STATUS status;
    int locs[WORD_LEN];
    int times;
    int times_guessed;
    char letter;
};

int load_words(struct List *words, char *filename) {
    int count = 0;
    initList(words);
    FILE *fp = fopen(filename, "r");
    char buf[WORD_LEN+2];
    char *w;
    struct Node *prev = words->head;
    while (fgets(buf, sizeof(buf), fp) != 0) {
        w = (char *)malloc(WORD_LEN+1); // space for word + \0
        strncpy(w, buf, WORD_LEN);      // copy word
        w[WORD_LEN] = 0;                // terminate with \0
        prev = addAfter(words, prev, w);
        count++;
    }
    return count;
}

void free_words(struct List *words) {
    traverseList(words, &free);
    removeAllNodes(words);
}

void print_word(void *w) {
    printf("%s\n", (char *)w);
}

int main(int argc, char **argv) {
    // Load the words.
    struct List answers;
    struct List guesses;
    int n_answers = load_words(&answers, "words-answers.txt");
    load_words(&guesses, "words-guesses.txt");

    // Seed randomness.
    srand(time(NULL));

    // Initialize.
    int n_guess;
    char guess[WORD_LEN+1];
    int r;
    struct Node *guess_node;
    struct Node *secret;
    char *secret_word;
    char *color;
    int c;
    struct Letter alphabet[26];

    // Start a game.
    n_guess = 1;

    // Choose one of these words at random to be the secret.
    if (argc != 2) {
        r = rand() % n_answers;
        secret = answers.head;
        for (int i=0; i<r; i++) {
            secret = secret->next;
        }
        secret_word = (char *)secret->data;
    } else {
        secret_word = argv[1];
        secret = findNode(&answers, secret_word, (int(*)(const void*, const void*))&strcmp);
        if (secret == NULL) {
            printf("debug word not in word list\n");
            exit(1);
        }
    }

    // Set up the alphabet.
    for (char c='a'; c<='z'; c++) {
        int count = 0;
        for (int i=0; i<WORD_LEN; i++) {
            if (secret_word[i] == c) {
                alphabet[c-'a'].locs[i] = 1;
                count++;
            } else {
                alphabet[c-'a'].locs[i] = 0;
            }
        }
        alphabet[c-'a'].status = UNGUESSED;
        alphabet[c-'a'].times  = count;
        alphabet[c-'a'].times_guessed  = 0;
        alphabet[c-'a'].letter = c;
    }

    // Display the alphabet and current guess at the top.
    for (int i=0; i<26; i++) {
        printf("%c", toupper(alphabet[i].letter));
    }
    printf("\n");
    printf("(%d/%d)\n", n_guess, MAX_GUESSES);

    // Outer game loop for each guess.
    while (1) {
        // Reset the alphabet guesses.
        for (int i=0; i<26; i++) {
            alphabet[i].times_guessed = 0;
        }

        // Take user input.
        while (1) {
            if (n_guess != 1) {
                // Update alphabet.
                printf("\x1b[%dF", n_guess+1);
                printf("\x1b[2K");
                for (int i=0; i<26; i++) {
                    if (alphabet[i].status == UNGUESSED) {          // Normal
                        color = NORMAL;
                    } else if (alphabet[i].status == UNUSED) {      // Red
                        color = RED;
                    } else if (alphabet[i].status == CORRECT) {     // Green
                        color = GREEN;
                    } else if (alphabet[i].status == WRONG) {       // Yellow
                        color = YELLOW;
                    }
                    printf("%s%c", color, toupper(alphabet[i].letter));
                }
                printf("%s\n", NORMAL);
                printf("\x1b[%dE", n_guess-1+1);

                // Update guess counter.
                printf("\x1b[%dF", n_guess);
                printf("\x1b[2K");
                printf("(%d/%d)\n", n_guess, MAX_GUESSES);
                printf("\x1b[%dE", n_guess-1);
            }
            fgets(guess, sizeof(guess), stdin);
            guess[WORD_LEN] = 0;
            if (guess[strlen(guess)-1] == '\n')
                guess[strlen(guess)-1] = 0;
            for (int i=0; guess[i]; i++) {
                guess[i] = tolower(guess[i]);
            }
            // Discard excess chars.
            if (strlen(guess) == 5) {
                while ((c = fgetc(stdin)) != 0 && c != '\n')
                    ;
            }
            printf("\x1b[1F");
            printf("\x1b[2K");

            // Make sure the word is in the list.
            guess_node = findNode(&guesses, guess, (int(*)(const void *,const void *))&strcmp);
            if (guess_node == NULL) {
                continue;
            } else {
                n_guess++;
                break;
            }
        }

        // Check if it's the secret word.
        guess_node = findNode(&answers, guess, (int(*)(const void *, const void *))&strcmp);
        if (guess_node == secret) {
            printf("%s%s%s\n", BRIGHT_GREEN, (char *)secret->data, NORMAL);
            printf("You got it!\n");
            break;
        }
        // Return info about letters.
        // Come up with a better DS+algo for this search.
        // Should use the alphabet.

        /* Guess letters:
         * Green if in correct spot.
         * Red if:
         *   - not in word
         *   - all other instances accounted for by green or yellow.
         * Yellow if in word but not in correct spot
         * If there are more instances of a letter in the guess than in the
         * secret word, highlight the earlier ones yellow first, but green takes
         * priority.
         */
        else {
            // Iterate over letters in the guess.
            // First count up the times you got it in the correct place.
            // go through again and decide how to print.
            for (int i=0; i<WORD_LEN; i++) {
                struct Letter *l = alphabet + (guess[i]-'a');
                if (l->locs[i]) {
                    l->times_guessed++;
                }
            }

            for (int i=0; i<WORD_LEN; i++) {
                struct Letter *l = alphabet + (guess[i]-'a');

                // Handle green.
                if (l->locs[i]) {
                    color = BRIGHT_GREEN;
                    printf("%s%c", color, guess[i]);
                    l->status = CORRECT;
                    continue;
                }

                // Handle yellow and red.
                for (int j=0; j<WORD_LEN; j++) {
                    if (l->locs[j] ) {
                        // Letter i of guess is in secret word.
                        // If already found all the instances, print red but
                        // don't touch the alphabet.
                        // If the letter was unguessed, print yellow and update
                        // the alphabet.
                        // If the letter was guessed before but is now wrong as
                        // entered (and the previous conditions were false),
                        // print yellow and do not update the alphabet.
                        if (l->times_guessed >= l->times) {
                            color = BRIGHT_RED;
                        } else if (l->status == UNGUESSED) {
                            color = BRIGHT_YELLOW;
                            l->status = WRONG;
                        } else {
                            color = BRIGHT_YELLOW;
                        }
                        break;
                    }
                }
                // If we went through all the locs and didn't find this letter,
                // then it should appear still unguessed since we haven't decided
                // what else to do with it.
                // In that case, letter i of guess is not in the secret word.
                // Print red if this is the case.
                // Only touch the alphabet if it was previously unguessed.
                if (l->status == UNGUESSED) {
                    color = BRIGHT_RED;
                    l->status = UNUSED;
                // Finally, if we already know the letter does not appear anywhere
                // in the secret word, we can safely color it red.
                } else if (l->status == UNUSED) {
                    color = BRIGHT_RED;
                }
                l->times_guessed++;
                printf("%s%c", color, guess[i]);
            }
            printf("%s\n", NORMAL);
        }

        if (n_guess > MAX_GUESSES) {
            printf("You used all your guesses.\n");
            printf("The word was '%s'.\n", (char *)secret->data);
            break;
        }
    }

    free_words(&answers);
    free_words(&guesses);
    return 0;
}
