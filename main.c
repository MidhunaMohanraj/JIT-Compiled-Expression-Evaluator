/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * main.c
 */

#include "jitc.h"
#include "parser.h"
#include "system.h"


static double 
sigmoid(double x) {
	return (1.0 / (1.0 + exp(-x)));
}

static void
reflect(const struct parser_dag *dag, FILE *file)
{
	if (dag) {
		reflect(dag->left, file);
		reflect(dag->right, file);
		if (PARSER_DAG_VAL == dag->op) {
			fprintf(file,
				"double t%d = %f;\n",
				dag->id,
				dag->val);
		}
		else if (PARSER_DAG_NEG == dag->op) {
			fprintf(file,
				"double t%d = - t%d;\n",
				dag->id,
				dag->right->id);
		}
		else if (PARSER_DAG_MUL == dag->op) {
			fprintf(file,
				"double t%d = t%d * t%d;\n",
				dag->id,
				dag->left->id,
				dag->right->id);
		}
		else if (PARSER_DAG_DIV == dag->op) {
			fprintf(file,
				"double t%d = t%d ? (t%d / t%d) : 0.0;\n",
				dag->id,
				dag->right->id,
				dag->left->id,
				dag->right->id);
		}
		else if (PARSER_DAG_ADD == dag->op) {
			fprintf(file,
				"double t%d = t%d + t%d;\n",
				dag->id,
				dag->left->id,
				dag->right->id);
		}
		else if (PARSER_DAG_SUB == dag->op) {
			fprintf(file,
				"double t%d = t%d - t%d;\n",
				dag->id,
				dag->left->id,
				dag->right->id);
		}
		else {
			EXIT("software");
		}
	}
}

/* Creates the c code for the out.c file during run-time based on the 
expression typed in by the user (calls reflect as a helper) */
static void
generate(const struct parser_dag *dag, FILE *file, double (*sigmoidPtr)(double))
{
	fprintf(file, "double (*sigmoidPtrLocalCopy)(double);\n"); /* declare a var to make a local copy*/

	fprintf(file, "double evaluate(void) {\n");
	fprintf(file, "sigmoidPtrLocalCopy = (double (*)(double))%lu;\n", (unsigned long)sigmoidPtr);  /* Give the address of sigmoid to pointer copy */
	reflect(dag, file);
	fprintf(file, "return sigmoidPtrLocalCopy(t%d);\n", dag->id);
	fprintf(file, "}\n");
}


typedef double (*evaluate_t)(void);


int
main(int argc, char *argv[])
{
	const char *SOFILE = "out.so";
	const char *CFILE = "out.c";
	struct parser *parser;
	struct jitc *jitc;
	evaluate_t fnc;
	FILE *file;

    double (*sigmoidPtr)(double);   /* Declare a function pointer */
    sigmoidPtr = sigmoid;           /* Save the address of the sigmoid function in the pointer */

	/* usage */

	if (2 != argc) {
		printf("usage: %s expression\n", argv[0]);
		return -1;
	}

	/* parse */

	if (!(parser = parser_open(argv[1]))) {
		TRACE(0);
		return -1;
	}

	/* generate C */

	if (!(file = fopen(CFILE, "w"))) {
		TRACE("fopen()");
		return -1;
	}
	generate(parser_dag(parser), file, sigmoidPtr);
	parser_close(parser);
	fclose(file);

	/* JIT compile */

	if (jitc_compile(CFILE, SOFILE)) {
		file_delete(CFILE);
		TRACE(0);
		return -1;
	}
	file_delete(CFILE);

	/* dynamic load */

	if (!(jitc = jitc_open(SOFILE)) ||
	    !(fnc = (evaluate_t)jitc_lookup(jitc, "evaluate"))) {
		file_delete(SOFILE);
		jitc_close(jitc);
		TRACE(0);
		return -1;
	}
	printf("Value of Sigmoid = %f\n", fnc());

	/* done */

	file_delete(SOFILE);
	jitc_close(jitc);
	return 0;
}