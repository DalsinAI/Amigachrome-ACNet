#!/usr/bin/env python3
"""Generate ACNet bsdsocket.library vectors from ACNet-owned manifests.

The core manifest contains only the classic application-facing Amiga BSD
socket ABI.  An optional compatibility guard manifest may extend the physical
vector table with failure-only slots so callers compiled for a larger legacy
ABI cannot jump beyond the library.

Usage:
    gen_vectors.py ABI OUT.c IMPL.c [...] [--guard GUARD.json]
"""
import json, os, re, sys

def failure(ret):
    if ret == "VOID": return None
    if "*" in ret or ret in ("STRPTR","APTR","BOOL"): return "0"
    return "-1"

def main():
    argv=sys.argv[1:]
    guard_path=None
    if "--guard" in argv:
        i=argv.index("--guard")
        try: guard_path=argv[i+1]
        except IndexError: raise SystemExit("--guard requires a manifest path")
        del argv[i:i+2]
    if len(argv)<3:
        raise SystemExit("usage: gen_vectors.py ABI OUT.c IMPL.c [...] [--guard GUARD.json]")
    abi,out,*impls=argv
    spec=json.load(open(abi,encoding='utf-8'))
    guard_spec=json.load(open(guard_path,encoding='utf-8')) if guard_path else None
    if spec.get('bias') != 30: raise SystemExit('unexpected ABI bias')

    have=set()
    for impl in impls:
        have |= set(re.findall(r'^\w[\w\s\*]*?\bbsd_(\w+)\s*\(',open(impl,encoding='latin-1').read(),re.M))

    inc=list(spec.get('includes',[])) + (list(guard_spec.get('includes',[])) if guard_spec else [])
    protos=['/* Generated from ACNet ABI manifest: do not edit. */',
            '#ifndef BSDSOCKET_PROTOS_H','#define BSDSOCKET_PROTOS_H',
            '#include "acnet_lib.h"']+[f'#include {x}' for x in inc]+['']
    body=['/* Generated from ACNet ABI manifest: do not edit. */',
          '#include "bsdsocket_protos.h"','']
    table=[]; done=0; stubs=0; core_slots=0

    for item in spec['slots']:
        if 'reserved' in item:
            n=int(item['reserved'])
            table += ['(APTR)lib_reserved']*n
            core_slots += n
            continue
        name,ret,args=item['name'],item['ret'],item.get('args',[])
        core_slots += 1
        protos.append(f"{ret} bsd_{name}("+
                      ', '.join(['struct SocketBase *sb']+[f'{t} {n}' for t,n,_ in args])+');')
        params=', '.join([f'REG({reg}, {typ} {name})' for typ,name,reg in args]+
                         ['REG(a6, struct SocketBase *sb)'])
        call=', '.join(['sb']+[name for _,name,_ in args])
        if name in have:
            stmt=f'bsd_{name}({call});' if ret=='VOID' else f'return bsd_{name}({call});'
            done += 1
        else:
            fail=failure(ret)
            stmt='lib_unimplemented(sb);'+('' if fail is None else f' return ({ret}){fail};')
            stmt += ''.join(f' (void){n};' for _,n,_ in args)
            stubs += 1
        body.append(f'static {ret} LIB_{name}({params}) {{ {stmt} }}')
        table.append(f'(APTR)LIB_{name}')

    guard_slots=0
    guard_impl=0
    if guard_path:
        guard=guard_spec
        expected=core_slots+1
        if guard.get('first_slot') != expected:
            raise SystemExit(f'guard starts at slot {guard.get("first_slot")}, expected {expected}')
        entries=guard.get('slots',[])
        if not entries or entries[-1].get('slot') != guard.get('last_slot'):
            raise SystemExit('guard manifest range is inconsistent')
        body += [
            '',
            '/* Compatibility-only guard tail. These slots are not ACNet APIs. */',
            'static LONG ACNET_GUARD_scalar(REG(a6, struct SocketBase *sb)) { lib_unimplemented(sb); return -1; }',
            'static APTR ACNET_GUARD_ptr(REG(a6, struct SocketBase *sb)) { lib_unimplemented(sb); return NULL; }',
            'static BOOL ACNET_GUARD_bool(REG(a6, struct SocketBase *sb)) { lib_unimplemented(sb); return FALSE; }',
            'static VOID ACNET_GUARD_void(REG(a6, struct SocketBase *sb)) { lib_unimplemented(sb); }',
            'static ULONG ACNET_GUARD_reserved(REG(a6, struct SocketBase *sb)) { lib_unimplemented(sb); return 0; }',
        ]
        for e in entries:
            if e.get('slot') != expected:
                raise SystemExit(f'guard slot sequence breaks at {e.get("slot")}, expected {expected}')
            kind=e.get('kind')
            if kind not in ('scalar','ptr','bool','void','reserved'):
                raise SystemExit(f'unknown guard kind {kind} at slot {expected}')
            name,ret,args=e.get('name'),e.get('ret'),e.get('args',[])
            if ret and name in have:
                protos.append(f"{ret} bsd_{name}("+
                              ', '.join(['struct SocketBase *sb']+[f'{t} {n}' for t,n,_ in args])+');')
                params=', '.join([f'REG({reg}, {typ} {arg})' for typ,arg,reg in args]+
                                 ['REG(a6, struct SocketBase *sb)'])
                call=', '.join(['sb']+[arg for _,arg,_ in args])
                stmt=f'bsd_{name}({call});' if ret=='VOID' else f'return bsd_{name}({call});'
                body.append(f'static {ret} LIB_{name}({params}) {{ {stmt} }}')
                table.append(f'(APTR)LIB_{name}')
                guard_impl += 1
            else:
                table.append(f'(APTR)ACNET_GUARD_{kind}')
            expected += 1
            guard_slots += 1

    protos += ['#endif']
    open(os.path.join(os.path.dirname(out) or '.', 'bsdsocket_protos.h'),'w').write('\n'.join(protos)+'\n')

    body += ['', 'const APTR acnet_vectors[] = {',
             '    (APTR)lib_open, (APTR)lib_close, (APTR)lib_expunge, (APTR)lib_reserved,']
    for i in range(0,len(table),4):
        body.append('    '+', '.join(table[i:i+4])+',')
    body += ['    (APTR)-1','};',
             f'/* core: {core_slots} slots, {done} implemented, {stubs} stubs; guard: {guard_slots} slots, {guard_impl} implemented. */']
    open(out,'w').write('\n'.join(body)+'\n')
    print(f'{out}: core {core_slots} slots ({done} implemented, {stubs} stubs), guard {guard_slots} ({guard_impl} implemented), physical {len(table)}')

if __name__=='__main__':
    main()
