"""Geometric primitives, homogeneous transformations, and alignment, reviewed in full."""


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


GUIDES = {"geometry": {
    "summary": "Fixed-size geometric primitives, component translations/scales, homogeneous 2D/3D transforms, graphics projection/view matrices, and three-dimensional point-set alignment. Values own embedded vectors or matrices; operations borrow input values without retaining addresses or allocating shape-dependent storage.",
    "entry_overrides": {
        "micron::math::geometry::aligned_box": "Owns minimum and maximum corners of a closed axis-aligned box in 2 through 16 dimensions; any inverted coordinate marks it empty.",
        "micron::math::geometry::hyperplane": "Owns normal and offset for normal·x+offset=0; signed_distance and projection require a unit normal for their geometric interpretation.",
        "micron::math::geometry::look_at": "Builds a 3D world-to-eye isometry using the requested handedness, eye, target, and nonparallel up vector; degenerate inputs produce a degenerate matrix without a status result.",
    },
    "sections": [
        section("Facilities and selection", "Include `math/geometry.hpp` for the complete family. Dimensional extents are compile-time parameters, not dynamic capacities. Vector-backed primitives inherit vec<F,Dim>'s Dim*sizeof(F)<=128 limit in addition to 2<=Dim<=16, so the largest available dimension depends on the scalar type.", table=table(["Facility", "Dimensions / use", "Work and stored data"], [
            ["aligned_box", "2..16; bounds, containment, intersection, corner enumeration", "Two embedded vectors; O(D) per operation"],
            ["hyperplane; parametrized_line", "2..16; plane equation, projection, line distance/intersection", "Embedded vector/scalar state; O(D) per operation"],
            ["translation; scaling", "2..16; simple component transforms", "One vector; O(D) application/composition/inverse"],
            ["transform", "2 or 3; isometry, affine, or projective homogeneous map", "One (D+1)² matrix; O(D²) application, O(D³) dense composition"],
            ["perspective_projection; orthographic_projection; frustum; look_at", "3; view/clip-space matrix construction", "Fixed 4x4 output; constant work"],
            ["unit_orthogonal", "2..16; one perpendicular unit vector for a nonzero input", "O(D) work; no complete orthonormal-basis construction"],
            ["umeyama", "3xN paired source/destination points", "O(N) centroid/covariance work and one fixed 3x3 SVD"],
        ])),
        section("Conventions and validity", "Matrices are stored row-major and act on column-coordinate vectors. Translation occupies the last column. A product a*b applies b first, then a. Transform modes choose algorithms at compile time and do not validate the stored matrix: an isometry label does not prove orthonormality, and an affine label does not repair a projective bottom row.",
                "These noexcept interfaces return numeric values, without a common error/status wrapper. Finite usable inputs, nonzero denominators, and the documented geometric invariants remain caller requirements. Degenerate values can return zeros, an unchanged numerator, an identity fallback, or nonfinite arithmetic depending on the operation; those conventions are specified individually."),
        section("Ownership, lifetime, and invalidation", "Primitive and transform objects own only embedded vec/mat/scalar members, with value copying and no retained pointers. Mutation changes the same fixed storage and does not reallocate it; destroying the object ends member-address validity. There are no iterators or cached mutable lookup fields. Concurrent immutable operations on a shared valid value use local scratch; concurrent mutation requires external synchronization.",
                "umeyama borrows two dynmat objects for the duration of the call and returns an independent fixed transform. Geometry does not resize or retain those matrices. Shared vector/matrix numerical kernels may select compile-time architecture paths, so exact rounding can differ by build."),
        section("Orthogonal vectors", "unit_orthogonal(v) chooses a coordinate axis with the smallest absolute component for 3D, crosses v with it, and normalizes a nonzero result. For other supported dimensions it uses the largest-magnitude coordinate and its successor to form a two-coordinate perpendicular vector, then normalizes. A zero input yields zero, not an arbitrary unit vector or a failure indication. Cost is O(D) and automatic scratch O(D)."),
        section("Point-set alignment", "umeyama(src,dst,with_scaling=true) expects both matrices to have three rows and equal nonzero column counts, with paired points in columns. Invalid shapes or N=0 return an identity affine transform. It computes centroids, destination/source cross covariance, source variance, and a 3x3 SVD; a determinant sign correction seeks a proper rotation. Rank-deficient left singular vectors are completed by normalization, a fallback coordinate axis, and a cross product.",
                "With scaling enabled, the signed singular-value trace is divided by positive source variance; zero variance leaves scale=1. Translation maps the source centroid to the destination centroid. with_scaling=false retains unit scale. Work is O(N) plus the fixed-size SVD, with O(1) auxiliary storage relative to N; the routine does not allocate point copies. It returns neither residual, rank, degeneracy, nor SVD-convergence status, so identity and other fallbacks cannot be distinguished by a status field."),
    ],
    "types": [
        datatype("micron::math::geometry::aligned_box", "math/geometry/aligned_box.hpp",
                 "Owns the minimum and maximum corners of a closed axis-aligned box in 2 through 16 dimensions.",
                 section("Members and operations", "min_corner and max_corner are embedded vec<F,Dim>. empty() sets each coordinate to min=1,max=-1; is_empty() returns true when any minimum exceeds its maximum. This explicit empty value differs from a value-initialized zero-sized box. Boundary points and touching boxes count as contained/intersecting. contains(empty_box) returns true; intersects returns false when either box is empty.",
                         "extend(point) replaces an empty box with that point or enlarges each coordinate bound; extend(box) ignores an empty argument. Both mutate and return *this. center() averages corners, diagonal() subtracts them, and volume() returns zero for emptiness or multiplies side lengths. center/diagonal do not specially normalize empty boxes. corner(i) selects minimum/maximum per bit i; bits above Dim are ignored, and an empty box still produces its stored corners.",
                         "All operations and copies cost O(Dim); there is no heap allocation or spatial index. Finite coordinates are required for ordinary geometric semantics: comparison-based contains/is_empty do not comprehensively reject NaNs.")),
        datatype("micron::math::geometry::hyperplane", "math/geometry/hyperplane.hpp",
                 "Owns a normal vector and offset for the equation normal·x+offset=0 in 2 through 16 dimensions.",
                 section("Construction and invariants", "through(point,normal) stores the supplied normal unchanged and offset=-normal·point. It does not normalize. signed_distance(point) returns normal·point+offset, and projection(point) returns point-distance*normal. These are Euclidean signed distance and orthogonal projection only when normal has unit length; scaling the plane equation without normalizing changes their interpretation.",
                         "through3(a,b,c), available in 3D, forms and normalizes (b-a) cross (c-a) when its norm is positive. Collinear/coincident points produce a zero normal and zero offset, with no invalid-plane status. Aggregate construction permits arbitrary normal/offset. Construction, distance, projection, and copy cost O(Dim), with embedded storage and no allocation.")),
        datatype("micron::math::geometry::parametrized_line", "math/geometry/parametrized_line.hpp",
                 "Owns origin and direction for an unbounded line origin+t*direction in 2 through 16 dimensions.",
                 section("Operations and degeneracy", "point_at(t) forms the affine point. distance(point) divides the projection dot product by direction squared norm, then measures the perpendicular residual; a zero direction uses parameter zero and returns distance to origin. Unit direction is not required.",
                         "intersection_param(plane) returns -plane(origin)/(normal·direction). An exactly zero denominator returns zero for both a parallel disjoint line and a line lying in the plane. intersection_point therefore returns origin in those cases, without distinguishing absence, coincidence, or a true t=0 intersection. There is no near-parallel tolerance. All operations/copies cost O(Dim), with O(Dim) automatic scratch in distance.")),
        datatype("micron::math::geometry::translation", "math/geometry/translation.hpp",
                 "Owns one displacement vector for componentwise translation in 2 through 16 dimensions.",
                 section("Operations and selection", "t is an embedded vec<F,Dim>. apply(p)=p+t; inverse negates t; composition adds displacements and returns an independent value. All cost O(Dim), use no allocation, and retain no input addresses. Use this representation for a pure displacement; conversion to transform::from_translation expands it to a homogeneous matrix.")),
        datatype("micron::math::geometry::scaling", "math/geometry/scaling.hpp",
                 "Owns one factor per coordinate for axis scaling in 2 through 16 dimensions.",
                 section("Operations and requirements", "s is an embedded vec<F,Dim>. uniform(k) fills every factor, apply(p) multiplies coordinates, and composition multiplies corresponding factors. inverse takes 1/s[i] without checking zero or finiteness; all factors must be nonzero for an invertible scale. Negative factors are accepted and can reflect coordinates. Operations/copies cost O(Dim), with no allocator or retained pointers. from_scaling expands the value into a homogeneous transform but does not change the selected transform mode.")),
        datatype("micron::math::geometry::transform", "math/geometry/transform.hpp",
                 "Owns a homogeneous matrix for a two- or three-dimensional map, with a compile-time inverse/application mode.",
                 section("Modes and layout", "H=Dim+1 and M is mat<F,H,H>. transform_mode enumerates isometry,affine,projective in that order; affine is the default template argument. identity() explicitly builds the identity. A value-initialized transform contains a zero matrix, so identity must be requested when required. from_translation/from_scaling/from_linear fill the corresponding affine block; 3D factories additionally accept SO3, SE3, or quat. Quaternion conversion uses vector-first x,y,z,w components and does not normalize the input.",
                         "linear_part and translation_part copy the leading block/last column. apply_homogeneous returns M*p unchanged. apply(p) appends w=1; projective mode divides spatial outputs by the resulting w only when w!=0, otherwise retaining the numerator. Other modes discard the computed w. No point-at-infinity status is returned."),
                 section("Inverse, composition, and complexity", "Isometry inverse transposes the leading rotation block and computes -R^T*t, requiring an orthonormal block and affine bottom row. Affine inverse uses inv2/inv3 for the leading block and -A^-1*t. Projective inverse calls the general fixed-matrix inverse and takes its X member, discarding its diagnostic result. Singular or mislabeled transforms therefore have no geometry-level checked inverse convention.",
                         "Composition performs dense homogeneous matrix multiplication and promotes its result to projective if either operand is projective, otherwise affine if either is affine, otherwise isometry. This type promotion reflects labels, not a matrix-invariant check. Application/copy are O(H²), composition O(H³), and specialized isometry/affine inverses O(Dim²)/fixed small-matrix work; Dim is restricted to 2 or 3, so every operation is bounded-size work with no heap allocation."),
                 covers=["micron::math::geometry::transform_mode"]),
        datatype("micron::math::geometry::handedness", "math/geometry/projection.hpp",
                 "Selects right- or left-handed view/projection formulas at compile time.",
                 section("Projection inventory", "handedness is right or left; clip_depth is neg_one_to_one or zero_to_one. Defaults are right-handed eye space looking down -Z and clip depth [-1,1]. Left-handed formulas look down +Z; zero_to_one selects [0,1] depth. The projection functions return transform<F,3,projective>, including the orthographic matrix.", table=table(["Function", "Construction / requirements"], [
                     ["perspective_projection(fov_y,aspect,near_z,far_z)", "Symmetric perspective with tan(fov_y/2), angle in radians; nonzero aspect and distinct usable near/far"],
                     ["frustum(left,right,bottom,top,near_z,far_z)", "Off-center perspective; nonzero horizontal/vertical/depth extents and usable perspective depth"],
                     ["orthographic_projection(left,right,bottom,top,near_z,far_z)", "Axis scale/translation; nonzero extents"],
                     ["look_at(eye,target,up)", "Normalizes forward and side, derives corrected up, and stores -R*eye translation"],
                     ["inv_perspective; inv_orthographic", "Closed-form inverses for the respective generated matrix layouts; not arbitrary projective-matrix inverses"],
                 ])),
                 section("Numerical and graphics notes", "The constructors do not validate frustum geometry, near/far signs/order, finite parameters, or zero denominators, and provide no special infinite-far or reversed-Z option. A singular layout can produce nonfinite matrix elements. All listed constructors/inverses cost O(1) and own fixed 4x4 outputs.",
                         "look_at requires distinct eye/target and an up vector not parallel to the view direction for a valid isometry. Zero norms skip normalization and leave degenerate basis vectors; no failure status is returned. The f32 SIMD overload uses shared graphics kernels. policy::fast_tag selects approximate reciprocal-square-root normalization on that path; other cases delegate to ordinary look_at. Handedness/depth do not by themselves flip framebuffer Y or perform viewport mapping."),
                 covers=["micron::math::geometry::clip_depth"]),
    ],
    "sources": ['math/geometry.hpp',
 'math/geometry/aligned_box.hpp',
 'math/geometry/hyperplane.hpp',
 'math/geometry/ortho.hpp',
 'math/geometry/parametrized_line.hpp',
 'math/geometry/projection.hpp',
 'math/geometry/scaling.hpp',
 'math/geometry/transform.hpp',
 'math/geometry/translation.hpp',
 'math/geometry/umeyama.hpp'],
}}


