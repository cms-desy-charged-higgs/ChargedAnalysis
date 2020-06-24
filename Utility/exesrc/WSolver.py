#!/usr/bin/env python3

from sympy import *
from pprint import pprint

def main():
    p_xe, p_ye, p_ze, m_e = symbols('p_xe p_ye p_ze m_e', real=True)
    p_xv, p_yv, p_zv = symbols('p_xv p_yv p_zv', real=True)
    mW = symbols('mW', real=True)

    eq = sqrt(m_e**2 + 2*sqrt(m_e**2 + p_xe**2 + p_ye**2 + p_ze**2)*sqrt(p_xv**2 + p_yv**2 + p_zv**2) - 2*p_xe*p_xv -  2*p_ye*p_yv -  2*p_ze*p_zv)

    print(solve(Eq(eq, mW), p_zv, symplify = True))

if __name__ == "__main__":
    main()
