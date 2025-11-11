#!/usr/bin/env python3
import subprocess
import sys
import re
import argparse
from typing import List, Tuple, Optional, Iterable

SHELL_BIN = "./shell"
MARK_START = "=== LEXER DEBUG OUTPUT ==="
MARK_END = "=== END DEBUG OUTPUT ==="
ANSI_RE = re.compile(r"\x1B\[[0-?]*[ -/]*[@-~]")
ELLIPSIS = "..."
ID_PREFIX_RE = re.compile(r"^\s*\[(?P<id>[A-Za-z0-9_\-:.]+)\]\s*(?P<rest>.+)$")

def strip_ansi(s: str) -> str:
	return ANSI_RE.sub("", s)

def run_shell_with_cmd(cmd: str) -> str:
	"""Run shell with -c flag and capture output"""
	result = subprocess.run(
		[SHELL_BIN, "--debug=lexer", "-c", cmd],
		capture_output=True,
		text=True,
		timeout=5
	)
	return (result.stdout or "") + (result.stderr or "")

def extract_debug_block(output: str) -> List[str]:
	"""Extract lines between debug markers, or raise if not found"""
	lines = output.splitlines()
	block = []
	in_block = False
	for line in lines:
		if not in_block and MARK_START in line:
			in_block = True
			continue
		if in_block and MARK_END in line:
			return block
		if in_block:
			if line.strip():
				block.append(line)
	# If markers not found, include tail of output to help debugging
	raise RuntimeError("Debug markers not found in output:\n" + output.strip())

def parse_token_line(raw: str) -> Tuple[str, str]:
	# Example: " TOKEN_IDENTIFIER            | line 1 | echo"
	s = strip_ansi(raw)
	parts = s.split("|")
	if not parts:
		return ("", "")
	name = parts[0].strip()
	lexeme = parts[-1].strip() if len(parts) >= 3 else ""
	return (name, lexeme)

def parse_block(block_lines: List[str]) -> List[Tuple[str, str]]:
	result = []
	for l in block_lines:
		l = l.strip()
		if not l:
			continue
		name, lexeme = parse_token_line(l)
		if name:
			result.append((name, lexeme))
	return result

def parse_expectations(spec: str) -> List[Tuple[str, Optional[str]]]:
	# Spec: "TOKEN_A TOKEN_B TOKEN_C(x) TOKEN_D(y z) ..."
	tokens = []
	i = 0
	while i < len(spec):
		while i < len(spec) and spec[i].isspace():
			i += 1
		if i >= len(spec):
			break
		# ellipsis support
		if spec.startswith(ELLIPSIS, i):
			tokens.append((ELLIPSIS, None))
			i += len(ELLIPSIS)
			continue
		j = i
		while j < len(spec) and not spec[j].isspace() and spec[j] != '(':
			j += 1
		name = spec[i:j]
		lex = None
		i = j
		if i < len(spec) and spec[i] == '(':
			depth = 1
			i += 1
			start = i
			while i < len(spec) and depth > 0:
				if spec[i] == '(':
					depth += 1
				elif spec[i] == ')':
					depth -= 1
					if depth == 0:
						break
				i += 1
			lex = spec[start:i].strip() if depth == 0 else ""
			if i < len(spec) and spec[i] == ')':
				i += 1
		tokens.append((name, lex))
	return tokens

def filter_actual_by_expected(actual: List[Tuple[str, str]],
                             expected: List[Tuple[str, Optional[str]]]) -> List[Tuple[str, str]]:
	# Ignore whitespace tokens unless explicitly present in expected list
	expected_names = {name for name, _ in expected}
	if "TOKEN_WHITESPACE" not in expected_names:
		actual = [t for t in actual if t[0] != "TOKEN_WHITESPACE"]
	return actual

def compare_tokens(expected, actual) -> Tuple[bool, str]:
	# Apply ellipsis semantics
	if expected and expected[-1][0] == ELLIPSIS:
		expected = expected[:-1]
		# only compare prefix
		if len(actual) < len(expected):
			return False, f"count mismatch before ellipsis: expected at least {len(expected)}, got {len(actual)}"
		n = len(expected)
	else:
		n = min(len(expected), len(actual))

	ok = True
	msgs = []
	if expected and (not expected or expected[-1][0] != ELLIPSIS) and len(expected) != len(actual):
		ok = False
		msgs.append(f"count mismatch: expected {len(expected)}, got {len(actual)}")
	for idx in range(n):
		en, el = expected[idx]
		an, al = actual[idx]
		if en != an:
			ok = False
			msgs.append(f"[{idx}] token name mismatch: expected {en}, got {an}")
		if el is not None and el != al:
			ok = False
			msgs.append(f"[{idx}] lexeme mismatch for {en}: expected '{el}', got '{al}'")
	return ok, "; ".join(msgs)

def parse_test_line(ln: str, lineno: int) -> Tuple[str, str, str]:
	"""
	Return (test_id, input_str, expected_spec)
	test_id: explicit from [id] prefix or auto 'L<lineno>'
	"""
	if "=>" not in ln:
		raise ValueError("no => in test line")
	left, right = ln.split("=>", 1)
	left = left.strip()
	m = ID_PREFIX_RE.match(left)
	if m:
		test_id = m.group("id")
		input_str = m.group("rest").rstrip()
	else:
		test_id = f"L{lineno}"
		input_str = left.rstrip()
	exp = right.strip()
	return test_id, input_str, exp

def should_run(test_id: str, input_str: str, ids: Optional[Iterable[str]], grep: Optional[str]) -> bool:
	if ids:
		if test_id not in ids:
			return False
	if grep:
		if grep not in input_str:
			return False
	return True

def run_tests_from_file(path: str, ids: Optional[List[str]] = None, grep: Optional[str] = None,
                        verbose: bool = False, log_file: Optional[str] = None, fail_fast: bool = False) -> int:
	with open(path, "r", encoding="utf-8") as f:
		lines = [ln.rstrip("\n") for ln in f]

	failures = 0
	total = 0
	logf = None
	if verbose:
		log_path = log_file or "srcs/test/lexer_tests.log"
		logf = open(log_path, "w", encoding="utf-8")
		def vlog(msg: str):
			logf.write(msg + "\n")
		vlog(f"# lexer test run\n# file: {path}\n")
	else:
		def vlog(msg: str):
			pass

	for idx, ln in enumerate(lines, start=1):
		ln_strip = ln.strip()
		if not ln_strip or ln_strip.startswith("#"):
			continue
		if "=>" not in ln:
			print(f"[SKIP] malformed line (no =>): {ln}")
			continue

		try:
			test_id, input_str, exp = parse_test_line(ln, idx)
		except Exception as e:
			print(f"[SKIP] parse error at line {idx}: {e}")
			continue

		if not should_run(test_id, input_str, ids, grep):
			continue

		expected = parse_expectations(exp)

		try:
			output = run_shell_with_cmd(input_str)
			block_lines = extract_debug_block(output)
			actual = parse_block(block_lines)
			actual = filter_actual_by_expected(actual, expected)
		except subprocess.TimeoutExpired:
			failures += 1
			total += 1
			print(f"[FAIL] [{test_id}] {input_str}")
			print(f"  error: command timed out")
			vlog(f"[{test_id}] TIMEOUT for: {input_str}")
			if fail_fast:
				break
			continue
		except Exception as e:
			failures += 1
			total += 1
			print(f"[FAIL] [{test_id}] {input_str}")
			print(f"  error: {e}")
			vlog(f"[{test_id}] ERROR: {e}\nRAW OUTPUT:\n{output}")
			if fail_fast:
				break
			continue

		ok, detail = compare_tokens(expected, actual)
		total += 1

		if verbose:
			vlog(f"--- [{test_id}] ---")
			vlog(f"INPUT   : {input_str}")
			vlog(f"EXPECTED: {expected}")
			vlog(f"RAW OUT :\n{output}")
			vlog(f"BLOCK   :\n" + "\n".join(block_lines))
			vlog(f"ACTUAL  : {actual}")
			if not ok and detail:
				vlog(f"DETAILS : {detail}")
			vlog("")

		if ok:
			print(f"[OK] [{test_id}] {input_str}")
		else:
			failures += 1
			print(f"[FAIL] [{test_id}] {input_str}")
			print(f"  expected: {expected}")
			print(f"  actual  : {actual}")
			if detail:
				print(f"  details : {detail}")
			if fail_fast:
				break

	if verbose and logf:
		logf.write(f"\nSummary in log.\n")
		logf.close()

	print(f"\nSummary: {total - failures}/{total} passed")
	return 0 if failures == 0 else 1

def main():
	parser = argparse.ArgumentParser(description="Run lexer tests")
	parser.add_argument("file", nargs="?", default="srcs/test/lexer_tests.txt",
	                    help="tests file (default: srcs/test/lexer_tests.txt)")
	parser.add_argument("--id", dest="ids", action="append",
	                    help="run only test(s) with given id (can repeat)")
	parser.add_argument("--grep", dest="grep", help="run tests whose input contains substring")
	parser.add_argument("--verbose", action="store_true", help="enable verbose logging to file")
	parser.add_argument("--log-file", dest="log_file", help="path to verbose log file (default: srcs/test/lexer_tests.log)")
	parser.add_argument("--fail-fast", action="store_true", help="stop on first failure")
	args = parser.parse_args()
	sys.exit(run_tests_from_file(args.file, ids=args.ids, grep=args.grep,
	                             verbose=args.verbose, log_file=args.log_file, fail_fast=args.fail_fast))

if __name__ == "__main__":
	main()
