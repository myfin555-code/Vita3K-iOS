#!/usr/bin/env python3
"""Validate real compiler output and interpret its straight-line address arithmetic.

This is a deliberately bounded SPIR-V test interpreter, not a GPU replacement.
The reference results use host 64-bit addition, independent of the emitted pair
of 32-bit words, sign extension and carry operations.
"""
import pathlib
import subprocess
import sys

root = pathlib.Path(sys.argv[1])
validator, disassembler = sys.argv[2:4]
modules = sorted(root.glob('*.spv'))
assert len(modules) >= 4, 'missing generated shader fixtures'
for module in modules:
    subprocess.run([validator, '--target-env', 'vulkan1.0', str(module)], check=True)

text = subprocess.check_output([disassembler, '--raw-id', str(root / 'address_arithmetic.spv')], text=True)
values, pointers, stores = {}, {}, {}
mask = (1 << 32) - 1
for line in text.splitlines():
    tokens = line.split()
    if not tokens:
        continue
    if tokens[0] == 'OpStore':
        stores[pointers[tokens[1]]] = values[tokens[2]]
        continue
    if len(tokens) < 4 or tokens[1] != '=':
        continue
    result, op = tokens[0], tokens[2]
    args = tokens[4:]  # result type is at index 3
    if op == 'OpConstant':
        values[result] = int(args[0]) & mask
    elif op in ('OpConstantComposite', 'OpCompositeConstruct'):
        values[result] = tuple(values[x] for x in args)
    elif op == 'OpCompositeExtract':
        values[result] = values[args[0]][int(args[1])]
    elif op == 'OpBitcast':
        values[result] = values[args[0]]
    elif op == 'OpShiftRightArithmetic':
        raw = values[args[0]]
        signed = raw - (1 << 32) if raw & (1 << 31) else raw
        values[result] = (signed >> values[args[1]]) & mask
    elif op in ('OpIAdd', 'OpIAddCarry'):
        total = values[args[0]] + values[args[1]]
        values[result] = (total & mask, total >> 32) if op == 'OpIAddCarry' else total & mask
    elif op == 'OpAccessChain':
        assert values[args[1]] == 0, 'unexpected result member'
        pointers[result] = values[args[2]]
expected = [tuple(map(int, line.split())) for line in (root / 'address_expected.txt').read_text().splitlines()]
assert len(stores) == len(expected) == 20
assert [stores[i] for i in range(len(expected))] == expected, 'signed GPU address arithmetic regression'
print(f'Validated {len(modules)} SPIR-V modules and 20 signed address cases')
