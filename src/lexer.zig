const std = @import("std");

pub const Token = struct {
    kind: Kind,
    location: Location,

    pub const Location = struct {
        start: usize,
        end: usize,
    };

    pub const Kind = enum {
        invalid,
        eof,

        newline,
        space,
        line_comment,

        asterisk,
        colon,
        comma,
        dot,
        period,
        plus,
        minus,
        slash,

        identifier,
        number_literal,
    };
};

pub const Lexer = struct {
    source: [:0]const u8,
    index: usize,

    pub fn init(source: [:0]const u8) Lexer {
        // Skip the UTF-8 BOM at the start if present.
        const start_index: usize = if (std.mem.startsWith(u8, source, "\xEF\xBB\xBF")) 3 else 0;
        return .{
            .source = source,
            .index = start_index,
        };
    }

    const State = enum {
        start,
        identifier,
        int,
        line_comment,
        space,
        slash,
    };

    pub fn next(self: *Lexer) Token {
        var state: State = .start;
        var result = Token{ .kind = .eof, .location = .{
            .start = self.index,
            .end = undefined,
        } };

        while (true) : (self.index += 1) {
            const ch = self.source[self.index];

            switch (state) {
                .start => switch (ch) {
                    0 => {
                        if (self.index != self.source.len) {
                            result.kind = .invalid;
                            result.location.start = self.index;
                            self.index += 1;
                            result.location.end = self.index;
                            return result;
                        }
                        break;
                    },
                    ' ', '\t' => {
                        result.kind = .space;
                        state = .space;
                    },
                    '\n' => {
                        result.kind = .newline;
                        self.index += 1;
                        break;
                    },
                    '\r' => {
                        result.kind = .newline;
                        self.index += 1;

                        if (self.index < self.source.len and self.source[self.index] == '\n') {
                            self.index += 1;
                        }
                        break;
                    },
                    '*' => {
                        result.kind = .asterisk;
                        self.index += 1;
                        break;
                    },
                    ':' => {
                        result.kind = .colon;
                        self.index += 1;
                        break;
                    },
                    ',' => {
                        result.kind = .comma;
                        self.index += 1;
                        break;
                    },
                    '.' => {
                        result.kind = .dot;
                        self.index += 1;
                        break;
                    },
                    '+' => {
                        result.kind = .plus;
                        self.index += 1;
                        break;
                    },
                    '-' => {
                        result.kind = .minus;
                        self.index += 1;
                        break;
                    },
                    '/' => {
                        state = .slash;
                    },
                    'a'...'z', 'A'...'Z', '_' => {
                        result.kind = .identifier;
                        state = .identifier;
                    },
                    '0'...'9' => {
                        result.kind = .number_literal;
                        state = .int;
                    },
                    else => {
                        result.kind = .invalid;
                        self.index += 1;
                        result.location.end = self.index;
                        return result;
                    },
                },
                .identifier => switch (ch) {
                    'a'...'z', 'A'...'Z', '_', '0'...'9' => {},
                    else => break,
                },
                .int => switch (ch) {
                    'a'...'z', 'A'...'Z', '_', '0'...'9' => {},
                    else => break,
                },
                .line_comment => switch (ch) {
                    0 => {
                        if (self.index != self.source.len) {
                            result.kind = .invalid;
                            self.index += 1;
                        } else {
                            result.kind = .line_comment;
                        }
                        break;
                    },
                    '\n' => {
                        result.kind = .line_comment;
                        break;
                    },
                    '\t' => {},
                    else => {},
                },
                .space => switch (ch) {
                    ' ', '\t' => {},
                    else => break,
                },
                .slash => switch (ch) {
                    '/' => {
                        state = .line_comment;
                    },
                    else => {
                        result.kind = .slash;
                        break;
                    },
                },
            }
        }

        result.location.end = self.index;
        return result;
    }
};
