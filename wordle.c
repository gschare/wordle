#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include "mylist.h"

#define WORD_LEN 5
#define MAX_GUESSES 6

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
    struct List words;
    char *filename = "words.txt";
    int n_words = load_words(&words, filename);

    // Seed randomness.
    srand(time(NULL));

    // Initialize.
    int n_guess;
    char guess[WORD_LEN+1];
    int r;
    struct Node *guess_node;
    struct Node *secret;
    char *color;
    int c;
    int bad_guess = 0;

    // Start a game.
    n_guess = 1;

    // Choose one of these words at random to be the secret.
    if (argc != 2) {
        r = rand() % n_words;
        secret = words.head;
        for (int i=0; i<r; i++) {
            secret = secret->next;
        }
    } else {
        char *secret_word = argv[1];
        secret = findNode(&words, secret_word, (int(*)(const void*, const void*))&strcmp);
        if (secret == NULL) {
            printf("debug word not in word list\n");
            exit(1);
        }
    }

    printf("(%d/%d)\n", n_guess, MAX_GUESSES);
    while (1) {
        // Take user input.
        while (1) {
            if (n_guess != 1) {
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
            guess_node = findNode(&words, guess, (int(*)(const void *,const void *))&strcmp);
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
        if (guess_node == secret) {
            printf("\033[1;32m%s\033[0m\n", (char *)secret->data);
            printf("You got it!\n");
            break;
        }
        // Return info about letters.
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
                    continue;
                }
                for (int j=0; j<WORD_LEN; j++) {
                    if (guess[i] == ((char *)secret->data)[j]) {
                        // Letter i of guess is in secret word.
                        if (i == j) {
                            // Letter in correct spot: print green.
                            color = "\033[1;32m";
                        } else {
                            // Letter not in correct spot: print yellow.
                            color = "\033[1;33m";
                        }
                        break;
                    } else {
                        // Letter i of guess not in secret word.
                        // Print white.
                        color = "\033[0m";
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

    free_words(&words);
    return 0;
}
