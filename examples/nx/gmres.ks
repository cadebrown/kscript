""" gmres.ks - GMRES implementation in kscript, using the 'nx' package

TODO: Implement some more methods

@author: Cade Brown <cade@kscript.org>
"""

import nx

func gmres(A, b, x0, niter=10) {
    r = b - A @ x0

    x = []
    q = [0] * niter

    x.push(r)
    q[0] = r / nx.la.norm(r)

    h = nx.zeros((niter+1, niter))
    
    for k in range(niter) {
        y = A @ q[k]

        for j in range(k) {
            h[j, k] = q[j] @ y
            y = y - h[j, k] * q[j]
        }

        h[k + 1, k] = nx.la.norm(y)
        if h[k + 1, k] != 0 && k != niter - 1 {
            q[k + 1] = y / h[k + 1, k]
        }

        b = nx.zeros(niter + 1)
        b[0] = nx.la.norm(r)

        res = nx.la.lstsq(h, b)[0]
        x.push(nx.array(q) @ res + x0)
    }

    ret x
}

print (gmres([[1, 0], [0, 1]] as nx.array, [0, 1] as nx.array, [0, 1] as nx.array))

