"""Random engines, distributions, and sampling; source-reviewed in full."""


def section(title, *paragraphs, table=None):
    value = {"title": title, "paragraphs": list(paragraphs)}
    if table is not None:
        value["table"] = table
    return value


def table(columns, rows):
    return {"columns": columns, "rows": rows}


def datatype(name, header, summary, *sections, covers=None):
    value = {"name": name, "header": header, "summary": summary,
             "sections": list(sections)}
    if covers:
        value["covers"] = covers
    return value


GUIDES = {"random": {
    "summary": "Deterministic integer engines, width-aware uniform draws, rejection/inverse-transform distributions, cached distribution objects, buffer filling, and sequence sampling. Engine state is explicit and owned; the dynamic-arity Dirichlet distribution instead borrows its parameter array.",
    "sections": [
        section("Engine selection", "`<micron/math/rng.hpp>` and `<micron/rng.hpp>` expose `micron::math::rng`. Engines use fixed embedded state and perform no allocation. next() returns one native result; generate(pointer,count) produces that same sequence while updating the engine. Copies/moves copy the state and duplicate the future stream rather than creating an independent stream.",
                table=table(["Engine", "State/result", "Implementation and use"], [
                    ["`splitmix64`", "8 bytes / u64", "Increment and invertible xor/multiply mixing; compact deterministic generator and seed expansion"],
                    ["`xoshiro256ss`", "32 bytes / u64", "Four-word xor/shift/rotate state with star-star output; jump and long_jump stream stepping"],
                    ["`xoshiro128ss`", "16 bytes / u32", "Four 32-bit words; next64 concatenates two consecutive results"],
                    ["`pcg64`", "32 bytes / u64", "128-bit linear congruential state, odd stream increment, xor/rotate output"],
                    ["`mt19937`", "624 u32 words plus index / u32", "624-word twist and temper; seed initialization truncates from_seed's u64 to u32"],
                    ["`mwc64`", "16 bytes / u64", "Two multiply-with-carry components combined into high/low 32-bit halves"],
                    ["`lcg64`", "8 bytes / u64", "Raw affine recurrence modulo 2^64; next32 explicitly uses the high half"],
                ])),
        section("Drawing and distribution namespaces", "rng_concept requires only next() convertible to u64. Internal width adapters prefer next32/next64 when supplied, otherwise truncate next() or concatenate two <=32-bit results. The concept does not verify full-width uniformity, engine quality, noexcept behavior, or period. A custom engine must satisfy the distribution's actual bit assumptions.",
                "`rng::dist` contains one-shot functions; `rng::dists` contains parameter-owning cached objects and sequence utilities. Samplers receive Rng& and advance its state. Distribution objects generally do not own an engine and const sampling does not mutate their parameter cache. Reproducibility requires the same engine state, distribution/parameters, result precision, and call order; rejection can consume a variable number of engine words.",
                table=table(["One-shot draw", "Output/domain", "Behavior and cost"], [
                    ["`uniform_real<F>`", "[0,1), or affine lo/hi interval", "Uses 53 high bits for sizeof(F)==8, otherwise 24 bits; O(1) word draws, no lo/hi validation"],
                    ["`uniform_open_real<F>`", "Strictly (0,1)", "Odd exactly representable mantissa grid, avoiding retry; 52 or 23 independently selected bits"],
                    ["`uniform_uint_below<U>`", "[0,range), zero when range==0", "32-/64-bit multiply-high with rejection; expected O(1), unbounded worst-case retries"],
                    ["`uniform_int<T>`", "Inclusive [lo,hi]", "hi<=lo returns lo; full-width interval uses a raw draw. Implementation widths are at most 64 bits, despite the wider integral constraint"],
                    ["`bernoulli`", "bool", "p<=0 false, p>=1 true, p=.5 uses one top bit; NaN comparison returns false"],
                    ["`normal`, `normal_ziggurat`", "mu+sigma*standard normal", "256-layer rejection/tail engine; no sigma validity check"],
                    ["`exp_dist`", "Exponential inverse transform", "-log(open_uniform)/lambda; lambda<=0 returns positive infinity"],
                    ["`poisson<I>`", "Nonnegative count", "Product loop below rate ten, transformed rejection otherwise; invalid/nonfinite/nonpositive rate returns zero"],
                ])),
        section("Cost and numerical conventions", "Engine steps are O(1) except a Mersenne Twister refill, which processes 624 words and is amortized over those outputs. generate and fills cost O(output_count) base work. Rejection samplers have expected constant trial cost for ordinary valid parameters and a suitable random source, but no finite worst-case trial bound. Small-rate one-shot Poisson uses an expected rate+1 open-uniform draws; cached Poisson changes that algorithm.",
                "pdf/pmf/cdf/ppf methods return numbers without a status or accuracy certificate. Incomplete-gamma/beta CDF helpers run at most 200 series/continued-fraction steps, with tolerances 2e-6 for four-byte F and 2e-15 otherwise; reaching the cap returns the last approximation. Normal ppf uses a piecewise rational approximation without iterative refinement. Overflow, tail cancellation, and integer result conversion are not uniformly checked.",
                "Parameter validation differs by method and distribution. A sampler's guard does not imply identical guards in its pdf/cdf/moment methods. I result types must represent sampled values; the unconstrained/default count types are not saturating wrappers. Floating parameters should be finite and satisfy the named distribution's mathematical domain."),
        section("Buffer and sequence operations", "fill::fill_uniform, fill_normal, fill_uniform_int, and fill_bytes write caller-owned buffers and retain no pointer. Uniform/normal fills unroll four scalar draws; they do not select a separate SIMD random stream. fill_bytes consumes ceil(bytes/8) next64 words and copies their native object bytes, so byte order is host-endian.",
                "dists::shuffle uses in-place Fisher-Yates with unbiased bounded draws, O(n) copies/assignments. The variadic container overload shuffles containers independently in sequence; it does not apply one shared permutation to aligned arrays. permutation first fills 0..n-1, then shuffles, requiring representable element indices.",
                "choice(items,n,g) returns one copied item and requires n>0. Weighted choice scans weights twice in O(n), assumes a nonempty finite nonnegative positive-total weight array, and falls back to the last item after the scan. It does not validate those assumptions. An exact zero uniform can select a leading zero-weight item because the test is <=0."),
        section("Seeding and concurrency", "Default engines have deterministic state. from_seed uses the documented engine-specific conversion/mixing, while hardware::seed_from_hw attempts four hardware words on native targets and falls back to clocks, stack address, optional timestamp counter, and SplitMix64 when unavailable. It returns xoshiro256ss and does not report whether fallback occurred.",
                "Native rdrand64/rdseed64 expose bool success, with compile-time ISA selection and bounded instruction retries; unavailable targets return false. The wasm delegate instead calls the host random hook and panics when seed acquisition fails or produces an all-zero seed. No runtime CPU detection or universal hardware-entropy guarantee is provided.",
                "Engine mutation is unsynchronized. Separate copied engines can be used independently but initially produce duplicate streams; sharing one engine requires coordination. Distribution objects own immutable parameters except dirichlet_dist<F,0>'s borrowed alpha pointer. Input/output intervals must remain valid and nonconflicting for all noexcept calls; throwing element operations in shuffle violate noexcept."),
    ],
    "types": [
        datatype("micron::math::rng::splitmix64", "math/rng/engines.hpp",
                 "An eight-byte deterministic generator storing one u64 state and applying increment/xor/multiply mixing.",
                 section("Operations and related compact engines", "Default seed is zero; explicit construction/from_seed take u64. next/next64 advance by a fixed odd increment and mix the new state. generate unrolls eight independent offset states while preserving the scalar sequence. Public state s can be saved/restored directly.",
                         table=table(["Type", "Construction and recurrence"], [
                             ["`lcg64`", "Default zero or explicit/from_seed u64; s=A*s+C modulo 2^64. generate unrolls four affine powers; next32 returns high 32 bits"],
                             ["`mwc64`", "Default fixed hi/lo or explicit raw pair; from_seed uses SplitMix64 and normalizes carry/range/nonzero state for each component. Direct pair construction does not normalize"],
                         ])),
                 section("Cost and lifetime", "Each step is O(1); generate costs O(n). All state is embedded and public, so copy/move are constant-size value copies without allocations or borrowed storage. These compact types suit deterministic streams and seed expansion; they do not independently initialize from operating-system entropy."),
                 covers=["micron::math::rng::lcg64", "micron::math::rng::mwc64", "micron::math::rng::rng_concept"]),
        datatype("micron::math::rng::xoshiro256ss", "math/rng/engines.hpp",
                 "A four-u64-state xor/shift/rotate generator with star-star output and fixed jump operations.",
                 section("Variants and seeding", "xoshiro256ss returns u64; xoshiro128ss uses four u32 words and returns u32. Both default to fixed nonzero states. Raw four-word constructors replace an all-zero state with one fixed nonzero word. from_seed expands a u64 using SplitMix64; direct later writes to public s can bypass the zero-state guard.",
                         "next/next64 on the 256-bit type perform one transition. The 128-bit type's next32 performs one and next64 concatenates low then high consecutive results. generate uses four local-state transitions per unrolled block, preserving scalar order. Both jump and long_jump apply fixed polynomial constants, evaluating 256 scalar transitions on xoshiro256ss or 128 on xoshiro128ss before replacing the state."),
                 section("Storage and use", "State sizes are asserted as 32 and 16 bytes. Step and fixed jump cost are O(1) with respect to requested stream length; generate is O(n). Copies duplicate streams, so distinct seeds or deliberate jump partitioning are required when duplicate sequences are unwanted. No element pointers or external storage are retained."),
                 covers=["micron::math::rng::xoshiro128ss"]),
        datatype("micron::math::rng::pcg64", "math/rng/engines.hpp",
                 "A 128-bit-state linear congruential generator with a 128-bit odd stream increment and 64-bit xor/rotate output.",
                 section("Construction and stepping", "Public state/inc occupy 32 bytes. The raw (u128 state,u128 increment) constructor forces increment odd but otherwise keeps the supplied state. make(u64 seed,u64 stream=1) constructs a stream increment and performs the seeding transitions. Default construction is deterministic.",
                         "next emits the permutation of the old state before advancing. generate unrolls four affine state powers and emits the same scalar sequence, then stores the final state. O(1) fixed-width arithmetic per draw and O(n) generation exclude target-specific costs of 128-bit multiplication. Copies/moves duplicate state without allocation.")),
        datatype("micron::math::rng::mt19937", "math/rng/engines.hpp",
                 "A 624-word Mersenne Twister state with 32-bit tempering output and an index into its current block.",
                 section("Storage and API", "The public mt[624] array and idx occupy approximately 2.5 KiB. Default seed is 5489; explicit seed_with accepts u32, and from_seed(u64) truncates to u32. next/next32 twist when the block is exhausted, then temper and increment the index. next64 concatenates two consecutive u32 values.",
                         "generate(out,n) emits tempered values in block spans. The separate no-argument generate() means twist(), advancing/replacing the current state block rather than filling a user buffer. twist costs O(624); next is amortized O(1), generation O(n+624) over refills, and copying copies all state words. No heap allocation is used.")),
        datatype("micron::math::rng::dist::mkbits::ziggurat_tables", "math/rng/ziggurat.hpp",
                 "Embedded 256-entry widths, densities, and integer acceptance thresholds for normal sampling.",
                 section("Construction and sampling", "The record contains f64 w[256], f64 f[256], and u64 k[256], plus fixed R/V constants. build_ziggurat is consteval and constructs the single constinit const table, checking its closing region during constant evaluation.",
                         "normal_ziggurat selects a region/sign/magnitude from a next64 draw. Most accepted rectangles return after one draw; rejected rectangles use exponential-density tests, and the base tail uses open-uniform logarithmic rejection. Work is trial-dependent with no worst-case retry limit. The sampler multiplies by sigma and adds mu without validating either; tables are read-only and shared, while engine state remains caller-owned.")),
        datatype("micron::math::rng::dists::poisson_dist", "math/rng/dists.hpp",
                 "An owning cached Poisson-rate sampler with a small-rate CDF table and transformed-rejection constants.",
                 section("Algorithms and parameter conventions", "poisson_dist<I=i64>(lambda) caches f64 parameters. Positive finite lambda<10 precomputes 32 cumulative probabilities; sampling compares one uniform against the table and extends the tail by recurrence only when needed. Larger rates use the shared transformed-rejection engine with logarithms/lgamma. Invalid/nonfinite/nonpositive rates sample zero, and mean()/variance() return zero.",
                         "The object owns fixed arrays/scalars, without heap allocation or an engine. Construction costs O(32) for the table. Sampling is fixed-table work plus possible tail extension, or unbounded rejection trials; no saturation protects I conversion. The one-shot dist::poisson reconstructs the cache and uses a product-of-uniforms algorithm for small rates, so engine consumption differs.")),
        datatype("micron::math::rng::dists::gamma_dist", "math/rng/dists.hpp",
                 "An owning gamma shape/scale parameter cache using normal-based rejection sampling.",
                 section("Gamma and beta family", "gamma_dist<F> aliases __gammadist<F>; beta_dist<F> aliases __betadist<F>. Gamma stores shape, scale, rejection constants, inverse shape, and log normalization. Shape below one is sampled through a boosted shape followed by an open-uniform power factor. Sampling, pdf, and cdf reject nonfinite/nonpositive shape or scale with NaN; mean/variance directly calculate the parameter formulas.",
                         "beta_dist owns alpha/beta, log-beta normalization, and two gamma samplers. A draw returns X/(X+Y) from unit-scale gamma draws. Its sampling/pdf/cdf validate positive finite parameters; boundary pdf returns zero, finite limits, or infinity according to the endpoint exponents. Cdf uses the bounded-iteration incomplete-beta helper. The X+Y denominator has no special underflow correction.",
                         "Both types expose value_type=F, const operator()(Rng&), pdf/cdf, mean, and variance. Gamma additionally exposes shape()/scale(). They own only fixed scalar caches, allocate nothing, and copy in O(1). Sampling has unbounded rejection trials; cdf uses up to 200 helper iterations without convergence reporting."),
                 covers=["micron::math::rng::dists::beta_dist"]),
        datatype("micron::math::rng::dists::chi2_dist", "math/rng/dists.hpp",
                 "An owning chi-square distribution implemented by a gamma(k/2,2) sampler.",
                 section("Related ratio distributions", table=table(["Type", "Owned parameters and draw", "Additional methods/limits"], [
                     ["`chi2_dist<F>`", "Degrees of freedom k and gamma cache", "pdf/cdf, mean=k, variance=2k; pdf/cdf return zero for x<=0"],
                     ["`student_t_dist<F>`", "nu, reciprocal nu, and chi-square cache; normal/sqrt(chi-square/nu)", "pdf/cdf; mean NaN for nu<=1, variance infinity for 1<nu<=2 and NaN below"],
                     ["`f_dist<F>`", "Two degrees/reciprocals and chi-square caches; ratio of normalized chi-square draws", "pdf/cdf; mean NaN when d2<=2, variance infinity when d2<=4"],
                 ])),
                 section("Cost and domain", "All own fixed scalar/cache values and borrow only the supplied engine during a call. Sampling inherits gamma/normal rejection cost and checks selected nonpositive degrees; pdf/cdf/moment guards are less comprehensive than sampling guards. The implemented chi-square pdf returns zero at x==0 even when the mathematical boundary density is nonzero/infinite. CDFs use incomplete gamma/beta approximations with the module's iteration cap and no status."),
                 covers=["micron::math::rng::dists::student_t_dist", "micron::math::rng::dists::f_dist"]),
        datatype("micron::math::rng::dists::lognormal_dist", "math/rng/dists.hpp",
                 "An owning lognormal parameter pair sampling exp(normal(mu,sigma)).",
                 section("Inverse-transform family", table=table(["Type", "Parameters and sample", "Analytic methods"], [
                     ["`lognormal_dist<F>`", "mu=0, sigma=1; exponential of a normal draw", "pdf/cdf/ppf, mean, variance; ppf uses rational normal quantiles"],
                     ["`weibull_dist<F>`", "lambda scale, k shape; lambda*(-log(open_uniform))^(1/k)", "pdf/cdf/ppf, gamma-based mean/variance; ppf clamps p<=0 to zero and p>=1 to infinity"],
                     ["`cauchy_dist<F>`", "x0=0, gamma=1; x0+gamma*tan(pi*(open_uniform-.5))", "pdf/cdf/ppf; mean/variance return NaN"],
                 ])),
                 section("Domain and complexity", "Weibull sampling rejects nonpositive scale/shape with NaN. Cauchy sampling returns x0 for zero scale and NaN for negative scale; its analytic methods do not share that special case. Lognormal sampling passes sigma through without validation. Parameter constructors store/cache values without throwing a domain error.",
                         "Each draw is O(1) analytic/transcendental work plus normal rejection for lognormal. These objects allocate nothing and copy in O(1). pdf/cdf/ppf parameter validation is not uniform; supply finite valid parameters and the intended probability domain. There is no configurable quantile-accuracy tolerance."),
                 covers=["micron::math::rng::dists::weibull_dist", "micron::math::rng::dists::cauchy_dist"]),
        datatype("micron::math::rng::dists::geometric_dist", "math/rng/dists.hpp",
                 "An owning probability cache for the number of failures before one success.",
                 section("Sampling and methods", "geometric_dist<I=i64,F=f64> stores p, reciprocal log(1-p), and a p==.5 flag. It samples zero for p>=1 and numeric_limits<I>::max() for p<=0. At p=.5 it counts zero bits with trailing-zero queries over next64 words; otherwise it uses floor(log(open_uniform)/log(1-p)).",
                         "pmf/cdf return zero for negative k, while mean/variance directly calculate (1-p)/p and (1-p)/p². Those methods do not reproduce the sampler's invalid-parameter sentinels. Ordinary inverse sampling is O(1) transcendental work; p=.5 can consume an unbounded run of all-zero words. Conversion/count overflow is not checked. State is fixed owned scalars, with no retained engine.")),
        datatype("micron::math::rng::dists::binomial_dist", "math/rng/dists.hpp",
                 "An owning trial/probability sampler dispatching among compact CDF, inversion, population-count, and transformed-rejection algorithms.",
                 section("Dispatch", "The constructor caches n,p and symmetry r=min(p,1-p). Sampling returns zero for n<=0, nonfinite p, or p<=0, and n for p>=1. For p=.5 it counts set bits in n random bits, using O(ceil(n/64)) words. Other probabilities use a 16-entry CDF when n<=16; small expected count n*r<10 uses recurrence inversion; remaining cases use BTRD rejection and mass-ratio/logarithmic acceptance.",
                         "The result is reflected n-k when p>.5. Inversion can visit up to n masses; BTRD has no fixed rejection bound. pmf uses log-gamma and cdf incomplete beta with the module's fixed helper cap. mean/variance directly return n*p and n*p*(1-p). The object owns fixed arrays/scalars and allocates nothing. I and F must represent relevant counts/intermediates; no count-overflow status is returned.")),
        datatype("micron::math::rng::dists::negative_binomial_dist", "math/rng/dists.hpp",
                 "An owning gamma-Poisson mixture for failures before a positive real success shape.",
                 section("Parameters and behavior", "negative_binomial_dist<I=i64,F=f64>(r,p) owns r,p and a gamma(r,(1-p)/p) cache. Sampling returns zero for r<=0, p<=0, p>1, or p==1; otherwise a gamma draw supplies the Poisson rate. This allows real positive shape rather than requiring integral success count.",
                         "pmf/cdf, mean=r*(1-p)/p, and variance=r*(1-p)/p² use analytic formulas without identical invalid-parameter guards. Sampling costs one gamma and one Poisson draw, including their unbounded rejection/tail work. All state is fixed embedded cache; supplied engine state is borrowed and advanced only during the call.")),
        datatype("micron::math::rng::dists::dirichlet_dist", "math/rng/dists.hpp",
                 "A gamma-normalized vector distribution with either owned compile-time arity parameters or borrowed runtime arity parameters.",
                 section("Storage variants", "dirichlet_dist<F,K> for K>0 owns alpha[K], K gamma caches, and a log-normalization value. Construction copies exactly K alpha elements; its optional count argument is ignored and does not reduce the required input extent. size() is static K. Changing the original alpha array does not change the object.",
                         "dirichlet_dist<F,0> stores const F* alpha and a runtime count, without copying parameters or allocating. It constructs gamma caches anew for every sampled coordinate and recomputes log normalization for pdf. The alpha array must remain valid through all calls; changing it changes subsequent behavior. Copying this specialization copies only pointer/count."),
                 section("Sampling, density, and cost", "operator()(Rng&,F* out) writes one gamma draw per coordinate, sums them, and divides by the sum only when it is positive. It does not retry a zero/NaN sum. Parameters should be positive finite and output must hold size() elements. pdf assumes positive simplex coordinates; it does not test nonnegativity, unit sum, or dimensions.",
                         "Construction/copy/storage are O(K) for fixed arity and O(1) for borrowed arity. A sample costs K gamma draws plus O(K) normalization; pdf is O(K) logarithmic/gamma work, with fixed arity caching its normalization. No per-call container allocation occurs. Overlap between borrowed alpha and output can change parameters during a dynamic draw and must be avoided.")),
    ],
    "sources": ['math/rng.hpp',
 'rng.hpp',
 'math/rng/engines.hpp',
 'math/rng/dist.hpp',
 'math/rng/dists.hpp',
 'math/rng/ziggurat.hpp',
 'math/rng/hardware.hpp',
 'math/rng/fill.hpp',
 'math/__asm/rdrand.hpp',
 'bits/wasm/math/random.hpp'],
}}
