import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
from tempfile import TemporaryDirectory


TEST_DIR = Path(__file__).resolve().parent
TOKEN_SPELLINGS = TEST_DIR.parents[1] / "include" / "token_spellings.hpp"


@dataclass(frozen=True)
class Case:
    stdout: bytes
    stdin: bytes = b""


CASES = {
    "factorial": Case(stdout=b"120\n", stdin=b"5\n"),
    "fibonacci": Case(stdout=b"89\n", stdin=b"10\n"),
    "gcd": Case(stdout=b"6\n", stdin=b"48 18\n"),
    "nested_loops": Case(stdout=b"36\n"),
    "scopes": Case(stdout=b"1\n1\n1\n3\n"),
    "statement_bodies": Case(stdout=b"3\n4\n"),
    "expressions": Case(
        stdout=b"14\n20\n12\n6\n-2\n1\n3\n1\n1\n1\n0\n0\n1\n1\n1\n1\n1\n1\n1\n7\n7\n"
    ),
    "short_circuit": Case(stdout=b"0\n1\n41\n0\n1\n99\n", stdin=b"41 0 -3 99\n"),
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
    with TemporaryDirectory(prefix="para_cl-e2e-") as directory:
        source = Path(directory) / "program.paracl"
        source.write_bytes(render_program(program_template(name)).encode("utf-8"))
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
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("case", choices=CASES)
    args = parser.parse_args()
    run_case(args.executable, args.case)
