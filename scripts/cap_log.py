#!/usr/bin/env python3
import sys


def copy_unbounded(out):
    while True:
        chunk = sys.stdin.buffer.read(1024 * 1024)
        if not chunk:
            break
        out.write(chunk)


def copy_bounded(out, max_bytes):
    head_bytes = max_bytes // 2
    tail_bytes = max_bytes - head_bytes
    total = 0
    written_head = 0
    tail = bytearray()
    truncated = False

    while True:
        chunk = sys.stdin.buffer.read(1024 * 1024)
        if not chunk:
            break

        total += len(chunk)
        rest = chunk

        if written_head < head_bytes:
            take = min(len(chunk), head_bytes - written_head)
            if take:
                out.write(chunk[:take])
                written_head += take
            rest = chunk[take:]

        if rest:
            truncated = True
            if tail_bytes > 0:
                tail.extend(rest)
                if len(tail) > tail_bytes:
                    del tail[:-tail_bytes]

    if truncated:
        discarded = max(0, total - written_head - len(tail))
        message = (
            "\n\n--- LOG TRUNCATED: discarded %d bytes; "
            "kept first %d and last %d bytes ---\n\n"
        ) % (discarded, written_head, len(tail))
        out.write(message.encode("utf-8"))
        out.write(tail)


def main():
    if len(sys.argv) != 3:
        print("usage: cap_log.py LOG_PATH MAX_BYTES", file=sys.stderr)
        return 2

    log_path = sys.argv[1]
    try:
        max_bytes = int(sys.argv[2])
    except ValueError:
        print("MAX_BYTES must be an integer", file=sys.stderr)
        return 2

    with open(log_path, "wb") as out:
        if max_bytes <= 0:
            copy_unbounded(out)
        else:
            copy_bounded(out, max_bytes)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
