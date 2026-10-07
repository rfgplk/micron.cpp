#!/usr/bin/env python3
# Copyright (c) 2026 David Lucius Severus
# Distributed under the Boost Software License, Version 1.0.
"""Native serial-executor overhead of reusable scans; no parallel speed claim."""
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def command(args):
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()

binary=ROOT/'bin/scan_workspace/bench/scan_workspace'
output=ROOT/'benches/results/scan_workspace.json'
if output.exists():
    raise RuntimeError('retain earlier results; choose a fresh output')
paths=sorted((ROOT/'src').rglob('*.hpp'))+[ROOT/'benches/scan_workspace.cpp',Path(__file__)]
hashes={str(p.relative_to(ROOT)):digest(p) for p in paths}
record={'commit':command(['git','rev-parse','HEAD']),'status':command(['git','status','--porcelain']),
        'compiler':command(['g++','--version']),'cpu':command(['lscpu']),
        'binary_sha256':digest(binary),'source_sha256':hashes,'samples':[],
        'correctness':json.loads((ROOT/'bin/scan_workspace/verified/results.json').read_text()),
        'scope':'serial executor, native core 0, 100 ms whole calls; ARM/i386 QEMU correctness only'}
for n in (32,256,4096,16384):
    for blocks in (1,4):
        for repeat in range(5):
            args=['taskset','-c','0',str(binary),str(n),str(blocks)]
            run=subprocess.run(args,cwd=ROOT,capture_output=True,text=True)
            clean=re.sub(r'\x1b\[[0-9;]*[A-Za-z]','',run.stdout)
            metrics=dict(re.findall(r'([a-zA-Z_/]+)=([^\s]+)',clean))
            sample={'n':n,'blocks':blocks,'repeat':repeat,'command':args,'exit':run.returncode,'stdout':clean,'stderr':run.stderr}
            record['samples'].append(sample)
            output.write_text(json.dumps(record,indent=2)+'\n')
            if run.returncode or metrics['allocs']!='0' or metrics['frees']!='0': raise RuntimeError(sample)
            sample['ns_per_op']=float(metrics['ns/op'])
record['summary']=[{'n':n,'blocks':b,'median_ns':statistics.median(s['ns_per_op'] for s in record['samples'] if (s['n'],s['blocks'])==(n,b))} for n in (32,256,4096,16384) for b in (1,4)]
record['sources_unchanged']=all(digest(ROOT/p)==h for p,h in hashes.items())
output.write_text(json.dumps(record,indent=2)+'\n')
if not record['sources_unchanged']: raise RuntimeError('source mutation during measurement')
print(record['summary'])
