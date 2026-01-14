#!/usr/bin/env python3
import os
import re
import sys

# Scans the repository for .h files, parses quoted includes, builds graph and finds cycles.
INCLUDE_RE = re.compile(r'#\s*include\s*"([^"]+)"')


def collect_headers(root):
    headers = []
    for dirpath, dirs, files in os.walk(root):
        for f in files:
            if f.endswith('.h'):
                headers.append(os.path.normpath(os.path.join(dirpath, f)))
    return headers


def build_graph(headers, root):
    header_set = set(headers)
    by_basename = {}
    for h in headers:
        by_basename.setdefault(os.path.basename(h), []).append(h)

    adj = {os.path.relpath(h, root): set() for h in headers}

    for h in headers:
        rel_h = os.path.relpath(h, root)
        try:
            with open(h, 'r', encoding='utf-8', errors='ignore') as fh:
                for line in fh:
                    m = INCLUDE_RE.search(line)
                    if not m:
                        continue
                    inc = m.group(1)
                    target = None
                    # if include has a path, resolve relative to this header
                    if '/' in inc:
                        cand = os.path.normpath(os.path.join(os.path.dirname(h), inc))
                        if cand in header_set:
                            target = cand
                    if target is None:
                        # try to match by basename
                        cands = by_basename.get(os.path.basename(inc), [])
                        if cands:
                            # prefer headers under incs/ when ambiguous
                            pref = [c for c in cands if os.path.normpath(os.path.join(root, 'incs')) in os.path.commonpath([root, c])]
                            if pref:
                                target = pref[0]
                            else:
                                target = cands[0]
                    if target:
                        adj[rel_h].add(os.path.relpath(target, root))
        except OSError:
            continue
    return adj


def find_cycles(adj):
    cycles = []
    temp_mark = set()
    perm_mark = set()
    stack = []

    def visit(node):
        if node in perm_mark:
            return
        if node in temp_mark:
            # found cycle; extract cycle path
            if node in stack:
                idx = stack.index(node)
                cycle = stack[idx:] + [node]
                cycles.append(cycle)
            return
        temp_mark.add(node)
        stack.append(node)
        for nb in adj.get(node, ()): 
            visit(nb)
        stack.pop()
        temp_mark.remove(node)
        perm_mark.add(node)

    for n in adj.keys():
        visit(n)
    # normalize cycles to avoid duplicates (rotate so smallest string first)
    norm = set()
    unique = []
    for c in cycles:
        if len(c) < 2:
            continue
        # represent cycle as tuple without duplicate final element
        cyc = c[:-1]
        # find rotation with minimal element first
        min_idx = min(range(len(cyc)), key=lambda i: cyc[i])
        rot = tuple(cyc[min_idx:] + cyc[:min_idx])
        if rot not in norm:
            norm.add(rot)
            unique.append(list(rot) + [rot[0]])
    return unique


def main():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    if len(sys.argv) > 1:
        root = os.path.abspath(sys.argv[1])
    headers = collect_headers(root)
    if not headers:
        print('No header files found under', root)
        return 0
    adj = build_graph(headers, root)
    cycles = find_cycles(adj)
    if not cycles:
        print('No header include cycles detected.')
        return 0
    print('Header include cycles detected:')
    for cyc in cycles:
        print('  ' + ' -> '.join(cyc))
    return 1


if __name__ == '__main__':
    sys.exit(main())
