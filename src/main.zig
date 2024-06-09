const std = @import("std");
const mem = std.mem;

const Lexer = @import("lexer.zig").Lexer;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer _ = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 2) {
        try repl(allocator);
    }
}

fn repl(allocator: mem.Allocator) !void {
    const stdin = std.io.getStdIn();
    const stdout = std.io.getStdOut();

    const reader = stdin.reader();
    const writer = stdout.writer();

    try writer.print("Jam 0.0.1\n", .{});
    try writer.print("Type \".help\" for more information or \".exit\" to quit.\n", .{});

    var buffer = try std.ArrayList(u8).initCapacity(allocator, 1024);
    defer buffer.deinit();

    const line_writer = buffer.writer();

    while (true) {
        try writer.print("> ", .{});

        try reader.streamUntilDelimiter(line_writer, '\n', null);
        const line = try buffer.toOwnedSliceSentinel(0);

        if (mem.eql(u8, line, ".exit")) {
            break;
        } else if (mem.eql(u8, line, ".help")) {
            try writer.print(
                \\Welcome to the Jam 0.0.1 help page
                \\To exit to program, type ".exit".
            ++ "\n", .{});
        } else {
            var lexer = Lexer.init(line);
            while (true) {
                const token = lexer.next();
                try writer.print("[{s} '{s}']\n", .{ @tagName(token.kind), line[token.location.start..token.location.end] });
                if (token.kind == .eof) break;
            }
        }
    }
}
