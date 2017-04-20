/*
 * Soubor:  proj1.c
 * Datum:   15.10.2014
 * Autor:   Adam Bezak - 1BIA - xbezak01@stud.fit.vutbr.cz
 * Projekt: Vypocty v tabulce
 * Popis:   Implementujte jednoduchy tabulkovy kalkulator. Program bude implementovat funkce vyhledani maxima, minima, funkce souctu
            a aritmetickeho prumeru vybranych bunek. Tabulku ve forme textoveho souboru bude program ocekavat na standartnim vstupu.
            Pozadovanou operaci a vyber bunek specifikuje uzivatel v argumentu prikazove radky.
 */

#include <stdio.h> /* vstup, vystup */
#include <stdlib.h> /* vseobecne funkcie */
#include <string.h> /* strcmp, strtok */
 
 
#define TRUE                    1
 
#define ERR_TOO_FEW_ARGS        11
#define ERR_TOO_MANY_ARGS       12
#define ERR_UNDEFINED_ARG       13
#define ERR_INVALID_LIMITS      14
 
#define ERR_LINE_LEN            21
#define ERR_MISSING_COL         22
#define ERR_MISSING_ROW         23
#define ERR_NON_NUMERIC         24
 
#define ARGS_MIN_COUNT          3
#define ARGS_MAX_COUNT          6
 
#define ACTION_HELP             1
#define ACTION_SELECT           2
#define ACTION_MIN              3
#define ACTION_MAX              4
#define ACTION_SUM              5
#define ACTION_AVG              6
 
#define WHERE_ROW               1
#define WHERE_COL               2
#define WHERE_ROWS              3
#define WHERE_COLS              4
#define WHERE_RANGE             5
 
#define LIMIT_ROWS_FROM         0
#define LIMIT_ROWS_TO           1
#define LIMIT_COLS_FROM         2
#define LIMIT_COLS_TO           3
 
#define LIMIT_OUT_YET           2 // PRED
#define LIMIT_OUT_ALREADY       3 // ZA
 
#define LIMIT_UNDEF             (-1)
 
#define LINE_MAX_LEN            1024
 
/* prototypy funkcii */
int parse_args(int arg_count, char *args[], int *action, int *where, int *limits); // spracovanie vstupu
void print_errors(int error); // vypis error code
void help(); // napoveda 
int read(int action, int *limits); // spracovanie stdin
int row_in_limit(int row, int *limits);  // funkcia na zistenie row-u v limite - aby sme predisli zbytocnym nacitavaniam stdinu, ktory nepotrebujeme 
int col_in_limit(int col, int *limits);  // funkcia na zistenie col-u v limite - aby sme predisli zbytocnym nacitavaniam stdinu, ktory nepotrebujeme 
 
/* main */ 
int main(int argc, char *argv[]) {
 
        int action, where, limits[4];
 
        int setup_result = parse_args(argc, argv, &action, &where, limits);
        int read_result;
 
        if (setup_result != TRUE) {
                print_errors(setup_result);
                return setup_result;
        }
        switch (action) {
                case ACTION_HELP:
                        help();
                        break;
                default:
                        read_result = read(action, limits);
                        if (read_result != TRUE) print_errors(read_result);
                        break;
        }
        return 0;
}
 
/* funkcia parse_args vrati TRUE (1) pri uspesnom spracovani argumentov alebo pri chybe dany ERROR CODE */
int parse_args(int arg_count, char *args[], int *action, int *where, int *limits) {
 
        //////////////////// INICIALIZIACIA ////////////////////
 
        *action = 0;
        *where = 0;
        char *endptr;
 
                limits[LIMIT_ROWS_FROM] = LIMIT_UNDEF;
                limits[LIMIT_ROWS_TO]   = LIMIT_UNDEF;
                limits[LIMIT_COLS_FROM] = LIMIT_UNDEF;
                limits[LIMIT_COLS_TO]   = LIMIT_UNDEF;
 
        arg_count--;
 
        //////////////////// ACTION SELECT ////////////////////
 
        // Ak je parametrov menej ako 1
        if (arg_count < 1) return ERR_TOO_FEW_ARGS;
 
        // Ak je parametrov viac ako 6 (SELECT RANGE A B X Y)
        if (arg_count > ARGS_MAX_COUNT) return ERR_TOO_MANY_ARGS;
 
        if (strcmp(args[1], "--help") == 0) {
                if (arg_count > 1) return ERR_TOO_MANY_ARGS;
                *action = ACTION_HELP;
                return TRUE;
        }
        // Ak strcmp.. == 0 tak str1 a str2 sa rovnaju, ak > 0 tak str2 je kratsi ako str1, ak < 0 tak str1 je kratsi ako str2
        else if (strcmp(args[1], "select") == 0)        *action = ACTION_SELECT;
        else if (strcmp(args[1], "min") == 0)           *action = ACTION_MIN;
        else if (strcmp(args[1], "max") == 0)           *action = ACTION_MAX;
        else if (strcmp(args[1], "sum") == 0)           *action = ACTION_SUM;
        else if (strcmp(args[1], "avg") == 0)           *action = ACTION_AVG;
        else                                                                                    return ERR_UNDEFINED_ARG;
 
        //////////////////// CELLS SELECT ////////////////////
 
        // Najmenej 3 parametre (SELECT ROW X)
        if (arg_count < ARGS_MIN_COUNT) return ERR_TOO_FEW_ARGS;
 
        if (strcmp(args[2], "row") == 0)                *where = WHERE_ROW;
        else if (strcmp(args[2], "col") == 0)           *where = WHERE_COL;
        else if (strcmp(args[2], "rows") == 0)          *where = WHERE_ROWS;
        else if (strcmp(args[2], "cols") == 0)          *where = WHERE_COLS;
        else if (strcmp(args[2], "range") == 0)         *where = WHERE_RANGE;
        else                                                                                    return ERR_UNDEFINED_ARG;
 
        //////////////////// CELLS SELECT CHECK ////////////////////
 
        switch (*where) {
                /*
                 * long int strtol (const char* str, char** endptr, int base);
                 * http://www.cplusplus.com/reference/cstdlib/strtol/
                 * strtol konvertuje string na long int
 
                 * **endptr - referencia na objekt typu *char, kde  moze funkcia ulozit pointer na miesto kde najde prvy
                    neciselny znak v danom stringu - funkcii predavam *endptr na zistenie ci dany parameter je iba cislo
                 * base - prevod sa uskutocnuje v ciselnej sustave ktoru definuje treti parameter funkcie (10 - dekadicke)
                */
                case WHERE_ROW:
                        if (arg_count > 3) return ERR_TOO_MANY_ARGS;
                        if (arg_count < 3) return ERR_TOO_FEW_ARGS;
                        limits[LIMIT_ROWS_FROM] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_ROWS_TO] = limits[LIMIT_ROWS_FROM];
                        break;
 
                case WHERE_ROWS:
                        if (arg_count > 4) return ERR_TOO_MANY_ARGS;
                        if (arg_count < 4) return ERR_TOO_FEW_ARGS;
                        limits[LIMIT_ROWS_FROM] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_ROWS_TO] = strtol(args[4], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        break;
 
                case WHERE_COL:
                        if (arg_count > 3) return ERR_TOO_MANY_ARGS;
                        if (arg_count < 3) return ERR_TOO_FEW_ARGS;
                        limits[LIMIT_COLS_FROM] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_COLS_TO] = limits[LIMIT_COLS_FROM];
                        break;
 
                case WHERE_COLS:
                        if (arg_count > 4) return ERR_TOO_MANY_ARGS;
                        if (arg_count < 4) return ERR_TOO_FEW_ARGS;
                        limits[LIMIT_COLS_FROM] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_COLS_TO] = strtol(args[4], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        break;
 
                case WHERE_RANGE:
                        if (arg_count > 6) return ERR_TOO_MANY_ARGS;
                        if (arg_count < 6) return ERR_TOO_FEW_ARGS;
                        limits[LIMIT_ROWS_FROM] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_ROWS_TO] = strtol(args[4], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_COLS_FROM] = strtol(args[5], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        limits[LIMIT_COLS_TO] = strtol(args[6], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
                        break;
        }

        // iteruje na poli limits, pokial nejaka hodnota == 0 (0 od uzivatela, alebo neuspesny prevod cisla zo strtol), tak ERROR
        for (int i=0; i!=4; i++) {
                if (limits[i] == 0) {
                        return ERR_INVALID_LIMITS;
                }
        }
 
        // podmienky zo zadania, ak prve cislo X je vacsie ako Y tak zle
        if (
                (limits[LIMIT_ROWS_FROM] > limits[LIMIT_ROWS_TO]) ||
                (limits[LIMIT_COLS_FROM] > limits[LIMIT_COLS_TO])
        ) {
                return ERR_INVALID_LIMITS;
        }
 
        // ak funkcia presla az sem, tak SUCCESS = 1
        return TRUE;
}
 
void print_errors(int error) {
        fprintf(stderr, "Chyba: ");
        switch (error) {
                case ERR_TOO_FEW_ARGS:
                        fprintf(stderr, "Zadal si malo argumentov, napis --help pre napovedu.\n");
                        break;
                case ERR_TOO_MANY_ARGS:
                        fprintf(stderr, "Zadal si vela argumentov, napis --help pre napovedu.\n");
                        break;
                case ERR_UNDEFINED_ARG:
                        fprintf(stderr, "Zadal si nedefinovane argumenty, napis --help pre napovedu.\n");
                        break;
                case ERR_INVALID_LIMITS:
                        fprintf(stderr, "Zadal si zle limity alebo 0, napis --help pre napovedu.\n");
                        break;
                case ERR_LINE_LEN:
                        fprintf(stderr, "Bola presiahnuta maximalna dlzka riadku: %d znakov.\n", LINE_MAX_LEN);
                        break;
                case ERR_NON_NUMERIC:
                        fprintf(stderr, "Program narazil v tabulke na neciselnu hodnotu.\n");
                        break;
                case ERR_MISSING_ROW:
                        fprintf(stderr, "Chybajuci riadok\n");
                        break;
                case ERR_MISSING_COL:
                        fprintf(stderr, "Chybajuci stlpec\n");
                        break;
        }
}
 
void help() {
    printf ("\n-------------------------------NAPOVEDA-------------------------------\n"
           "\nPopis programu:\n"
           "\nProgram funguje ako jednoduchy tabulkovy kalkulator, ktory vyhladava: \n"
           "maximun, minimum, vypocita sucet a aritmeticky priemer vybranych buniek.\n"
           "Tabulku vo formate textoveho suboru ocakava na standartnom vstupe.\n"
           "\nARGUMENTY PROGRAMU:\n"
           "\nArgument --help/-h: \n"
           "-------Pri zadani argumentu --help/- sa vytiskne napoveda pouzivani programu.\n"
           "-------Tento argument funguje len samostatne!\n"
           "\nArgument I:\n"
           "-------Povinny argument, ktory reprezentuje jednu z nasledujucich operacii.\n"
           "-------'select' Z danej tabulky vyberie a nasledne vytiskne hodnoty danych buniek.\n"
           "-------'min' Vyhladavanie a nasledny tisk minimalnej hodnoty z daneho rozsahu buniek.\n"
           "-------'max' Vyhladavanie a nasledny tisk maximalnej hodnoty z daneho rozsahu buniek.\n"
           "-------'sum' Vypocita a vytiskne sumy hodnot vsetkych vybranych buniek.\n"
           "-------'avg' Vypocita a vytiskne aritmeticky priemer vybranych buniek.\n"
           "\nArgument II:\n"
           "-------Povinny argument, ktory symbolizuje rozsah tabulky, nad ktorym ma byt prevedena dana operacia.\n"
           "-------Je dany argumentami :\n"
           "-------'row X' znaci vyber vsetkych buniek na riadku X. (X > 0)\n"
           "-------'col X' znaci vyber vsetkych buniek v stlpci X. (X > 0)\n"
           "-------'rows X Y' znaci vyber vsetkych buniek od riadku X (vratane) az po Y (vratane). (0 < X <= Y)\n"
           "-------'cols X Y' znaci vyber vsetkych buniek od stlpca X (vratane) az po Y (vratane). (0 < X <= Y)\n"
           "-------'range A B X Y' znaci vyber buniek od riadku A po riadok B a od stlpca X po stlpec Y,\n"
           "-------(vratane danych riadkov a stlpcov). (0 < A <= B), (0 < X <= Y)\n"
           "\nSyntax argumentov:\n"
           "I je povinny argument, ktory musi byt zadany ako prvy a za nim musi byt parameter II vo spravnom\n"
           "tvare a formate. Poradie argumentov je pevne dane a nemoze sa lubovolne zamienat!!!\n");        
}
 
int read(int action, int *limits) {

        int row = 1, col = 1;
        char *cell_endptr;
        char line[LINE_MAX_LEN + 2]; // dlzka riadku + 2 koli \n a rezerva aby som mohol nacitat 1025 znakov 
        char *cell; // ukazatel na char, strtok vrati pointer na char
        int cell_int = 0;
        int sum = 0, min = 0, max = 0, numerics_loaded = 0;
 
        /*
         * delimitery pre funkciu strtok, vsetky biele znaky, vid manual k funkcii isspace:
         * http://www.cplusplus.com/reference/cctype/isspace/
        */
        const char *delimiters = " \t\n\v\f\r";
 
        while (!feof(stdin) && (row_in_limit(row, limits) != LIMIT_OUT_ALREADY)) { // ak NIEJE koniec stdinu A row nie je uz LIMIT_OUT_ALREADY
 
                fgets(line, LINE_MAX_LEN + 2, stdin);
 
                if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0'; // zamiena pripadny \n za \0  ... -1 lebo string sa indexuje od 0
                if (strlen(line) > 1024) return ERR_LINE_LEN; // zistenie realnej dlzky stringu do ktoreho sa nepocita \n
 
                // ak je aktualny riadok nizsi nez spodny limit, tak preskoci rovno na dalsiu iteraciu - nacita dalsi riadok
                if (row_in_limit(row, limits) == LIMIT_OUT_YET) {
                        row++;
                        col = 1;
                        continue;
                }

               // rozkuskuje string na viacero tokenov na zaklade delimiterov
                cell = strtok(line, delimiters);
                while (cell != NULL && (col_in_limit(col, limits) != LIMIT_OUT_ALREADY)) {
                                // ak je aktualny stlpec nizsi nez spodny limit, tak preskoci rovno na dalsiu iteraciu - nacita dalsi stlpec
                                if (col_in_limit(col, limits) == LIMIT_OUT_YET) {
                                        cell = strtok(NULL, delimiters);
                                        col++;
                                        continue;
                                }
 
                                cell_int = strtol(cell, &cell_endptr, 10);
                               
                                if (*cell_endptr == '\0') {
                                        numerics_loaded++;
                                        switch (action) {
                                            case ACTION_MIN:
                                                    if (numerics_loaded == 1) min = cell_int;
                                                    if (cell_int < min) min = cell_int;
                                                    break;
                                            case ACTION_MAX:
                                                    if (numerics_loaded == 1) max = cell_int;
                                                    if (cell_int > max) max = cell_int;
                                                    break;
                                            case ACTION_AVG: case ACTION_SUM:
                                                    sum += cell_int;
                                                    break;
                                            case ACTION_SELECT:
                                                    printf("%.10g\n", (double)cell_int);
                                                    break;
                                            }
                                } else if (action == ACTION_SELECT) {
                                                printf("%s\n", cell);
                                } else {
                                        return ERR_NON_NUMERIC;
                                }
                        cell = strtok(NULL, delimiters);
                        col++;
                }
                if (limits[LIMIT_COLS_TO] != LIMIT_UNDEF && limits[LIMIT_COLS_TO] != col-1) {
                        return ERR_MISSING_COL; // chybajuci stlpec
                }
                row++; // Riadok nacitany
                col = 1; // Kazdy novy riadok resetuje col
        }
        if (limits[LIMIT_ROWS_TO] != LIMIT_UNDEF && limits[LIMIT_ROWS_TO] != row-1) { // chybajuci riadok
                return ERR_MISSING_ROW;
        }
        switch (action) {
                case ACTION_MIN:
                        printf("%.10g\n", (double)min);
                        break;
                case ACTION_MAX:
                        printf("%.10g\n", (double)max);
                        break;
                case ACTION_AVG:
                        printf("%.3f\n", (double)sum/numerics_loaded);
                        break;
                case ACTION_SUM:
                        printf("%.10g\n", (double)sum);
                        break;
        }
        return TRUE;
}
 
int row_in_limit(int row, int *limits) {
        if (
                (limits[LIMIT_ROWS_FROM] == LIMIT_UNDEF || row >= limits[LIMIT_ROWS_FROM]) && // ak LIMIT_ROWS_FROM je -1 ALEBO row je >= ako LIMIT_ROWS_FROM A
                (limits[LIMIT_ROWS_TO] == LIMIT_UNDEF || row <= limits[LIMIT_ROWS_TO]) // LIMIT_ROWS_TO je -1 ALEBO row <= LIMIT_ROWS_TO tak true
        )       return TRUE;
 
        if (limits[LIMIT_ROWS_FROM] != LIMIT_UNDEF && row < limits[LIMIT_ROWS_FROM]) // ak LIMIT_ROWS_FROM je iny nez -1  A row je < ako LIMIT_ROWS_FROM
                return LIMIT_OUT_YET;
 
        if (limits[LIMIT_ROWS_TO] != LIMIT_UNDEF && row > limits[LIMIT_ROWS_TO]) // ak LIMIT_ROWS_TO je iny nez -1 A row je > ako LIMIT_ROWS_TO
                return LIMIT_OUT_ALREADY;
 
        // sem by sa nemala funkcia dostat, ale koli warningu z kompilatora..
        return TRUE;
}
 
int col_in_limit(int col, int *limits) {
 
        if (
                (limits[LIMIT_COLS_FROM] == LIMIT_UNDEF || col >= limits[LIMIT_COLS_FROM]) &&
                (limits[LIMIT_COLS_TO] == LIMIT_UNDEF || col <= limits[LIMIT_COLS_TO])
        )       return TRUE;
 
        if (limits[LIMIT_COLS_FROM] != LIMIT_UNDEF && col < limits[LIMIT_COLS_FROM])
                return LIMIT_OUT_YET;
 
        if (limits[LIMIT_COLS_TO] != LIMIT_UNDEF && col > limits[LIMIT_COLS_TO])
                return LIMIT_OUT_ALREADY;
 
        // sem by sa nemala funkcia dostat, ale koli warningu z kompilatora..
        return TRUE;
}