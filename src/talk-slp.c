/*
 ============================================================================
 Name        : talk-slp.c
 Author      : Reiner Jung
 Version     :
 Copyright   : Public Domain
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum EMode {
	NORMAL, LISTING, TABLE, COPY
};

int level = 0;
char *level_stack[100];
enum EMode mode = NORMAL;
int line_number;

void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}

void process_image(char **token, int size) {
	printf("img:[");
	if (size == 2) {
		printf("width=\\textwidth");
	} else {
		for (int i=1;i < size-1;i++) {
			printf("%s",token[i]);
			if (i < size - 2) printf(",");
		}
	}
	printf("] %s\n",token[size-1]);
}

void process_uniq(char **token, int size) {
	printf("$~<%s>{", token[1]);
	level_stack[level++] = "}\n";
}

void process_cite(char **token, int size) {
	if (strcmp("base", token[2]) == 0)
		printf("\\vspace{-6mm}{\\hfill\\parencite{%s}}", token[1]);
	else
		printf("\\hfill\\parencite{%s}", token[1]);
	if (mode == NORMAL) {
		putchar('\n');
	}
}

void process_two_column_start(char **token, int size) {
	if (size > 1) {
		char *percentage = "";
		if (strncmp(token[1],"width",5) == 0) {
			char *separator = strchr(token[1],'=');
			separator++;
			trim(separator);
			percentage = separator;
		}
		printf("col:%s\n", percentage);
	} else
		printf("col:\n");
}

void process_two_column_sep(char **token, int size) {
	if (size > 1) {
			char *percentage = "";
			if (strncmp(token[1],"width",5) == 0) {
				char *separator = strchr(token[1],'=');
				separator++;
				trim(separator);
				percentage = separator;
			}
			printf("sep:%s\n", percentage);
		} else
			printf("sep:\n");
}

void process_two_column_end(char **token, int size) {
	printf("end:\n");
}

void process_command(char *command) {
	// tokenize
	char *token[100];
	int token_count = 0;
	token[token_count++] = command;
	for (char *ch=command; *ch != 0 ; ch++) {
		if (isspace(*ch)) {
			*ch = 0;
			ch++;
			for (; isspace(*ch); ch++);
			token[token_count++] = ch;
		}
	}

	// execute
	if (strcmp("image", token[0]) == 0)
		process_image(token, token_count);
	else if (strcmp("uniq", token[0]) == 0)
		process_uniq(token, token_count);
	else if (strcmp("cite", token[0]) == 0)
		process_cite(token, token_count);
	else if (strcmp("two-column", token[0]) == 0)
		process_two_column_start(token, token_count);
	else if (strcmp("sep", token[0]) == 0)
		process_two_column_sep(token, token_count);
	else if (strcmp("/two-column", token[0]) == 0)
		process_two_column_end(token, token_count);
}

void command_parser(char *line) {
	for (char *ch=line; *ch != 0; ch++) {
		if (*ch == '[') { // command start
			char *start = ch + 1;
			for (; *ch != 0 && *ch != ']';ch++);
			char *end = ch;
			if (*end == ']') {
				*end = 0;
				process_command(start);
				end++;
				printf("%s",end);
			} else {
				fprintf(stderr,"[%d]: Missing ] in command\n", line_number);
			}
		} else {
			putchar(*ch);
		}
	}
	if (level > 0) {
		// level down
		for (int i = level-1; i >= 0 ; i--) {
			printf("%s", level_stack[i]);
		}
		level = 0;
	}
}

void process_listing_start(char *line) {
	mode = LISTING;
	char *language = line+3;
	printf("-- %s\n", language);
}

void process_listing_line(char *line) {
	if (strncmp("```", line, 3) == 0) {
		mode = NORMAL;
		printf("--\n");
	} else {
		printf("%s",line);
	}
}

void process_buffer(char *line) {
	if (mode == NORMAL) {
		if (line[0] == '#') { // section headline
			printf("\nsec: %s\n", line+2);
		} else if (line[0] == '>') { // slide title, can be copied
			printf("\n%s\n",line);
		} else if (line[0] == ':') { // copy to slp
			mode = COPY;
			level_stack[level++]="\n";
			command_parser(line+2);
			mode = NORMAL;
		} else if (line[0] == '[') { // start of complex command
			command_parser(line);
		} else if (strncmp(line,"```",3) == 0) { // listing block
			process_listing_start(line);
		}
	} else if (mode == LISTING) {
		process_listing_line(line);
	}
}

int main(int argc, char *argv[]) {

	char buffer[1024];

	FILE *input = fopen(argv[1], "r");
	FILE *output = stdout;

	line_number = 0;
	while(fgets(buffer,1023,input) != NULL) {
		if (strlen(buffer) > 0) {
			line_number++;
			if (mode ==NORMAL) trim(buffer);
			process_buffer(buffer);
		}
	}

	fclose(output);
	fclose(input);

	return EXIT_SUCCESS;
}
