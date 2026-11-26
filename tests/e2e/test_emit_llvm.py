from pathlib import Path
from shutil import which
import subprocess
import sys
from tempfile import TemporaryDirectory
from threading import Timer

from test_cli import EXPRESSION_OUTPUT, program_template, render_program


def llvm_expressions() -> str:
    shared, _ = program_template("expressions").split("\n\n\n")
    return shared


def check_print(executable: str, lli: str, name: str, expected: str) -> None:
    check_print_program(executable, lli, program_template(f"llvm/{name}"), expected)


def check_print_program(executable: str, lli: str, template: str, expected: str) -> None:
    with TemporaryDirectory() as directory:
        source = Path(directory) / "print.paracl"
        source.write_text(render_program(template), encoding="utf-8")
        generated = subprocess.run([executable, "--emit-llvm", str(source)],
                                   capture_output=True, text=True, timeout=5)
        assert generated.returncode == 0 and not generated.stderr, generated
        executed = subprocess.run([lli, "-"], input=generated.stdout,
                                  capture_output=True, text=True, timeout=5)
        assert executed.returncode == 0 and not executed.stderr, executed
        assert executed.stdout == expected, executed
        interpreted = subprocess.run([executable, str(source)], capture_output=True, text=True, timeout=5)
        assert interpreted.returncode == 0 and not interpreted.stderr, interpreted
        assert interpreted.stdout == executed.stdout, interpreted


def check_loop_prefix(executable: str, lli: str, template: str, expected: list) -> None:
    with TemporaryDirectory() as directory:
        source = Path(directory) / "loop.paracl"
        ir = source.with_suffix(".ll")
        source.write_text(render_program(template), encoding="utf-8")
        with ir.open("wb") as output:
            subprocess.run([executable, "--emit-llvm", str(source)], stdout=output, check=True, timeout=5)
        with subprocess.Popen([lli, str(ir)], stdout=subprocess.PIPE, text=True) as process:
            deadline = Timer(5, process.kill)
            deadline.start()
            try:
                for value in expected:
                    line = process.stdout.readline(32)
                    assert line == f"{value}\n", (template, value, line)
            finally:
                deadline.cancel()
                deadline.join()
                process.kill()


def main(executable: str) -> None:
    cases = [
        ("empty", None),
        ("empty_statement", None),
        ("empty_block", None),
        ("nested_blocks", None),
        ("print_in_block", None),
        ("expressions", None),
        ("unsupported_after_print", "Visiting this expression is not implemented"),
        ("if_statement", None),
        ("while_statement", None),
    ]
    with TemporaryDirectory() as directory:
        source = Path(directory) / "program.paracl"
        for name, unsupported in cases:
            template = llvm_expressions() if name == "expressions" else program_template(f"llvm/{name}")
            source.write_text(render_program(template), encoding="utf-8")
            result = subprocess.run([executable, "--emit-llvm", str(source)],
                                    capture_output=True, text=True, timeout=5)
            if unsupported is None:
                assert result.returncode == 0 and not result.stderr, result
                assert "define i32 @main()" in result.stdout and "ret i32 0" in result.stdout, result
                if name == "expressions":
                    logical_operators = template.count("@LogicalAnd") + template.count("@LogicalOr")
                    assert result.stdout.count("br i1 ") == logical_operators, result
                if name == "while_statement":
                    body_end = result.stdout.split("\nif.end:", 1)[1].split("\n\n", 1)[0]
                    assert "br label %while.cond" in body_end, result
            else:
                assert result.returncode != 0 and not result.stdout, result
                assert unsupported in result.stderr, result


if __name__ == "__main__":
    if sys.argv[2:] in (["--run-print"], ["--run-loops"]):
        lli = which("lli")
        if lli is None:
            print("Skipping LLVM execution test: lli is missing from PATH")
            sys.exit(77)
        if sys.argv[2:] == ["--run-loops"]:
            finite, repeated, nested = program_template("llvm/while_statement").split("\n\n\n")
            check_print_program(sys.argv[1], lli, finite, "7\n8\n9\n")
            check_loop_prefix(sys.argv[1], lli, repeated, [1, 2] * 4)
            check_loop_prefix(sys.argv[1], lli, nested, [1, 2, 2, 2, 2])
            sys.exit(0)
        check_print_program(sys.argv[1], lli, llvm_expressions(), EXPRESSION_OUTPUT)
        check_print(sys.argv[1], lli, "if_statement", "1\n2\n3\n4\n5\n")
    else:
        main(sys.argv[1])
