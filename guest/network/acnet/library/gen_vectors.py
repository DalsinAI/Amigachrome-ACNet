#!/usr/bin/env python3
"""Generate ACNet's classic Amiga bsdsocket.library vectors from our own ABI manifest.

The manifest is ACNet-owned and contains only the
classic application-facing Amiga BSD socket ABI plus reserved growth slots.
"""
import json, os, re, sys

def failure(ret):
    if ret == "VOID": return None
    if "*" in ret or ret in ("STRPTR","APTR","BOOL"): return "0"
    return "-1"

def main():
    abi,out,*impls=sys.argv[1:]
    spec=json.load(open(abi,encoding='utf-8'))
    if spec.get('bias') != 30: raise SystemExit('unexpected ABI bias')
    have=set()
    for impl in impls:
        have |= set(re.findall(r'^\w[\w\s\*]*?\bbsd_(\w+)\s*\(',open(impl,encoding='latin-1').read(),re.M))
    inc=spec.get('includes',[])
    protos=['/* Generated from ACNet ABI manifest: do not edit. */','#ifndef BSDSOCKET_PROTOS_H','#define BSDSOCKET_PROTOS_H','#include "acnet_lib.h"']+[f'#include {x}' for x in inc]+['']
    body=['/* Generated from ACNet ABI manifest: do not edit. */','#include "bsdsocket_protos.h"','']
    table=[]; done=0; stubs=0; slots=0
    for item in spec['slots']:
        if 'reserved' in item:
            n=int(item['reserved']); table += ['(APTR)lib_reserved']*n; slots += n; continue
        name,ret,args=item['name'],item['ret'],item.get('args',[]); slots += 1
        protos.append(f"{ret} bsd_{name}("+', '.join(['struct SocketBase *sb']+[f'{t} {n}' for t,n,_ in args])+');')
        params=', '.join([f'REG({reg}, {typ} {name})' for typ,name,reg in args]+['REG(a6, struct SocketBase *sb)'])
        call=', '.join(['sb']+[name for _,name,_ in args])
        if name in have:
            stmt=f'bsd_{name}({call});' if ret=='VOID' else f'return bsd_{name}({call});'; done += 1
        else:
            fail=failure(ret); stmt='lib_unimplemented(sb);'+('' if fail is None else f' return ({ret}){fail};')+''.join(f' (void){n};' for _,n,_ in args); stubs += 1
        body.append(f'static {ret} LIB_{name}({params}) {{ {stmt} }}')
        table.append(f'(APTR)LIB_{name}')
    protos += ['#endif']
    open(os.path.join(os.path.dirname(out) or '.', 'bsdsocket_protos.h'),'w').write('\n'.join(protos)+'\n')
    body += ['', 'const APTR acnet_vectors[] = {', '    (APTR)lib_open, (APTR)lib_close, (APTR)lib_expunge, (APTR)lib_reserved,']
    for i in range(0,len(table),4): body.append('    '+', '.join(table[i:i+4])+',')
    body += ['    (APTR)-1','};',f'/* {slots} application/reserved slots; {done} implemented, {stubs} stubs. */']
    open(out,'w').write('\n'.join(body)+'\n')
    print(f'{out}: {slots} slots, {done} implemented, {stubs} stubs')
if __name__=='__main__': main()
