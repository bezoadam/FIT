/*
 * Soubor:  proj3.c
 * Datum:   6.12.2014
 * Autor:   Adam Bezak - 1BIA - xbezak01@stud.fit.vutbr.cz
 * Projekt: Pruchod bludistem
 *			Vytvorte program, ktory v danom bludisku a jeho vstupe najde priechod von. Bludisko je ulozene v textovom subore vo forme
 *			obdlznikovej matice celych cisel. Cielom programu je vypis suradnic policok bludiska, cez ktore vedie cesta z vchodu bludiska do jeho vychodu
 *
 */

#include <stdio.h> // vstup a vystup
#include <string.h> // strtod
#include <stdlib.h> // vseobecne funkcie
#include <stdbool.h> // bool

/* definicie */

#define ERR_FAILURE_OPEN 10
#define ERR_TOO_MANY_ARGS 11
#define ERR_UNDEFINED_ARG 12
#define ERR_INVALID_TXT 13
#define ERR_ALOCATION_FAIL 14
#define ACTION_HELP 2
#define ACTION_RPATH 3
#define ACTION_LPATH 4
#define ACTION_TEST 5
#define TRUE 1
#define UNDEFINED -1
#define R 0
#define C 1
#define LEFT 0
#define RIGHT 1
#define UP_BOT 2
#define UP 3
#define DOWN 4
#define TURN_DOWN  1
#define TURN_UP  1
#define TURN_LEFT 1
#define TURN_RIGHT 1

/* struktura */ 
typedef struct {
	int rows;
	int cols;
	unsigned char *cells;
} Map;

/* prototypy funkcii */
void print_errors(int error); // vypis chybovych hlasiek
void help(); // napoveda
int parse_args(int argc, char *args[], int *action, int *limits); //spracovanie vstupnych parametrov
int test(char *args[], int *action, int *limits); // testovanie tabulky (--test)
bool isborder(Map *map, int r, int c, int border); // funkcia na testovanie hrany trojuholnika
int start_border(Map *map, int r, int c, int leftright); // funkcia na testovanie startovaceho vstupu do mapy
void rpath(Map *map, int r, int c, int leftright); // pravidlo pravej ruky
void lpath(Map *map, int r, int c, int leftright); // pravidlo lavej ruky

/*main*/
int main(int argc, char *argv[]) {
	int limits[2]; // pomocne pole pri spracovani vstupov
	int action; // premenna, ktora urcite co sa bude vykonavat
	int setup_error = parse_args(argc, argv, &action, limits); // spracovanie vstupu
	int path_error; 
	if (setup_error != TRUE) {
		print_errors(setup_error);
	}
	else {
		switch(action){
			case ACTION_HELP:
				help();
				break;
			case ACTION_TEST:
				path_error = test(argv, &action, limits);
				break;
			case ACTION_RPATH:
				path_error = test(argv, &action, limits); // funkcia test sa vykona vzdy, aj pri hladani cesty podla pravidla pravej ruky
				break;
			case ACTION_LPATH:
				path_error = test(argv, &action, limits); // funkcia test sa vykona vzdy, aj pri hladani cesty podla pravidla lavej ruky
				break;
		}
	}
	if (path_error != TRUE) {
		print_errors(path_error);
	}
	return 0;
}
int parse_args(int argc, char *args[], int *action, int *limits) {
	*action = 0;
	char *endptr = NULL; // ak strtol neprekonvertoval cislo
	argc--; // pre lepsi prehlad argc
	limits[R] = UNDEFINED;
	limits[C] = UNDEFINED;
	if (strcmp(args[1], "--help") == 0) { // --help
		if (argc > 1) return ERR_TOO_MANY_ARGS;
		*action = ACTION_HELP;
		return TRUE;
	} else if (strcmp(args[1], "--test") == 0) { // --test nazov.txt
			if (argc == 2){
			*action = ACTION_TEST;
		}
		else return ERR_UNDEFINED_ARG;
	} else if (strcmp(args[1], "--rpath") == 0) { // --rparh R C nazov.txt
			if (argc == 4){
			*action = ACTION_RPATH;
			limits[R] = strtol(args[2], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			limits[C] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
		}
			else return ERR_UNDEFINED_ARG;
	} else if (strcmp(args[1], "--lpath") == 0) { // --lpath R C nazov.txt
			if (argc == 4){
			*action = ACTION_LPATH;
			limits[R] = strtol(args[2], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			limits[C] = strtol(args[3], &endptr, 10); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
		}
			else return ERR_UNDEFINED_ARG;
	}
	else return ERR_UNDEFINED_ARG;
return TRUE;
}
int test(char *args[], int *action, int *limits){
	Map map; // vytvorenie struktury
	FILE *fr;
	int x; // nacitavanie cisel z textoveho suboru
	int invalid = 0; 
	int count = 0; // pocet 
	int setup;
	int col = 0;
	int row = 0;
	int path;
	int r = limits[R]; // aktualny row
	int c = limits[C]; // aktualny col
	int start_result;
	switch (*action) { //otvaranie suboru
		case ACTION_TEST:
			fr = fopen(args[2], "r");
			break;
		case ACTION_RPATH:
			fr = fopen(args[4], "r");
			path = ACTION_RPATH;
			break;
		case ACTION_LPATH:
			fr = fopen(args[4], "r");
			path = ACTION_LPATH;
			break;
	}
	if (fr == NULL) { // ak sa nepodaril otvorit
		return ERR_FAILURE_OPEN;
	}
	int rows = 0, cols = 0;
	fscanf(fr, "%d %d ", &rows, &cols); // nacitanie velkost matice
	map.rows = rows; // do struktury map.rows nacitam velkost matice
	map.cols = cols; // do struktury map.rows nacitam velkost matice
	map.cells = malloc(rows*cols*sizeof(unsigned char));
	if ((map.cells) == NULL) return ERR_ALOCATION_FAIL; // ak sa nepodarilo naalokovat strukturu
	for (int i=0; i < rows*cols; i++) {
		setup = fscanf(fr, "%d", &x);
		if ((x < 0) || (x > 7)) { // ak cislo nie je v platnom rozsahu
			invalid++;
		}
		else if (setup == -1) { // ak chybaju cisla v matici
			return ERR_INVALID_TXT;
		}
		else {
			map.cells[i] = (unsigned char) x;
			count++;
		}
	}
	if (invalid != 0) {
		return ERR_INVALID_TXT;
	}
	if (fscanf(fr, "%d", &x) == 1) {
		return ERR_INVALID_TXT; // ak je viac cisel v matici
	}
	for (int i = 0; i < rows*cols; i++){
		col = i % cols; //aktulny stlpec
		col++;
		if ((i % cols) == 0) {
			row++; // aktualny riadok
		}
		/* kontrola validity mapy */
		if (row == 1){ // ak sa nachadzame na vrchu mapy (row == 1) tak kontrolujem, ci lavy border suhlasi s pravym borderom susedneho trojuholnika
			if ((col != 1) && ((i + 1) % map.cols) > col-1) {
				if (isborder(&map, row, col, LEFT) != isborder(&map, row, col - TURN_LEFT, RIGHT)) { // kontrolujem ci sa nachadya lavy border s pravym borderom nasledujuceho trojuhonika
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
				}
				if (isborder(&map, row, col, RIGHT) != isborder(&map, row, col + TURN_RIGHT, LEFT)) { // kontrolujem ci sa nachadza pravy border s lavym borderom nasledujuceho trojuholnika
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
				}
				if (((row + col) % 2) != 0) {
					if (isborder(&map, row, col, UP_BOT) != isborder(&map, row + TURN_DOWN, col, UP_BOT)) { // spodny border s vrchnym borderom trojuhonika pod
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
					}
				}
			}
		}
		else if (col == 1) { // lava strana mapy
			if ((row != 1) && (row + TURN_DOWN < map.rows)) {
				if ((((row + col) % 2) != 0) && ((row + TURN_DOWN) > row)) {
					if (isborder(&map, row, col, UP_BOT) != isborder(&map, row + TURN_DOWN, col, UP_BOT)){
						free (map.cells);
						fclose (fr);
						return ERR_INVALID_TXT;
					}
				}
				if (isborder(&map, row, col, RIGHT) != isborder(&map, row, col + TURN_RIGHT, LEFT)){
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
				}
			}
		}
		else if (col == map.cols) { // prava strana mapy
			if ((row != 1) && (col + TURN_RIGHT < map.cols)){
				if ((((row + col) % 2) != 0) && ((row + TURN_DOWN) > row)) {
					if (isborder(&map, row, col, UP_BOT) != isborder(&map, row + TURN_DOWN, col, UP_BOT)) {
						free (map.cells);
						fclose (fr);
						return ERR_INVALID_TXT;
					}
				}
			}
		}
		else if (row == map.rows) { // spodna strana mapy
			if ((col != 1) && ((i + 1) % map.cols) > col-1) {
				if (isborder(&map, row, col, LEFT) != isborder(&map, row, col - TURN_LEFT, RIGHT)) {
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
				}
				if (isborder(&map, row, col, RIGHT) != isborder(&map, row, col + TURN_RIGHT, LEFT)) {
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
				}
				if (((row + col) % 2) == 0) {
					if (isborder(&map, row, col, UP_BOT) != isborder(&map, row - TURN_UP, col, UP_BOT)) {
						free (map.cells);
						fclose (fr);
						return ERR_INVALID_TXT;
					}
				}
			}
		}
		else if ((row == 1) && (col == 1)) { // lavy horny trojuholnik
			if (isborder(&map, row, col, RIGHT) != isborder(&map, row, col + TURN_RIGHT, LEFT)) {
				free (map.cells);
				fclose (fr);
				return ERR_INVALID_TXT;
			}
		}
		else if ((row == 1) && (col == map.cols)) { // pravy horny trojuholnik
			if (isborder(&map, row, col, LEFT) != isborder(&map, row, col - TURN_LEFT, RIGHT)) {
				free (map.cells);
				fclose (fr);
				return ERR_INVALID_TXT;				
			}
		}
		else if ((row == map.rows) && (col == 1)) { // lavy spodny trojuholnik
			if (isborder(&map, row, col, RIGHT) != isborder(&map, row, col + TURN_RIGHT, LEFT)) {
				free (map.cells);
				fclose (fr);
				return ERR_INVALID_TXT;
			}
		}
		else if ((row == map.rows) && (col == map.cols)) { // pravy spodny trojuholnik
			if (isborder(&map, row, col, LEFT) != isborder(&map, row, col - TURN_LEFT, RIGHT)) {
				free (map.cells);
				fclose (fr);
				return ERR_INVALID_TXT;				
			}
		}
		else { // vnutro mapy
				if (isborder(&map, row, col, RIGHT) != isborder(&map, row, col + TURN_RIGHT, LEFT)) {
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
				}
				if (isborder(&map, row, col, LEFT) != isborder(&map, row, col - TURN_LEFT, RIGHT)) {
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;				
			}
				if (((row + col) % 2) != 0) {
					if (isborder(&map, row, col, UP_BOT) != isborder(&map, row + TURN_DOWN, col, UP_BOT)) {
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
					}
				}
				if (((row + col) % 2) == 0) {
					if (isborder(&map, row, col, UP_BOT) != isborder(&map, row - TURN_UP, col, UP_BOT)) {
					free (map.cells);
					fclose (fr);
					return ERR_INVALID_TXT;
					}
				}
			}
		}
	switch (*action) { // ak bola mapa Validna tak sa dostaneme do tohto switchu
		case ACTION_TEST:
			printf("Valid\n");
			break;
		case ACTION_RPATH: // pravidlo pravej ruky
			start_result = start_border(&map, r, c, path);
			if (start_result == ERR_UNDEFINED_ARG) {
				print_errors(start_result);
			}
			else rpath(&map, r, c, start_result);
			break;
		case ACTION_LPATH: // pravidlo lavej ruky
			start_result = start_border(&map, r, c, path);
			if (start_result == ERR_UNDEFINED_ARG) {
				print_errors(start_result);
			}
			else lpath(&map, r, c, start_result);
	}
	free (map.cells); // dealokacia
	fclose (fr);
	return TRUE;	
}
void rpath(Map *map, int r, int c, int leftright){	
	int i = 0;
		while(1 == 1) {
			if ((r > map->rows) || (c > map->cols) || (r < 1) || (c < 1)) break; // ukoncenie nekonecneho cyklu pokial vyjdem z mapy
			printf("%d,%d\n", r, c);
		if (i == 0){ // ak este len vstupujeme do pola, tj. i == 0, tak kontroluje hrany podla podmienok
			if ((((c + r) % 2) != 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
			}
		else if ((((c + r) % 2) == 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
			}
		else if ((((c + r) % 2) == 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
			}
		else if ((((c + r) % 2) != 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
		}
		else if ((((c + r) % 2) != 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
		}
		else if ((((c + r) % 2) == 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
		}
		else if ((((c + r) % 2) != 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
		}
		i++;
	}
		else { // pokracovanie vo vnutri mapy
			/* Moj algoritmus na pohyb v bludisku spociva v kontrolovani hran na zaklade pravidla pravej/lavej ruky a predchadzajucej hrany
			* Ak pouzivam pravidlo podla pravej ruky (rpath) tak premenna leftright udava z ktorej hrany som prisiel do trojuholnika.
			* pr. ak som vstupil do trojuholnika /_\ ((c+r % 2)== 0) z lavej hrany tak  podla pravidla pravej ruky kontrolujem 
			* spodnu hranu, ak tam nie je tak sa posuniem o riadok nizsie (r++)
			* ak tam je tak sa posuvam na pravu hranu a tu kontrolujem, ak tam nie je tak sa posuniek o stlpec vpravo (c++)
			* ak je to "slepa cesta" tak sa vratim nazad
			*/
			if ((((c + r) % 2) != 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else {
					r++;
					leftright = UP_BOT;
				}
			}
			else if ((((c + r) % 2) != 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else {
					c--;
					leftright = RIGHT;
				}
			}
			else if ((((c + r) % 2) != 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else {
					c++;
					leftright = LEFT;
				}
			}
			else if ((((c + r) % 2) == 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else {
					c--;
					leftright = RIGHT;
				}
			}
			else if ((((c + r) % 2) == 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else {
					c++;
					leftright = LEFT;
				}
			}
			else if ((((c + r) % 2) == 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else {
					r--;
					leftright = UP_BOT;
				}
			}
		}
	}
}
void lpath(Map *map, int r, int c, int leftright) { // lpath pracuje na tom istom principe ako rpath
	int i = 0;
	while(1 == 1) {
		if ((r > map->rows) || (c > map->cols) || (r < 1) || (c < 1)) break;
		printf("%d,%d\n",r, c);
		if (i == 0){
			if ((((c + r) % 2) != 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
			}
		else if ((((c + r) % 2) == 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
			}
		else if ((((c + r) % 2) != 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
		}
		else if ((((c + r) % 2) != 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
		}
		else if ((((c + r) % 2) == 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
		}
		else if ((((c + r) % 2) == 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c,RIGHT)) == false) {
					c--;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
		}
		else if ((((c + r) % 2) != 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
		}
		i++;
	}
		else {
			if ((((c + r) % 2) != 0) && (leftright == UP_BOT)) { //neparny stlpec, parny riadok
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else {
					r++;
					leftright = UP_BOT;
				}
			}
			else if ((((c + r) % 2) != 0) && (leftright == LEFT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else {
					c--;
					leftright = RIGHT;
				}
			}
			else if ((((c + r) % 2) != 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r++;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else {
					c++;
					leftright = LEFT;
				}
			}
			else if ((((c + r) % 2) == 0) && (leftright == LEFT)) { //ak parny stlpec a parny riadok
				if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else {
					c--;
					leftright = RIGHT;
				}
			}
			else if ((((c + r) % 2) == 0) && (leftright == RIGHT)) {
				if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else if ((isborder(map, r, c, UP_BOT)) == false) {
					r--;
					leftright = UP_BOT;
				}
				else {
					c++;
					leftright = LEFT;
				}
			}
			else if ((((c + r) % 2) == 0) && (leftright == UP_BOT)) {
				if ((isborder(map, r, c, RIGHT)) == false) {
					c++;
					leftright = LEFT;
				}
				else if ((isborder(map, r, c, LEFT)) == false) {
					c--;
					leftright = RIGHT;
				}
				else {
					r--;
					leftright = UP_BOT;
				}
			}
		}
	}	
}
int start_border(Map *map, int r, int c, int leftright){
	if (leftright == ACTION_RPATH) { // ak hladame podla pravidla pravej ruky
		if ((c == 1) && ((r % 2) != 0) && (isborder(map, r, c, LEFT) == false)) { // vrati pravy border ak vstupujeme zlava a na lichom rowe
			return RIGHT;
		}
		else if ((c == 1) && ((r % 2) == 0) && (isborder(map, r, c, LEFT) == false)) { // vrati dolny border ak vstupujeme zlava na sudem rowe
			return UP_BOT;
		}
		else if ((r == 1) && ((c % 2) != 0) && (isborder(map, r, c, UP_BOT) == false )) { // vrati lavy border ak vstupujeme zhora
			return LEFT;
		}
		else if ((r == map->rows) && ((c % 2) == 0) && (isborder(map, r, c, UP_BOT) == false)) { // vrati pravy border ak vstupujeme zdola
			return RIGHT;
		}
		else if ((c == map->cols) && ((r % 2) != 0) && (isborder(map, r, c, RIGHT) == false)) { // vrati horny border ak vstupujeme zprava a na lichem rowe
			return UP_BOT;
		}
		else if ((c == map->cols) && ((r % 2) == 0) && (isborder(map, r, c, RIGHT) == false)) { // vrati lavy border ak vstupujeme zprava a na sudem rowe
			return LEFT;
		}
	}
	else if (leftright == ACTION_LPATH) {
		if ((c == 1) && ((r % 2) != 0) && (isborder(map, r, c, LEFT) == false)) { // vrati dolny border vstupujeme zlava na lichom rowe
			return UP_BOT;
		}
		else if ((c == 1) && ((r % 2) == 0) && (isborder(map, r, c, LEFT) == false)) { // vrati pravy border ak vstupujeme zlava na sudem rowe
			return RIGHT;
		}
		else if ((r == 1) && ((c % 2) != 0) && (isborder(map, r, c, UP_BOT) == false )) { // vati pravy border ak vstupujeme zhora
			return RIGHT;
		}
		else if ((r == map->rows) && ((c % 2) == 0) && (isborder(map, r, c, UP_BOT) == false)) { // vrati lavy border ak vstupujeme z dola
			return LEFT;
		}
		else if ((c == map->cols) && ((r % 2) != 0) && (isborder(map, r, c, RIGHT) == false)) { // vrati lavy border ak vstupujeme zprava na lichom rowe
			return LEFT; 
		}
		else if ((c == map->cols) && ((r % 2) == 0) && (isborder(map, r, c, RIGHT) == false)) { // vrati horny border ak vstupujeme zprava na sudem rowe
			return UP_BOT;
		}		
	}
	return ERR_UNDEFINED_ARG;
}
bool isborder(Map *map, int r, int c, int border){
r--; // dekrementacia koli indexovaniu pola od 0
c--;
	int number = map->cells[((r*map->cols)+c)]; // r*cols+c = vzorec na vypocet aktualneho cisla na suradniciach
	//printf("%d\n", number);
	switch(border) {
		case LEFT:
			if ((number & 1) == 1){ // bitovy AND s 1 
			return true;
			}
			else return false;
		break;
		case RIGHT:
			if ((number & 2) == 2){ // bitovy AND s 2
			return true;
			}
			else return false;
		break;
		case UP_BOT:
			if ((number & 4) == 4){ // bitovy AND s 4
			return true;
			}
			else return false;
		break;
	}
	return true;
}
void print_errors(int error) {
	switch (error) {
		case ERR_FAILURE_OPEN:
			fprintf(stderr,"ERROR: Subor sa nepodarilo otvorit.\n");
			break;
		case ERR_TOO_MANY_ARGS:
			fprintf(stderr,"ERROR: Zadal si vela argumentov\n");
			break;
		case ERR_UNDEFINED_ARG:
			fprintf(stderr,"ERROR: Zadal si nespravne argumenty\n");
			break;
		case ERR_INVALID_TXT:
			printf("Invalid\n");
			break;
		case ERR_ALOCATION_FAIL:
			fprintf(stderr,"ERROR: Chyba pri alokacii v pamati\n");
			break;
	}
}
void help() {
    printf ("\n-------------------------------NAPOVEDA-------------------------------\n"
           "\nPopis programu:\n"
           "\nProgram v danom bludisku a jeho vstupe najde priechod von.\n"
           "\nARGUMENTY PROGRAMU:\n"
           "\nArgument --help: \n"
           "-------Pri zadani argumentu --help sa vytiskne napoveda pouzivani programu.\n"
           "-------Tento argument funguje len samostatne!\n"
           "\nArgument --test nazov.txt\n"
           "\nnazov.txt - subor obsahujuci mapu\n"
           "\nSkontroluje, subor.txt obsahuje riadnu definiciu mapy bludiska.\n "
           "\nV pripade, ze format obrazku odpovida definicii vypise Valid\n"
           "\nV opacnom pripade program vypise Invalid\n"
           "\nArgument --rpath R C bludiste.txt\n"
           "\nhlada priechod bludiskom na vstupe na riadku R a stlpci C pomoci pravidla pravej ruky\n"
           "\nArgument --lpath R C bludiste.txt\n"
           "\nhlada priechod bludiskom na vstupe na riadku R a stlpci C pomoci pravidla lavej ruky\n"
		   "\nPoradie argumentov je pevne dane a nemoze sa lubovolne zamienat!!!\n");        
}