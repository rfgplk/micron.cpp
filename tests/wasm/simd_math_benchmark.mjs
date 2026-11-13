// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
import {createMicron} from '../../src/bits/wasm/host.mjs';
import fs from 'node:fs';
import os from 'node:os';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
const cases=[['bench_simd',128,4096*128*4],['bench_quad_fma',2000,2000],['bench_quad_sqrt',2000,2000],['bench_quad_sin',256,256],
  ['bench_quad_lgamma',256,256],['bench_quad_gamma',128,128],['bench_quad_pow',128,128]];
const build=process.argv.includes('--build');
const files=build ? ['scalar','simd'].map(p=>`bin/wasm-math-review/bench-${p}/wasm_math_simd.wasm`) : process.argv.slice(2);
if(!files.length)throw new Error('pass --build or prebuilt module paths');
const report={node:process.version,cpu:os.cpus()[0].model,platform:process.platform,arch:process.arch,
  execution_context:'shared development machine; concurrent build/test load not controlled',
  unit:'ns_per_operation',warmups:3,samples:7,profiles:[],results:[]};
for(const [index,file] of files.entries()) {
  const profile={file};
  if(build) {
    const directory=file.slice(0,file.lastIndexOf('/'));
    const args=['build','benches/wasm_math_simd.cpp','--wasm32','--std','c++23','--start','start','-i','.','-i','src','-O2','--no-lto','-o',directory,'-f'];
    if(index===1)args.push('--wasm-simd128');
    const output=execFileSync('bin/duck',args,{encoding:'utf8'}).replace(/\x1b\[[0-9;]*m/g,'');
    const command=output.split('\n').find(line=>line.includes('with command: ')).split('with command: ')[1];
    Object.assign(profile,{compiler:execFileSync('/usr/bin/clang++',['--version'],{encoding:'utf8'}).split('\n')[0],command,
      optimization:'O2',lto:false,threads:false,exceptions:false});
  }
  const bytes=fs.readFileSync(file);
  Object.assign(profile,{bytes:bytes.length,sha256:createHash('sha256').update(bytes).digest('hex')});
  const runtime=await createMicron(bytes);
  profile.simdWidth=runtime.call('bench_simd_width');
  report.profiles.push(profile);
  runtime.call('bench_initialize');
  for(const [name,iterations,operations] of cases) {
    for(let i=0;i<3;++i)runtime.call(name,iterations);
    const samples=[];let checksum;
    for(let i=0;i<7;++i) {const start=process.hrtime.bigint();checksum=runtime.call(name,iterations);samples.push(Number(process.hrtime.bigint()-start)/operations);}
    samples.sort((a,b)=>a-b);
    report.results.push({file,name,operations,median_ns:samples[3],minimum_ns:samples[0],maximum_ns:samples[6],checksum});
  }
  await runtime.shutdown();
}
console.log(JSON.stringify(report,null,2));
