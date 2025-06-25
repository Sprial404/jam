#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void push_token(Lexer *lexer, Token type, char *lexeme);
static void ensure_capacity(Lexer *lexer);
static void handle_identifier(Lexer *lexer);

void lexer_init(Lexer *lexer, const char *source_code)
{
  assert(lexer != NULL);
  assert(source_code != NULL);

  lexer->start          = (char *)source_code;
  lexer->current        = lexer->start;
  lexer->line_start     = lexer->start;
  lexer->token_count    = 0;
  lexer->token_capacity = 0;
  lexer->line_number    = 1;
  lexer->tokens         = NULL;
  lexer->locations      = NULL;
}

int lexer_run(Lexer *lexer)
{
  while (*lexer->current != '\0') {
    switch (*lexer->current) {
    case ' ':
    case '\t':
    case '\r':
      // Ignore whitespace characters.
      break;
    case '\n':
      // Increment line number on newline.
      lexer->line_number += 1;
      lexer->line_start = lexer->current + 1;
      break;
    case '(':
      push_token(lexer, tok_lparen, "(");
      break;
    case ')':
      push_token(lexer, tok_rparen, ")");
      break;
    case '{':
      push_token(lexer, tok_lbrace, "{");
      break;
    case '}':
      push_token(lexer, tok_rbrace, "}");
      break;
    case '[':
      push_token(lexer, tok_lbracket, "[");
      break;
    case ']':
      push_token(lexer, tok_rbracket, "]");
      break;
    case ',':
      push_token(lexer, tok_comma, ",");
      break;
    case ':':
      if (*(lexer->current + 1) == ':') {
        // Handle double colon (::).
        lexer->current += 1; // Skip the second colon.
        push_token(lexer, tok_double_colon, "::");
      } else {
        // Handle single colon (:).
        push_token(lexer, tok_colon, ":");
      }
      break;
    case ';':
      push_token(lexer, tok_semicolon, ";");
      break;
    case '+':
      push_token(lexer, tok_plus, "+");
      break;
    case '-':
      push_token(lexer, tok_minus, "-");
      break;
    case '*':
      push_token(lexer, tok_star, "*");
      break;
    case '/':
      if (*(lexer->current + 1) == '/') {
        // Handle single-line comment.
        lexer->current += 2;
        while (*lexer->current != '\0' && *lexer->current != '\n') {
          lexer->current += 1;
        }
        continue;
      } else {
        push_token(lexer, tok_slash, "/");
      }
      break;
    case '=':
      push_token(lexer, tok_equal, "=");
      break;
    case '<':
      push_token(lexer, tok_less_than, "<");
      break;
    case '>':
      push_token(lexer, tok_greater_than, ">");
      break;
    case '.':
      push_token(lexer, tok_period, ".");
      break;
    case '&':
      push_token(lexer, tok_ampersand, "&");
      break;
    case '|':
      push_token(lexer, tok_pipe, "|");
      break;
    case '!':
      push_token(lexer, tok_exclamation, "!");
      break;
    case '^':
      push_token(lexer, tok_caret, "^");
      break;
    case '%':
      push_token(lexer, tok_percent, "%");
      break;
    default: {
      // Handle string literals.
      if (*lexer->current == '"') {
        lexer->current += 1;

        char *string_start = lexer->current;
        while (*lexer->current != '\0' && *lexer->current != '"') {
          if (*lexer->current == '\n') {
            lexer->line_number += 1;
            lexer->line_start = lexer->current + 1;
          }
          lexer->current += 1;
        }

        if (*lexer->current == '\0') {
          fprintf(stderr, "Unterminated string literal at line %zu\n", lexer->line_number);
          return -1; // Error: Unterminated string literal.
        }

        size_t length = lexer->current - string_start;
        char  *lexeme = (char *)malloc(length + 1);
        if (lexeme == NULL) {
          fprintf(stderr, "Memory allocation failed\n");
          exit(EXIT_FAILURE); // Error: Memory allocation failure.
        }

        strncpy(lexeme, string_start, length);
        lexeme[length] = '\0';

        push_token(lexer, tok_string, lexeme);
        free(lexeme); // Free the lexeme  since push token will copy it.

        lexer->current += 1; // Skip the closing quote.
        continue;
      } else if (isdigit(*lexer->current)) {
        // Handle numeric literals.
        char *number_start = lexer->current;

        while (isdigit(*lexer->current)) {
          lexer->current += 1;
        }

        if (*lexer->current == '.') {
          // Handle floating-point numbers.
          lexer->current += 1; // Skip the dot.
          while (isdigit(*lexer->current)) {
            lexer->current += 1;
          }
        }

        size_t length = lexer->current - number_start;
        char  *lexeme = (char *)malloc(length + 1);
        if (lexeme == NULL) {
          fprintf(stderr, "Memory allocation failed\n");
          exit(EXIT_FAILURE); // Error: Memory allocation failure.
        }

        strncpy(lexeme, number_start, length);
        lexeme[length] = '\0';

        push_token(lexer, tok_number, lexeme);
        free(lexeme); // Free the lexeme since push token will copy it.

        continue;
      } else if (isalpha(*lexer->current)) {
        handle_identifier(lexer);
        continue;
      } else {
        // Handle invalid characters.
        fprintf(stderr, "Invalid character '%c' at line %zu\n", *lexer->current, lexer->line_number);
        return -1; // Error: Invalid character.
      }
    }
    }

    lexer->current += 1; // Move to the next character.
  }

  // Push EOF token at the end of the input.
  push_token(lexer, tok_eof, NULL);
  return 0; // Success.
}

void lexer_free(Lexer *lexer)
{
  assert(lexer != NULL);

  for (size_t i = 0; i < lexer->token_count; i++) {
    if (lexer->tokens[i].lexeme != NULL) {
      if (lexer->tokens[i].type == tok_string || lexer->tokens[i].type == tok_identifier ||
          lexer->tokens[i].type == tok_number || lexer->tokens[i].type == tok_if || lexer->tokens[i].type == tok_else ||
          lexer->tokens[i].type == tok_while || lexer->tokens[i].type == tok_func ||
          lexer->tokens[i].type == tok_return) {
        // Free lexemes for string, identifier, number, and keywords.
        free(lexer->tokens[i].lexeme);
      }
    }
  }

  if (lexer->tokens != NULL) {
    free(lexer->tokens);
  }

  if (lexer->locations != NULL) {
    free(lexer->locations);
  }

  lexer->start          = NULL;
  lexer->current        = NULL;
  lexer->tokens         = NULL;
  lexer->locations      = NULL;
  lexer->token_count    = 0;
  lexer->token_capacity = 0;
  lexer->line_number    = 0;
}

static void push_token(Lexer *lexer, Token type, char *lexeme)
{
  ensure_capacity(lexer);

  lexer->tokens[lexer->token_count].type = type;

  if (lexeme != NULL) {
    char *allocated_lexeme = strdup(lexeme);
    if (allocated_lexeme == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE); // Error: Memory allocation failure.
    }

    lexer->tokens[lexer->token_count].lexeme = allocated_lexeme;
    lexer->tokens[lexer->token_count].length = strlen(allocated_lexeme);
  } else {
    lexer->tokens[lexer->token_count].lexeme = NULL;
    lexer->tokens[lexer->token_count].length = 0;
  }

  lexer->locations[lexer->token_count].line   = lexer->line_number;
  lexer->locations[lexer->token_count].column = (size_t)(lexer->current - lexer->line_start);
  lexer->locations[lexer->token_count].offset = (size_t)(lexer->current - lexer->start);

  lexer->token_count += 1;
}

static void ensure_capacity(Lexer *lexer)
{
  if (lexer->token_count >= lexer->token_capacity) {
    size_t new_capacity = lexer->token_capacity == 0 ? 8 : lexer->token_capacity * 2;

    TokenData *new_tokens    = (TokenData *)realloc(lexer->tokens, new_capacity * sizeof(TokenData));
    Location  *new_locations = (Location *)realloc(lexer->locations, new_capacity * sizeof(Location));

    if (new_tokens == NULL || new_locations == NULL) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE); // Error: Memory allocation failure.
    }

    lexer->tokens         = new_tokens;
    lexer->locations      = new_locations;
    lexer->token_capacity = new_capacity;
  }
}

static void handle_identifier(Lexer *lexer)
{
  char *identifier_start = lexer->current;

  while (isalnum(*lexer->current) || *lexer->current == '_') {
    lexer->current += 1;
  }

  size_t length = lexer->current - identifier_start;
  char  *lexeme = (char *)malloc(length + 1);
  if (lexeme == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE); // Error: Memory allocation failure.
  }

  strncpy(lexeme, identifier_start, length);
  lexeme[length] = '\0';

  // Determine the token type based on the lexeme.
  Token token_type = tok_identifier;

  if (strcmp(lexeme, "if") == 0) {
    token_type = tok_if;
  } else if (strcmp(lexeme, "else") == 0) {
    token_type = tok_else;
  } else if (strcmp(lexeme, "while") == 0) {
    token_type = tok_while;
  } else if (strcmp(lexeme, "return") == 0) {
    token_type = tok_return;
  } else if (strcmp(lexeme, "func") == 0) {
    token_type = tok_func;
  }

  push_token(lexer, token_type, lexeme);
  free(lexeme); // Free the lexeme  since push token will copy it.
}

const char *token_type_to_string(Token type)
{
  switch (type) {
  case tok_eof:
    return "EOF";
  case tok_lparen:
    return "Left Parenthesis";
  case tok_rparen:
    return "Right Parenthesis";
  case tok_lbrace:
    return "Left Brace";
  case tok_rbrace:
    return "Right Brace";
  case tok_lbracket:
    return "Left Bracket";
  case tok_rbracket:
    return "Right Bracket";
  case tok_comma:
    return "Comma";
  case tok_colon:
    return "Colon";
  case tok_semicolon:
    return "Semicolon";
  case tok_plus:
    return "Plus";
  case tok_minus:
    return "Minus";
  case tok_star:
    return "Star";
  case tok_slash:
    return "Slash";
  case tok_equal:
    return "Equal";
  case tok_ampersand:
    return "Ampersand";
  case tok_pipe:
    return "Pipe";
  case tok_exclamation:
    return "Exclamation";
  case tok_caret:
    return "Caret";
  case tok_percent:
    return "Percent";
  case tok_less_than:
    return "Less Than";
  case tok_greater_than:
    return "Greater Than";
  case tok_period:
    return "Period";
  case tok_if:
    return "If Keyword";
  case tok_else:
    return "Else Keyword";
  case tok_while:
    return "While Keyword";
  case tok_func:
    return "Func Keyword";
  case tok_return:
    return "Return Keyword";
  case tok_string:
    return "String Literal";
  case tok_identifier:
    return "Identifier";
  case tok_number:
    return "Number Literal";
  default:
    return "Unknown Token Type";
  }
}