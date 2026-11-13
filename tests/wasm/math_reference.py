#!/usr/bin/env python3
# Copyright (c) 2026- David Lucius Severus
# Distributed under the Boost Software License, Version 1.0.
# Test-only mpmath oracle, never a library/build dependency.
import json
import sys
import mpmath as mp
mp.mp.prec = 200
SIGN=1<<127; INF=0x7fff<<112; NAN=INF|(1<<111)
def encode(x):
    if mp.isnan(x): return NAN
    if mp.isinf(x): return INF | (SIGN if x<0 else 0)
    sign, n, e, _ = x._mpf_
    if not n: return SIGN if sign else 0
    top=n.bit_length()-1+e
    unit=max(top-112,-16494)
    shift=unit-e
    if shift<=0: q=n<<-shift
    else:
        q,r=divmod(n,1<<shift); h=1<<(shift-1)
        q+=r>h or (r==h and q&1)
    exponent=unit+112+16383
    bits=q if exponent<=0 else ((exponent-1)<<112)+q
    return (SIGN if sign else 0)|min(bits,INF)
def decode(b):
    e=(b>>112)&32767; f=b&((1<<112)-1)
    return (-1 if b&SIGN else 1)*mp.mpf(f+((1<<112) if e else 0))*mp.power(2,(e or 1)-16383-112)
funcs=[mp.exp,lambda x:mp.power(2,x),mp.expm1,mp.log,lambda x:mp.log(x,2),mp.log1p,mp.sin,mp.cos,mp.tan,
       mp.atan,mp.asin,mp.acos,mp.sinh,mp.cosh,mp.tanh,mp.asinh,mp.acosh,mp.atanh,
       lambda x:mp.sign(x)*mp.root(abs(x),3),mp.power,lambda x,y:mp.sqrt(x*x+y*y),mp.atan2,
       mp.erf,mp.erfc,mp.gamma,lambda x:mp.log(abs(mp.gamma(x))),lambda x:mp.besselj(0,x),lambda x:mp.besselj(1,x),lambda x:mp.bessely(0,x),lambda x:mp.bessely(1,x)]
def generate():
    values=['-10','-1','-0.75','-0.125','0.125','0.5','0.75','1','1.25','2','4','10','32','63','64','128']
    rows=[]
    for op,fn in enumerate(funcs):
        for text in values:
            a=encode(mp.mpf(text)); x=decode(a); y=mp.mpf('1.25')
            if op in [3,4,16,24,25,28,29] and x<=0: continue
            if op==16 and x<1:continue
            if op in [5] and x<=-1:continue
            if op in [10,11] and abs(x)>1:continue
            if op==17 and abs(x)>=1:continue
            if op==19 and x<0:continue
            try:r=fn(x,y) if op in [19,20,21] else fn(x)
            except (ValueError,ZeroDivisionError):continue
            rows.append([op,hex(a),hex(encode(y)),hex(encode(r))])
    # Extended exponents, range-reduction and tiny-input seams.
    for op,text in [(0,'10000'),(0,'-11432'),(1,'-16494'),(3,'1e4000'),(4,'1e-4000'),(18,'1e-4000'),(18,'-1e4000'),(22,'1e-1000'),(23,'106.8')]:
        a=encode(mp.mpf(text)); rows.append([op,hex(a),'0x0',hex(encode(funcs[op](decode(a))))])
    mp.mp.prec=17500
    for exponent in [114,1000,10000,16383]:
        a=encode(mp.power(2,exponent)+mp.power(2,exponent-112))
        for op in [6,7,8]:rows.append([op,hex(a),'0x0',hex(encode(funcs[op](decode(a))))])
    return {'generator':'mpmath '+mp.__version__, 'cases':rows}

if __name__ == '__main__':
    json.dump(generate(),sys.stdout,indent=2)
