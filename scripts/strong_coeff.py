"""Leading-coefficient test for looped symmetric near-path targets."""

import math


def quotient_data(degrees):
    """Return the looped symmetric quotient matrix and orbit sizes."""
    dim = len(degrees) + 1
    q = [[0.0] * dim for _ in range(dim)]
    q[0][0] = 1.0
    for i, d in enumerate(degrees):
        q[i][i + 1] = float(d)
        q[i + 1][i] = 1.0

    sizes = [1.0]
    for d in degrees:
        sizes.append(sizes[-1] * float(d))
    return q, sizes


def matvec(q, v):
    """Multiply a dense square matrix by a vector."""
    return [sum(row[j] * v[j] for j in range(len(v))) for row in q]


def symmetric_eigenpairs(matrix, sweeps=100, tolerance=1e-14):
    """Compute eigenpairs of a small symmetric matrix by Jacobi rotations."""
    n = len(matrix)
    a = [row[:] for row in matrix]
    vectors = [[1.0 if i == j else 0.0 for j in range(n)] for i in range(n)]

    for _ in range(sweeps):
        p, q = 0, 1
        max_offdiag = 0.0
        for i in range(n):
            for j in range(i + 1, n):
                value = abs(a[i][j])
                if value > max_offdiag:
                    max_offdiag = value
                    p, q = i, j
        if max_offdiag < tolerance:
            break

        if a[p][p] == a[q][q]:
            angle = math.pi / 4.0
        else:
            angle = 0.5 * math.atan2(2.0 * a[p][q], a[q][q] - a[p][p])
        c = math.cos(angle)
        s = math.sin(angle)

        app = c * c * a[p][p] - 2.0 * s * c * a[p][q] + s * s * a[q][q]
        aqq = s * s * a[p][p] + 2.0 * s * c * a[p][q] + c * c * a[q][q]
        a[p][p] = app
        a[q][q] = aqq
        a[p][q] = 0.0
        a[q][p] = 0.0

        for k in range(n):
            if k == p or k == q:
                continue
            akp = c * a[k][p] - s * a[k][q]
            akq = s * a[k][p] + c * a[k][q]
            a[k][p] = a[p][k] = akp
            a[k][q] = a[q][k] = akq

        for k in range(n):
            vkp = c * vectors[k][p] - s * vectors[k][q]
            vkq = s * vectors[k][p] + c * vectors[k][q]
            vectors[k][p] = vkp
            vectors[k][q] = vkq

    return [(a[i][i], [vectors[row][i] for row in range(n)]) for i in range(n)]


def leading_ratio(degrees):
    """Return the Perron leading-coefficient ratio for E_n^(2) versus P_n."""
    q, sizes = quotient_data(degrees)
    dim = len(q)
    b = [
        [q[i][j] * math.sqrt(sizes[i] / sizes[j]) for j in range(dim)]
        for i in range(dim)
    ]
    lam, y = max(symmetric_eigenpairs(b), key=lambda pair: pair[0])
    if y[0] < 0:
        y = [-x for x in y]
    u = [y[i] / math.sqrt(sizes[i]) for i in range(dim)]
    one = [1.0] * len(q)
    w1 = matvec(q, one)
    w2 = matvec(q, w1)
    numerator = sum(sizes[i] * w1[i] * w2[i] * u[i] for i in range(len(q)))
    denominator = (lam**3) * sum(sizes[i] * u[i] for i in range(len(q)))
    return numerator / denominator
