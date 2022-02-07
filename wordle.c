#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "mylist.h"

#define WORD_LEN 5
#define MAX_GUESSES 6

enum L_STATUS {
    CORRECT,
    WRONG,
    UNUSED,
    UNGUESSED
};

struct Letter {
    enum L_STATUS status;
    struct List *locs;
    int times;
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
    int n_guesses = load_words(&guesses, "words-guesses.txt");

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
    int bad_guess = 0;
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
        struct List *locs = malloc(sizeof(struct List));
        initList(locs);
        struct Node *prev = locs->head;
        int count = 0;
        for (int i=0; i<WORD_LEN; i++) {
            if (secret_word[i] == c) {
                int *p = malloc(sizeof(int));
                *p = i;
                prev = addAfter(locs, prev, p);
                count++;
            }
        }
        alphabet[c-'a'].status = UNGUESSED;
        alphabet[c-'a'].locs   = locs;
        alphabet[c-'a'].times  = count;
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
        // Take user input.
        while (1) {
            if (n_guess != 1) {
                // Update alphabet.
                printf("\x1b[%dF", n_guess+1);
                printf("\x1b[2K");
                for (int i=0; i<26; i++) {
                    if (alphabet[i].status == UNGUESSED) {          // Normal
                        color = "\033[0m";
                    } else if (alphabet[i].status == UNUSED) {      // Red
                        color = "\033[0;31m";
                    } else if (alphabet[i].status == CORRECT) {     // Green
                        color = "\033[0;32m";
                    } else if (alphabet[i].status == WRONG) {       // Yellow
                        color = "\033[0;33m";
                    }
                    printf("%s%c", color, toupper(alphabet[i].letter));
                }
                printf("\033[0m\n");
                printf("\x1b[%dE", n_guess-1+1);

                // Update guess counter.
                printf("\x1b[%dF", n_guess);
                if (bad_guess) {
                    printf("\x1b[1F");
                }
                printf("\x1b[2K");
                printf("(%d/%d)\n", n_guess, MAX_GUESSES);
                printf("\x1b[%dE", n_guess-1);
                if (bad_guess) {
                    printf("\x1b[1E");
                }
            }
            fgets(guess, sizeof(guess), stdin);
            guess[WORD_LEN] = 0;
            for (int i=0; guess[i]; i++) {
                guess[i] = tolower(guess[i]);
            }
            // Discard excess chars.
            while ((c = fgetc(stdin)) != 0 && c != '\n')
                ;
            printf("\x1b[1F");
            printf("\x1b[2K");
            if (bad_guess) {
                printf("\x1b[1F");
                printf("\x1b[2K");
            }

            // Make sure the word is in the list.
            guess_node = findNode(&guesses, guess, (int(*)(const void *,const void *))&strcmp);
            if (guess_node == NULL) {
                printf("Word '%s' not recognized. Guess again.\n", guess);
                bad_guess = 1;
                continue;
            } else {
                n_guess++;
                bad_guess = 0;
                break;
            }
        }

        // Check if it's the secret word.
        guess_node = findNode(&answers, guess, (int(*)(const void *, const void *))&strcmp);
        if (guess_node == secret) {
            printf("\033[1;32m%s\033[0m\n", (char *)secret->data);
            printf("You got it!\n");
            break;
        }
        // Return info about letters.
        // Come up with a better DS+algo for this search.
        else {
            for (int i=0; i<WORD_LEN; i++) {
                if (guess[i] == ((char *)secret->data)[i]) {
                    // Letter in correct spot: print green.
                    // This needs to be checked first so that green is highest
                    // priority, or else we might mislead players in cases where
                    // the letter appears multiple times in the word.
                    // e.g. in the word 'swiss', the second and third 's' will
                    // see the first 's' and stop at yellow without this check.
                    color = "\033[1;32m";
                    printf("%s%c", color, guess[i]);
                    alphabet[guess[i]-'a'].status = CORRECT;
                    continue;
                }
                for (int j=0; j<WORD_LEN; j++) {
                    if (guess[i] == ((char *)secret->data)[j]) {
                        // Letter i of guess is in secret word.
                        if (i == j) {
                            // Letter in correct spot: print green.
                            color = "\033[1;32m";
                            alphabet[guess[i]-'a'].status = CORRECT;
                        } else {
                            // Letter not in correct spot: print yellow.
                            color = "\033[1;33m";
                            alphabet[guess[i]-'a'].status = WRONG;
                        }
                        break;
                    } else {
                        // Letter i of guess not in secret word.
                        // Print white.
                        color = "\033[0;31m";
                        alphabet[guess[i]-'a'].status = UNUSED;
                    }
                }
                printf("%s%c", color, guess[i]);
            }
            printf("\033[0m\n");
        }

        if (n_guess > MAX_GUESSES) {
            printf("You used all your guesses.\n");
            printf("The word was '%s'.\n", (char *)secret->data);
            break;
        }
    }

    free_words(&answers);
    free_words(&guesses);
    for (int i=0; i<26; i++) {
        traverseList(alphabet[i].locs, &free);
        removeAllNodes(alphabet[i].locs);
    }
    return 0;
}
