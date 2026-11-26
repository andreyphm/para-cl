import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory


TEST_DIR = Path(__file__).resolve().parent
TOKEN_SPELLINGS = TEST_DIR.parents[1] / "frontend" / "include" / "token_spellings.hpp"

EXPRESSION_OUTPUT = (
    "42\n0\n2147483647\n7\n-1\n-2147483647\n-7\n5\n1\n0\n0\n-1\n"
    "4\n16\n12\n-10\n"
    "2\n-2\n-2\n2\n0\n6\n"
    "1\n0\n0\n1\n1\n0\n5\n0\n1\n0\n1\n0\n"
    "0\n0\n0\n1\n5\n0\n0\n1\n1\n1\n5\n0\n"
    "41\n42\n"
    "14\n20\n12\n6\n-2\n1\n3\n1\n1\n1\n0\n0\n1\n1\n1\n1\n1\n"
)

@dataclass(frozen=True)
class Case:
    stdout: bytes
    stdin: bytes = b""


CASES = {
    "factorial.0": Case(stdout=b"1\n", stdin=b"0\n"),
    "factorial.1": Case(stdout=b"1\n", stdin=b"1\n"),
    "factorial.2": Case(stdout=b"2\n", stdin=b"2\n"),
    "factorial.5": Case(stdout=b"120\n", stdin=b"5\n"),
    "factorial.7": Case(stdout=b"5040\n", stdin=b"7\n"),
    "fibonacci.0": Case(stdout=b"1\n", stdin=b"0\n"),
    "fibonacci.1": Case(stdout=b"1\n", stdin=b"1\n"),
    "fibonacci.2": Case(stdout=b"2\n", stdin=b"2\n"),
    "fibonacci.10": Case(stdout=b"89\n", stdin=b"10\n"),
    "fibonacci.20": Case(stdout=b"10946\n", stdin=b"20\n"),
    "fibonacci_if_else.0": Case(stdout=b"0\n", stdin=b"0\n"),
    "fibonacci_if_else.1": Case(stdout=b"1\n", stdin=b"1\n"),
    "fibonacci_if_else.10": Case(stdout=b"55\n", stdin=b"10\n"),
    "gcd.common_factor": Case(stdout=b"6\n", stdin=b"48 18\n"),
    "gcd.coprime": Case(stdout=b"1\n", stdin=b"35 64\n"),
    "gcd.equal": Case(stdout=b"21\n", stdin=b"21 21\n"),
    "gcd.zero_second": Case(stdout=b"27\n", stdin=b"27 0\n"),
    "gcd.zero_first": Case(stdout=b"27\n", stdin=b"0 27\n"),
    "short_circuit.and_false_or_true": Case(
        stdout=b"0\n1\n41\n0\n1\n99\n", stdin=b"41 0 -3 99\n"
    ),
    "short_circuit.and_false_or_false": Case(
        stdout=b"0\n1\n12\n0\n0\n34\n", stdin=b"12 0 0 34\n"
    ),
    "short_circuit.and_true_or_false": Case(
        stdout=b"0\n1\n56\n1\n0\n78\n", stdin=b"56 -2 0 78\n"
    ),
    "short_circuit.and_true_or_true": Case(
        stdout=b"0\n1\n90\n1\n1\n17\n", stdin=b"90 2 -5 17\n"
    ),
    "short_circuit.mixed_whitespace": Case(
        stdout=b"0\n1\n-11\n1\n1\n-22\n", stdin=b"-11\n  7\t9\n-22"
    ),
    "nested_loops": Case(stdout=b"36\n"),
    "scopes": Case(stdout=b"1\n1\n1\n3\n"),
    "statement_bodies": Case(stdout=b"3\n4\n"),
    "empty_statement": Case(stdout=b"42\n", stdin=b"5 7 0 -3 0 42\n"),
    "logical_not": Case(stdout=b"1\n0\n0\n3\n0\n1\n42\n", stdin=b"0 42\n"),
    "nested_if_else.inner_true": Case(stdout=b"3\n", stdin=b"1 1\n"),
    "nested_if_else.inner_false": Case(stdout=b"2\n", stdin=b"1 0\n"),
    "nested_if_else.else_if_true": Case(stdout=b"1\n", stdin=b"0 1\n"),
    "nested_if_else.else_if_false": Case(stdout=b"0\n", stdin=b"0 0\n"),
    "dangling_else": Case(stdout=b"2\n"),
    "expressions": Case(
        stdout=EXPRESSION_OUTPUT.encode() + b"1\n1\n7\n7\n"
    ),
    "empty": Case(stdout=b""),
}


def program_template(name: str) -> str:
    if name == "empty":
        return ""
    return (TEST_DIR / "programs" / f"{name}.in").read_text(encoding="utf-8")


def render_program(template: str) -> str:
    header = TOKEN_SPELLINGS.read_text(encoding="utf-8")
    entries = re.findall(r'\{\s*"([^"\\]+)"\s*,\s*TokenKind::(\w+)\s*\}', header)
    spellings = {name: spelling for spelling, name in entries}

    def replace_marker(match: re.Match) -> str:
        name = match.group(1)
        if name not in spellings:
            raise ValueError(f"Unknown token marker: @{name}")
        return spellings[name]

    return re.sub(r"@(\w+)", replace_marker, template)


def run_case(executable: Path, name: str) -> None:
    case = CASES[name]
    program_name = name.partition(".")[0]
    with TemporaryDirectory(prefix="para_cl-e2e-") as directory:
        source = Path(directory) / "program.paracl"
        source.write_bytes(render_program(program_template(program_name)).encode("utf-8"))
        result = subprocess.run(
            [str(executable.resolve()), str(source)],
            input=case.stdin,
            capture_output=True,
            timeout=5,
        )

    stdout = result.stdout.replace(b"\r\n", b"\n")
    if result.returncode != 0 or result.stderr or stdout != case.stdout:
        raise AssertionError(
            f"{name}: expected exit code 0, empty stderr and stdout {case.stdout!r}\n"
            f"actual exit code: {result.returncode}\n"
            f"actual stderr: {result.stderr!r}\n"
            f"actual stdout: {stdout!r}"
        )


if __name__ == "__main__":
    if sys.argv[1:] == ["--list-cases"]:
        print(*CASES, sep="\n")
        raise SystemExit(0)

    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("case", choices=CASES)
    args = parser.parse_args()
    run_case(args.executable, args.case)
