"""Fourier transforms and related reusable plans, reviewed in full."""


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


GUIDES = {"fft": {
    "summary": "Complex and packed-real discrete Fourier transforms, DCT/DST transforms, fixed-kernel convolution, frequency/shift helpers, and reusable serial or executor-based parallel workspaces. Plans own precomputed coefficients; execution borrows caller input, output, and scratch storage.",
    "entry_overrides": {
        "micron::math::fft::rfft(input, n)": "Allocates a plan/workspace and returns floor(n/2)+1 complex nonnegative-frequency bins from n real samples, using backward normalization by default.",
        "micron::math::fft::irfft(input, n)": "Consumes floor(n/2)+1 packed complex bins and reconstructs n real samples. n is the original real length, including its parity.",
        "micron::math::fft::descriptor<T>": "Owns dimensions, selected axes, signed input/output layouts, batches, transform domain, normalization, and extra forward/backward scale factors.",
        "micron::math::fft::plan<T>": "Owns a validated descriptor and precomputed mixed-radix/Bluestein engines; repeated execute calls use supplied workspace without constructing a new plan.",
        "micron::math::fft::workspace<T>": "Owns allocator_serial-backed complex scratch elements; workspace_view borrows that storage, and plan.workspace_size reports the required element count.",
    },
    "sections": [
        section("Interface selection", "All public names are in `micron::math::fft`. `<micron/math/fft.hpp>` includes complex/real plans, convolution, and helpers. The parallel_workspace adapter requires `<micron/math/fft/parallel.hpp>` separately.",
                table=table(["Facility", "Operation and storage"], [
                    ["`transform`, `ifft`, `rfft`, `irfft`", "One-shot transforms returning owned buffers; construct a new plan and scratch allocation on each call"],
                    ["`plan<F>`", "Repeated complex or packed-real transforms, arbitrary rank/axes, signed strides, offsets, and batches"],
                    ["`r2r_plan<F>`, `dct`, `dst`, `real_transform`", "Real-to-real cosine/sine transforms I-IV, with reusable or one-shot execution"],
                    ["`convolution_plan<F>`, `convolve`, `correlate`", "Row-major linear/circular convolution or correlation; a plan owns one pretransformed fixed kernel"],
                    ["`workspace`, `workspace_view`, `parallel_workspace`", "Owned serial scratch, borrowed scratch, or scratch plus executor job state"],
                    ["`fftfreq`, `rfftfreq`, `fftshift`, `ifftshift`, `next_fast_len`", "Frequency coordinates, index rotation, and smooth transform-length selection"],
                ])),
        section("Transform convention and normalization", "Forward complex transforms use exp(-2*pi*i*k/n); backward transforms reverse that phase. Packed real spectra include DC and the nonnegative bins through floor(n/2). For multidimensional real transforms, only the last selected axis is packed; inverse reconstruction mirrors the selected transformed coordinates and conjugates them.",
                table=table(["normalization", "Forward factor", "Backward factor"], [
                    ["`backward` (default)", "1", "1/N"],
                    ["`none`", "1", "1"],
                    ["`forward`", "1/N", "1"],
                    ["`ortho`", "1/sqrt(N)", "1/sqrt(N)"],
                ])),
        section("Algorithms, precision, and complexity", "Each smooth one-dimensional length is factored into radix-2/3/4/5/7 stages, with radix-8 leaves. Other prime factors use Bluestein convolution padded to next_fast_len(2*n-1). Twiddles use integer quadrant reduction followed by fixed polynomial-series evaluation. Arithmetic is floating point and has no user-specified accuracy or convergence tolerance.",
                "For one smooth line, execution costs O(n log n); a Bluestein line costs O(m log m), m=next_fast_len(2*n-1), with two m-element scratch spans. Separable multidimensional execution applies an engine to every line of every selected axis, plus input/output and axis gather/scatter work. Let B be batch count, V full logical volume, and n_a each selected axis length: smooth arithmetic is O(B*V*sum(log n_a)). Noncontiguous indexing adds rank-dependent coordinate calculations.",
                "Planning stores stage metadata and O(sum(engine lengths)) twiddles/chirps/kernels; Bluestein setup also computes the transformed kernel. Execution with an existing workspace performs no container allocation. One-shot wrappers allocate owned outputs and construct plan/workspace state. SIMD packet widths are chosen at compile time, with scalar tails; long double uses the scalar packet, ARMv7 double remains scalar, and MICRON_NO_SIMD selects generic packets."),
        section("Return values and buffers", "Plan construction checks shape products, transform kinds, axes, layout rank, stride representability, and scratch byte sizes, reporting invalid_argument or length_error. Execute methods are noexcept and return a signed max_t: nonnegative logical output bytes, -22 for a domain/direction/null-pointer mismatch, or -105 for insufficient/invalid workspace. Zero batches return zero without reading sample buffers. One-shot wrappers convert negative execution status to invalid_argument.",
                "Input/output capacities are not passed to execute and are not validated. Signed strides/offsets are element counts; the supplied pointer must make every accessed address valid, including negative offsets. Real-domain strides count F elements and spectrum strides count complex pairs. Layout overlap/injectivity and padded in-place capacity belong to the caller. Input/output equality is supported by scratch staging, but arbitrary partial overlap is not a uniform fast-path contract; workspace must be separate from sample buffers."),
        section("Helpers", "next_fast_len finds the smallest 2/3/5/7-smooth length at least n by enumerating prime-power combinations, returning one for n<=1; it rejects sizes beyond its checked range. fftfreq returns n signed frequency bins in ordinary FFT order; rfftfreq returns floor(n/2)+1 nonnegative bins. Both require positive spacing and n>0, and cost O(n) output work.",
                "Pointer-to-pointer fftshift/ifftshift require disjoint intervals and rotate by ceil(n/2)/floor(n/2) respectively in O(n). In-place overloads allocate a temporary. Multidimensional shift validates selected distinct axes and returns a new buffer; its index computation costs O(volume*rank*(selected_axes+1)). Returned buffers own their elements."),
        section("Lifetime and concurrency", "Plans own configuration/coefficients and borrow only sample/scratch buffers during execution. A const plan can be reused concurrently with distinct workspaces and nonconflicting outputs. A workspace_view, split descriptor, or configuration reference does not retain an owner. Moving/replacing/destroying its owner invalidates borrowed association; sample buffers and executors must remain alive until execution returns."),
    ],
    "types": [
        datatype("micron::math::fft::descriptor", "math/fft/common.hpp",
                 "An owning transform configuration with dimensions, axes, layouts, batch count, domain, and scaling.",
                 section("Fields and policy types", "shape is a buffer<usize> of positive dimensions; constructors accept one length, an initializer list, or a dimension buffer. axes is empty to select all axes in row-major order, otherwise it lists distinct transformed axes in the supplied order. input/output are layout records; batches defaults to one, kind to complex, norm to backward, and forward_scale/backward_scale to one.",
                         table=table(["Type", "Values and meaning"], [
                             ["`layout`", "Owned signed strides, signed offset, and signed batch distance; empty strides choose row-major, distance zero chooses contiguous logical volume"],
                             ["`direction`", "forward, backward"],
                             ["`domain`", "complex, real_forward, real_backward"],
                             ["`normalization`", "backward, none, forward, ortho"],
                             ["`fft_scalar`", "A supported complex scalar of size four/eight, or long double"],
                             ["`buffer<T>`", "vector<T,allocator_serial<>,false>; owns dynamic contiguous values"],
                         ])),
                 section("Validation and cost", "Descriptor construction stores values; the plan performs full validation and fills default layouts/axes into its own copy. Strides cannot be zero for a dimension/batch count greater than one, but valid distinct strides can still overlap coordinates. Offsets may be negative. Scale factors are not checked for finiteness. Copy cost is proportional to rank and selected axes; the descriptor retains no sample pointers."),
                 covers=["micron::math::fft::layout", "micron::math::fft::direction", "micron::math::fft::domain", "micron::math::fft::normalization", "micron::math::fft::fft_scalar", "micron::math::fft::buffer"]),
        datatype("micron::math::fft::plan", "math/fft/plan.hpp",
                 "An owning reusable complex/packed-real transform plan with per-axis engines and precomputed coefficients.",
                 section("Construction and dimensions", "plan(n,norm) constructs a one-axis complex plan. plan(descriptor) validates positive nonempty shape, domain/normalization, distinct in-range axes, signed layouts, and checked size products. It builds one engine per transformed axis. Real even lengths use a half-length complex engine plus real recombination; odd lengths use a full complex engine.",
                         "configuration(), input_shape(), and output_shape() return borrowed const references to completed plan state. input_size/output_size count logical elements over all batches, excluding stride gaps. real_forward shortens the last selected output axis to n/2+1; real_backward shortens that input axis. Normalization N is the product of selected full axis lengths, excluding batches and untransformed axes."),
                 section("Execution overloads", "Complex execution accepts interleaved complex pointers or split_input/split_output records plus an explicit direction. Real forward accepts F input and complex/split output; real backward accepts complex/split input and F output. execute_inplace(F*) writes packed complex pairs into scalar storage or reconstructs real values without creating complex objects that alias F storage.",
                         "All variants stage/gather data in supplied scratch except selected contiguous one-dimensional fast paths. Those paths specifically handle exact input/output equality; they do not establish arbitrary partial-overlap safety. workspace_size(workers) reports complex elements, workspace_bytes reports bytes, and zero workers is rejected. A generic serial plan needs full-volume batch staging plus 5*maximum_axis_length and the largest engine scratch per worker."),
                 section("Ownership and invalidation", "Copies duplicate descriptor and coefficient vectors; moves transfer vector storage. Execution does not mutate plan coefficients. Returned configuration/shape references and any pointers into them are invalidated by replacing or destroying the plan. The plan does not retain the input, output, or supplied workspace after a completed synchronous call.")),
        datatype("micron::math::fft::workspace", "math/fft/common.hpp",
                 "An owning default-allocator buffer of complex scratch elements for transform execution.",
                 section("Workspace and split-buffer records", "workspace(elements) allocates that many complex values; workspace(plan) uses plan.workspace_size(). size reports element count and view returns workspace_view<F>{data,size}. The view is a borrowed pointer/count descriptor, without resizing, allocation, or lifetime retention. A caller can provide its own suitably sized complex buffer through workspace_view.",
                         "split_input<F> stores const F* real and imag; split_output<F> stores mutable F* real and imag. They describe two separate arrays with the plan's complex-element indexing convention. Default pointers are null and execute validates nullness when batches are nonzero. Copying these records is O(1); copying an owning workspace duplicates its elements.",
                         "Views become invalid when their workspace is replaced, moved, or destroyed. One workspace is mutable scratch and cannot serve overlapping executions. Capacity must match the particular plan and worker count, rather than only the input length."),
                 covers=["micron::math::fft::workspace_view", "micron::math::fft::split_input", "micron::math::fft::split_output"]),
        datatype("micron::math::fft::r2r_plan", "math/fft/real.hpp",
                 "An owning separable real-to-real cosine/sine transform plan with an FFT embedding per selected axis.",
                 section("Kinds and inverse pairs", "r2r_kind enumerates dct_i, dct_ii, dct_iii, dct_iv, dst_i, dst_ii, dst_iii, dst_iv. A one-dimensional constructor accepts n and one kind; the descriptor constructor requires one kind per selected axis and kind==domain::complex in the descriptor. Lengths must be positive, and DCT-I requires n>=2.",
                         "Backward direction exchanges DCT-II with III and DST-II with III; kinds I and IV are self-paired. Each engine embeds reflected/antisymmetric or phased samples in a complex transform. Logical normalization lengths are 2*(n-1) for DCT-I, 2*(n+1) for DST-I, and 2*n otherwise. Type-IV embeddings use 8*n complex points. Ortho additionally adjusts endpoint coefficients according to the kind.",
                         "execute accepts F input/output and workspace_view and returns logical output bytes or the common negative statuses. The one-shot real_transform/dct/dst wrappers allocate plans, scratch, and outputs; dct/dst type arguments must be 1..4."),
                 section("Complexity and storage", "Each axis costs its embedding engine's O(m log m) line transform work plus embedding/gather/scatter. Plan state owns phase arrays and FFT coefficients. Scratch includes batch-volume complex staging, two maximum-length line spans, and the largest embedded engine workspace per worker. size() counts all logical real elements. Plan and scratch lifetimes follow the complex-plan family."),
                 covers=["micron::math::fft::r2r_kind"]),
        datatype("micron::math::fft::convolution_plan", "math/fft/convolution.hpp",
                 "An owning fixed-kernel convolution/correlation plan with a precomputed kernel spectrum.",
                 section("Descriptor and modes", "convolution_descriptor owns input_shape, kernel_shape, optional period, mode=full, and correlation=false. Input/kernel ranks must agree and be nonempty. I/O is contiguous row-major. A nonempty kernel pointer is consumed during construction, copied into padded transform storage, and not retained.",
                         table=table(["convolution_mode", "Output/crop per axis"], [
                             ["`full`", "n+m-1 samples, uncropped"],
                             ["`same`", "n samples beginning at floor((m-1)/2) of the full result"],
                             ["`valid`", "n-m+1 samples beginning at m-1; requires n>=m on every nonempty axis"],
                             ["`circular`", "The explicitly supplied positive period per axis; input and kernel dimensions must not exceed it"],
                         ])),
                 section("Implementation and errors", "Linear modes pad each axis to next_fast_len(n+m-1). Linear correlation reverses/conjugates the kernel before transforming; circular correlation conjugates its spectrum. The short convolve/correlate length overload does not supply circular periods, so circular use requires the full descriptor.",
                         "A zero input/kernel dimension creates an empty plan/result, subject to remaining mode/period validation; execute then returns zero without dereferencing pointers. Otherwise execute embeds input, performs forward transform, pointwise multiplication, inverse transform, and crop. It returns output bytes, -22 for null sample pointers, or -105 for workspace failure. output_shape/output_size describe the cropped result."),
                 section("Complexity and lifetime", "Let P be padded transform volume. Planning computes one kernel transform; each execution computes two transforms plus O(P) multiplication/embedding and output cropping. Smooth multidimensional transform work is O(P*sum(log padded_axis)), with Bluestein costs where required. Plan storage includes O(P) kernel spectrum and FFT coefficients; scratch is two P-element arrays plus the FFT workspace.",
                         "The fixed kernel spectrum belongs to the plan and can be reused after the original kernel storage changes. Copies own independent coefficients. Distinct execution workspaces permit concurrent reuse of one const plan. convolve/correlate wrappers rebuild this state on each call."),
                 covers=["micron::math::fft::convolution_descriptor", "micron::math::fft::convolution_mode"]),
        datatype("micron::math::fft::parallel_workspace", "math/fft/parallel.hpp",
                 "An owning transform workspace and reusable job array executing through a caller-supplied executor.",
                 section("Executor contract", "Construction takes a plan and worker count in 1..0x7fffffff and allocates plan.workspace_size(workers) complex elements plus job records. Copying is deleted. The executor must provide noexcept bool-like submit(void(*)(void*) noexcept,void*). An accepted job must eventually run; false submission must not retain the job. Rejected submissions and the final partition execute on the calling thread.",
                         "Workers receive distinct scratch slices. The runner partitions lines or top-level transform work, uses atomic completion counts, and waits with futex helpers before returning or advancing an axis. A single large smooth transform parallelizes above the engine's 1024-point threshold. This adapter does not construct or own a thread pool.",
                         "Overloads execute complex/real plans, split complex buffers, scalar in-place real storage, r2r plans, and convolution plans. The chosen input/output types must match the plan domain. Output/error conventions match serial execution."),
                 section("Complexity and concurrency", "Parallel execution performs the same arithmetic/gather/scatter work, plus submission/barrier overhead; worker count does not itself guarantee a speedup or available threads. Storage adds one scratch slice and job record per worker. The same parallel_workspace cannot run overlapping calls because jobs, scratch, and completion state are shared. Executor and sample buffers must remain valid until execute returns; all accepted jobs have completed at that point.")),
    ],
    "sources": ['math/fft.hpp',
     'math/fft/arch/packet_amd64.hpp',
     'math/fft/arch/packet_arm32.hpp',
     'math/fft/arch/packet_arm64.hpp',
     'math/fft/arch/packet_generic.hpp',
     'math/fft/arch/packet_neon.hpp',
     'math/fft/common.hpp',
     'math/fft/convolution.hpp',
     'math/fft/engine.hpp',
     'math/fft/helpers.hpp',
     'math/fft/kernels.hpp',
     'math/fft/packet.hpp',
     'math/fft/parallel.hpp',
     'math/fft/plan.hpp',
     'math/fft/real.hpp'],
}}
