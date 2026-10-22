// Calculates the eigenvalues and eigenvectors of the input, based on QR
// iteration methods described in
// <https://people.inf.ethz.ch/arbenz/ewp/Lnotes/chapter4.pdf>. Eigenvalues are
// calculated using the following steps:
//
//   1. "Balance" the input by transforming it with `B = D^-1 A D`, where D is a
//      diagonal matrix. The elements of D are chosen so that if `||A||` is
//      relatively large then `||B|| < ||A||`. The error of the QR algorithm is
//      proportional to the norm of the input, so this step aims to improve the
//      accuracy of the final output.
//
//   2. Reduce B to Hessenberg form, `H = P^* B P`, which is almost an upper
//      triangular matrix, except that the elements directly below the diagonal
//      can be nonzero. P is a product of householder reflector matrices, each
//      chosen to zero out the elements `row_index > column_index + 1` in one
//      column. A householder reflector is any matrix that can be written in
//      the form `I - 2 u u^*`, `u` is a vector.
//
//      We choose one householder reflector for all but the last two columns of
//      `B`. We apply the householder reflector for each column `k` in sequence,
//      `H_{k+1} = (P_k)^* H_k P_k`, starting with `H_0 = B`. The householder
//      reflector will zero out the elements `H_k[k+2:, k]`, which is done like
//      so:
//
//        x_k = H_k[k+1:, k]
//        u_k'[0] = x_k[0] (1 + ||x_k|| / |x_k[0]|)
//        u_k'[1:] = x_k[1:]
//        u_k = u_k' / ||u_k'||
//        P_k' = I - 2 u_k (u_k)^*
//
//      If `B` is an n x n matrix, `P_k` is (n-k-1) x (n-k-1), so we pad the
//      first `k + 1` dimension with an identity matrix:
//
//        P_k =  I  0
//               0  P_k'
//
//      However we avoid forming the entire `P_k` matrix and calculating the two
//      full matrix multiplies, `(P_k)^* H_k P_k`. Instead, we simplify it to
//      two matrix-vector products and two vector outer products, based on the
//      fact that `H_k P_k = H_k (I - 2 u u^*) = H_k - 2 H_k u u^*`.
//
//   3. Perform Hessenberg QR iteration to convert H into an upper triangular
//      matrix T. The method we use is based on the following "explicit"
//      Hessenberg QR iteration with a Wilkinson shift:
//
//        Each iteration aims to bring one of the elements, called the target
//        element, below the diagonal closer to 0. It starts with the
//        bottom-rightmost element, iterates until it is near zero, and moves on
//        to the next element to the above-left. Each iteration applies the
//        transformation:
//
//          Q, R = decompQR(H[:m, :m] - μI)
//          H[:m, :m] := R Q + μI
//
//        `H[m-1, m-2]` marks the target element, and `m` is decremented each
//        time the current target element reaches near zero. `μ` is a Wilkinson
//        shift value. To calculate it, we take the 2x2 matrix at the bottom
//        right corner of `H[:m, :m]`, which is `H[m-2:m, m-2:m]`, calculate its
//        two eigenvalues and choose whichever one is closest to the bottom
//        right element, `H[m-1, m-1]`.
//
//      However, we actually avoid calculating the QR decomposition directly.
//      Instead we apply a series of Givens rotations which mathematically gives
//      the same result as one iteration of explicit Hessenberg QR iteration
//      with less computational work.
//
// The final output, T, is upper triangular, so its diagonal elements are its
// eigenvalues. Since T was obtained only by applying similarity
// transformations, which preserve the eigenvalues, its diagonal elements are
// the eigenvalues of the original input A.
//
// If `compute_eigenvectors` is turned on, we compute the Schur vectors of B,
// which are the columns of the matrix `(prod P_k) (prod Q_j)`, where `P_k` is
// one of the padded householder reflectors and `Q_j` is one of the Q matrices
// from QR decomposition during QR iteration. Then we convert the schur vectors
// into the eigenvectors.
//
// This is accomplished in the following steps:
//
//   1. After the reduction of B to Hessenberg form, recover the product of
//      the householder reflectors, since we didn't calculate it explicitly.
//      This is done efficiently with `at::linalg_householder_product`, since it
//      avoids calculating full matrix multiplications. Given a matrix
//      containing `v_k` and `tau_k` for each iteration k of the hessenberg
//      reduction, it calculates:
//
//        P = prod P_k = prod (I - 2 u_k (u_k)^*) = prod (I - tau_k v_k (v_k)^*)
//
//      `v` is defined as a vector where `v_k[:k] = 0` and `v_k[k] = 1`. In terms
//      of `u_k'` from earlier:
//
//        v_k = u_k' / u_k'[0]
//        tau_k = 2 |u_k'[0]|^2 / ||u_k'||^2
//
//      `tau_k` and `v_k` are both calculated during the Hessenberg reduction
//      step itself.  `tau_k` is stored in its own tensor, but `v_k` is actually
//      written into the elements of H that were zeroed out, `H[k+2:, k]`. Since
//      those elements are known to be zero, they are reused rather than
//      creating a separate tensor for `v_k`.
//
//   2. 