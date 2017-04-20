/*
 * Soubor:  proj2.c
 * Datum:   20.11.2014
 * Autor:   Adam Bezak - 1BIA - xbezak01@stud.fit.vutbr.cz
 * Projekt: Iteracne vypocty
 * Popis:   Implementujte vypocet vzdalenosti a vysky mereneho objektu pomoci udaju ze senzoru
 *			natoceni mericiho pristroje. Vypocet provedte pouze pomoci matematickych operaci +,-,*,/.
 *
 */

#include <stdio.h> // vstup a vystup
#include <string.h> // strtod
#include <stdlib.h> // vseobecne funkcie 
#include <math.h> // tangens

#define ERR_TOO_FEW_ARGS        11
#define ERR_TOO_MANY_ARGS       12
#define ERR_UNDEFINED_ARG       13
#define ERR_INVALID_LIMITS      14
#define ARGV_MAX_COUNT 			5

#define TRUE 					1
#define ACTION_HELP				1
#define ACTION_TAN 				2
#define ACTION_RANGE 			3

#define RADIANS 				0
#define WHERE_N 				1
#define WHERE_M 				2

#define RADIANS_A 				0
#define RADIANS_B 				1
#define HEIGHT 					2

#define UNDEF 					-1
#define MAX 					14
#define MAX_RAD 				1.4
#define CONST_HEIGHT 			1.5
#define MAX_HEIGHT 				100
#define ITERATIONS 				9

/* konstanty */
const long long citatel[] = {1, 1, 2, 17, 62, 1382, 21844, 929569, 6404582, 443861162, 18888466084, 113927491862, 58870668456604};
const long long menovatel[] = {1, 3, 15, 315, 2835, 155925, 6081075, 638512875, 10854718875, 1856156927625, 194896477400625, 49308808782358125, 3698160658676859375};

/* protypy funkcii */
int parse_args(int argc, char *args[], int *action, double *params_tan, double *params_range); // kontrola argumentov a spracovanie
void print_errors(int error); // vypis errorov
void help(); // vypis napovedy
void iteration_tan(double *params_tan); // iteracie tangens
void iteration_range(double *params_range, int argc); // iteracie vypocet vzdialenosti
double taylor_tan(double x, unsigned int n); // taylorov polynom
double cfrac_tan(double x, unsigned int n); // zretazeny zlomok
double f_ak(int i, double x); // pomocna funkcia pri vypocte zretazeneho zlomku
int f_bk(int i); // pomocna funkcia pri vypocte zretazeneho zlomku
double myabs(double n); // absolutna hodnota

/* MAIN */
int main(int argc, char *argv[]) {
	int action, setup_result;
	double params_tan[3]; // pomocne pole pre vypocet tangensu
	double params_range[3]; // pomocne pole pre vypocet vzdialenosti
	setup_result = parse_args(argc, argv, &action, params_tan, params_range);
    if (setup_result != TRUE) {
        print_errors(setup_result);
        return setup_result;
    }
	switch (action) { // switch vyhodnoti dany case na zaklade funkcie parse_args v ktorej sa meni hodnota premennej action (odkazom)
		case ACTION_HELP:
			help();
			break;
		case ACTION_TAN:
			iteration_tan(params_tan);
			break;
		case ACTION_RANGE:
			iteration_range(params_range, argc);
			break;
	}

	return 0;
}
int parse_args(int argc, char *args[], int *action, double *params_tan, double *params_range) {
	//////////////////// INICIALIZIACIA ////////////////////
	*action = 0;
	argc --;
	char *endptr = NULL;
		params_tan[RADIANS] = UNDEF;
		params_tan[WHERE_N] = UNDEF;
		params_tan[WHERE_M] = UNDEF;

		params_range[RADIANS_A] = UNDEF;
		params_range[RADIANS_B] = UNDEF;
		params_range[HEIGHT] = CONST_HEIGHT;
	//////////////////// ACTION ///////////////////////

	if (argc < 1) return ERR_TOO_FEW_ARGS;
	// max parametrov 5 - [-c X] -m A [B]
	if (argc > ARGV_MAX_COUNT) return ERR_TOO_MANY_ARGS;
	if (strcmp(args[1], "--help") == 0){
		if (argc > 1) return ERR_TOO_MANY_ARGS;
		*action = ACTION_HELP;
		return TRUE;
	}
	else if (strcmp(args[1], "--tan") == 0) *action = ACTION_TAN;
	else if (argc == 2) { // ak su argumenty ./proj2 -m A
		if (strcmp(args[1], "-m") == 0) {
			params_range[RADIANS_A] = strtod(args[2], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG; // ak argument A nie je cislo
			if (params_range[RADIANS_A] < 0) return ERR_INVALID_LIMITS; // ak argument A je mensi ako 0
			*action = ACTION_RANGE;
		}
		else return ERR_UNDEFINED_ARG;
	}
	else if (argc == 3) { // ./proj2 -m A B
		if (strcmp(args[1], "-m") == 0) {
			params_range[RADIANS_A] = strtod(args[2], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			params_range[RADIANS_B] = strtod(args[3], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			if ((params_range[RADIANS_A] < 0) || (params_range[RADIANS_B]) < 0) return ERR_INVALID_LIMITS;
			*action = ACTION_RANGE;
		}
		else return ERR_UNDEFINED_ARG;
	}
	else if (argc == 4) { // ./proj2 -c X -m A
		if (strcmp(args[1], "-c") == 0) {
			params_range[HEIGHT] = strtod(args[2], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			if (params_range[HEIGHT] < 0) return ERR_INVALID_LIMITS;
		}
		else return ERR_UNDEFINED_ARG;
		if (strcmp(args[3], "-m") == 0) {
			params_range[RADIANS_A] = strtod(args[4], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			if (params_range[RADIANS_A] < 0) return ERR_INVALID_LIMITS;
			*action = ACTION_RANGE;
		}
		else return ERR_UNDEFINED_ARG;
	}
	else if (argc == 5) { // ./proj2 -c X -m A B
		if (strcmp(args[1], "-c") == 0) {
			params_range[HEIGHT] = strtod(args[2], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			if (params_range[HEIGHT] < 0) return ERR_INVALID_LIMITS;
		}
		else return ERR_UNDEFINED_ARG;
		if (strcmp(args[3], "-m") == 0) {
			params_range[RADIANS_A] = strtod(args[4], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			if (params_range[RADIANS_A] < 0) return ERR_INVALID_LIMITS;
		}
		else return ERR_UNDEFINED_ARG;
		*action = ACTION_RANGE;
		params_range[RADIANS_B] = strtod(args[5], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
		if (params_range[RADIANS_B] < 0) return ERR_INVALID_LIMITS;
	}
	else return ERR_UNDEFINED_ARG;
	switch (*action) {
		case ACTION_TAN: 	
			if (argc < 4) return ERR_TOO_FEW_ARGS;
			if (argc > 4) return ERR_TOO_MANY_ARGS;
			params_tan[RADIANS] = strtod(args[2], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			params_tan[WHERE_N] = strtod(args[3], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			params_tan[WHERE_M] = strtod(args[4], &endptr); if (*endptr != '\0') return ERR_UNDEFINED_ARG;
			if ((params_tan[RADIANS] < 0) || (params_tan[WHERE_N] < 0)) return ERR_INVALID_LIMITS;
			break;
	} 
	for (int i = 0; i!=3; i++) { // iteracia na poli params_tan, ak nejaka hodnota == 0 tak error
		if ((params_tan[i] == 0) || (params_range[i] == 0)) {
		return ERR_INVALID_LIMITS;
		}
	}
/* kontrola podmienok zo zadania projektu */
	if (
		(params_tan[WHERE_M] < params_tan[WHERE_N]) ||
		(params_tan[WHERE_M] > MAX) ||
		(params_tan[RADIANS] > MAX_RAD)
		)
	{
		return ERR_INVALID_LIMITS;
	}

	if (
		(params_range[RADIANS_A] > MAX_RAD) ||
		(params_range[RADIANS_B] > MAX_RAD) ||
		(params_range[HEIGHT] > MAX_HEIGHT)
		)
	{
		return ERR_INVALID_LIMITS;
	}

	return TRUE;
}
void iteration_tan(double *params_tan) {
	double math_tan = tan(params_tan[RADIANS]); // tangens uhla z math.h
	double result_taylor, result_cfrac;
	double f_abs_taylor, f_abs_cfrac;
	for(int i = (int) params_tan[WHERE_N] ; i<=params_tan[WHERE_M]; i++) { // iterovanie i s pociatocnou hodnotou zadanou od uzivatela (params_tan[WHERE_N])
		result_taylor = taylor_tan(params_tan[RADIANS], (unsigned int) i); // do funkcie taylor_tan sa posiela zadany uhol a dane i
		result_cfrac = cfrac_tan(params_tan[RADIANS], (unsigned int) i); // do funkcie cfrac_tan sa posiela zadany uhol a dane i
		f_abs_taylor = myabs(result_taylor - math_tan); // absolutna odchylka medzi vysledkom z taylorovho polynomu a matematickou knihovnou
		f_abs_cfrac = myabs(result_cfrac - math_tan); // absolutna odchylka medzi zretazenym zlomkom a matematickou knihovnou
		printf("%d %e %e %e %e %e\n", (int) i, math_tan, result_taylor, f_abs_taylor, result_cfrac, f_abs_cfrac); // vypis vysledku
	}
}
void iteration_range(double *params_range, int argc) {
	argc--;
	double result_alfa, result_beta, range, height;

	if ((argc == 2) && ((params_range[RADIANS_B] != UNDEF) || (params_range[HEIGHT] != MAX_HEIGHT))) { // ak bolo zadane len ./proj2 -m A
		result_alfa = cfrac_tan(params_range[RADIANS_A], ITERATIONS); // do funkcie cfrac_tan sa posiela zadany uhol A a pocet iteracii (9)
		range = (params_range[HEIGHT] / result_alfa); // matematicky vzorec, podla vztahov goniometrickych funkcii v pravouhlom trojuholniku
		printf("%e\n", range);
	}
	else if ((argc == 3) && (params_range[HEIGHT] != MAX_HEIGHT)) { // ./proj2 -m A B
		result_alfa = cfrac_tan(params_range[RADIANS_A], ITERATIONS);
		result_beta = cfrac_tan(params_range[RADIANS_B], ITERATIONS);
		range = (params_range[HEIGHT] / result_alfa);
		height = (params_range[HEIGHT] + (result_beta * range));
		printf("%.10e\n%.10e\n", range, height);
	}
	else if ((argc == 4) && (params_range[RADIANS_B]) == UNDEF) { // ./proj2 -c X -m A
		result_alfa = cfrac_tan(params_range[RADIANS_A], ITERATIONS);
		range = (params_range[HEIGHT] / result_alfa);
		printf("%.10e\n", range);
	}
	else if (argc == 5) { // ./proj2 -c X -m A B
		result_alfa = cfrac_tan(params_range[RADIANS_A], ITERATIONS);
		result_beta = cfrac_tan(params_range[RADIANS_B], ITERATIONS);
		range = (params_range[HEIGHT] / result_alfa);
		height = (params_range[HEIGHT] + (result_beta * range));
		printf("%.10e\n%.10e\n", range, height);
	}
}
double taylor_tan(double x, unsigned int n) {
	double result;
	double help_y;
	unsigned int i = 0;
	double x_x = x*x;
	result = help_y = (x * citatel[i]/menovatel[i]); // pociatocny stav vysledku 
	for(i = 1; i < n; i++) { // cyklus je true pokial i < pocet iteracii z iteration_tan
		x = x * x_x; // zo vzorca x^2
		help_y = (x * citatel[i]/menovatel[i]); 
		result += help_y;
	}
	return result;
}
double cfrac_tan(double x, unsigned int n) {
	double result = 0.0;
	double ak, bk;
	for (unsigned int i = n; i > 0; i--) { // dekrementovanie i, pretoze zretazeny zlomok treba pocitat od konca
		ak = f_ak(i, x);
		bk = f_bk(i);
		result =  (ak / (bk + result));
	}
	return result;
}
double f_ak(int i, double x){ // ak i > 1 tak je minus.. podla vzorca
	return i > 1 ? -(x*x) : x; 
}
int f_bk(int i){ // 1,3,5,7 ....
	return (2*i)-1;
}
double myabs(double n) {
	return n < 0 ? -n : n;
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
        }
}
void help() {
    printf ("\n-------------------------------NAPOVEDA-------------------------------\n"
           "\nPopis programu:\n"
           "\nImplementujte vypocet vzdalenosti a vysky mereneho objektu pomoci udaju ze senzoru\n"
 		   "natoceni mericiho pristroje. Vypocet provedte pouze pomoci matematickych operaci +,-,*,/.\n"
           "\nARGUMENTY PROGRAMU:\n"
           "\nArgument --help: \n"
           "-------Pri zadani argumentu --help sa vytiskne napoveda pouzivani programu.\n"
           "-------Tento argument funguje len samostatne!\n"
           "\nArgument --tan\n"
           "\nVzor: --tan A N M\n"
           "\nVypise tangens radianu A z math.h, tangens pomocou taylorovho polynomu, odchylku\n "
           "\ntangens pomocou zretazenych zlomkov a jeho odchylku\n"
           "\nArgument -m\n"
           "\nVzor: [-c X] -m -A [B] // udaje v zatvorkach su nepovinne\n"
           "\n-c X : vyska 'cloveka'\n"
           "\n-m A : uhol alfa\n"
           "\n[B] : uhol beta\n"
		   "\nPoradie argumentov je pevne dane a nemoze sa lubovolne zamienat!!!\n");        
}