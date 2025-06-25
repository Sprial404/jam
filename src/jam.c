#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linenoise.h"

#include "lexer.h"

static const char *input_file;
static const char *output_file;

static void print_version(void)
{
  fprintf(stderr, "Jam version 0.1.0\n");
  exit(0);
}

static void print_help(void)
{
  fprintf(
      stderr, "\n  Usage: jam [options] [input_file]\n"
              "\n  To get started, run `jam repl`.\n"
              "\n  Available options:\n"
              "    -h, --help     Show this help message and exit\n"
              "    -i, --input <file>  Specify input file to read from\n"
              "    -o, --output <file> Specify output file to write to\n"
              "    -v, --version  Show version information and exit\n"
              "\n"
  );
  exit(0);
}

static const char **parse_args(int *argc, const char **argv)
{
  const char *arg, **args = argv;

  for (int i = 0, len = *argc; i < len; i++) {
    arg = argv[i];

    if (!strcmp("-h", arg) || !strcmp("--help", arg)) {
      print_help();
    } else if (!strcmp("-v", arg) || !strcmp("--version", arg)) {
      print_version();
    } else if (!strcmp("-i", arg) || !strcmp("--input", arg)) {
      if (i + 1 < len) {
        input_file = argv[++i];
      } else {
        fprintf(stderr, "Error: Missing input file after '%s'\n", arg);
        exit(1);
      }
    } else if (!strcmp("-o", arg) || !strcmp("--output", arg)) {
      if (i + 1 < len) {
        output_file = argv[++i];
      } else {
        fprintf(stderr, "Error: Missing output file after '%s'\n", arg);
        exit(1);
      }
    } else if ('-' == arg[0]) {
      {
        fprintf(stderr, "unknown flag: %s\n", arg);
        exit(1);
      }
    }
  }

  return argv;
}

static void jam_repl(void)
{
  printf("Welcome to Jam v0.1.0\n");

  char *line;
  while ((line = linenoise("jam> ")) != NULL) {
    if (line[0] != '\0') {
      // Process the input line.
      Lexer lexer;
      lexer_init(&lexer, line);
      if (lexer_run(&lexer) == 0) {
        // Successfully lexed the input.
        for (size_t i = 0; i < lexer.token_count; i++) {
          const char *token_type_str = token_type_to_string(lexer.tokens[i].type);
          printf(
              "Token: %s (Type: %s, Line: %zu, Column: %zu)\n", lexer.tokens[i].lexeme, token_type_str,
              lexer.locations[i].line, lexer.locations[i].column
          );
        }
      } else {
        fprintf(stderr, "Error lexing input: %s\n", line);
      }
    }
    linenoiseFree(line);
  }

  exit(0);
}

static char *read_file_contents(const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Error opening file");
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *file_content = malloc(file_size + 1);
  if (!file_content) {
    perror("Error allocating memory");
    fclose(file);
    return NULL;
  }

  fread(file_content, 1, file_size, file);
  file_content[file_size] = '\0';

  fclose(file);
  return file_content;
}

int main(int argc, const char *argv[])
{
  argv = parse_args(&argc, argv);

  if (input_file != NULL) {
    // Read the input file contents.
    char *file_content = read_file_contents(input_file);
    if (file_content == NULL) {
      fprintf(stderr, "Error reading input file: %s\n", input_file);
      exit(1);
    }

    // Initialize the lexer with the file content.
    Lexer lexer;
    lexer_init(&lexer, file_content);
    if (lexer_run(&lexer) == 0) {
      // Successfully lexed the input.
      for (size_t i = 0; i < lexer.token_count; i++) {
        const char *token_type_str = token_type_to_string(lexer.tokens[i].type);
        printf(
            "Token: %s (Type: %s, Line: %zu, Column: %zu)\n", lexer.tokens[i].lexeme, token_type_str,
            lexer.locations[i].line, lexer.locations[i].column
        );
      }
    }

    free(file_content);
    lexer_free(&lexer);
  } else {
    jam_repl();
  }

  return 0;
}
