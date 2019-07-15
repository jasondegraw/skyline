// Copyright (c) 2019, Alliance for Sustainable Energy, LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef SKYLINE_HPP
#define SKYLINE_HPP

#include <numeric>
#include <optional>

namespace skyline {

template <typename I, typename R, template <typename ...> typename V> class SymmetricMatrix
{
public:

  SymmetricMatrix(V<V<R>> &M)
  {
    I n = M.size();
    for (auto &v : M) {
      n = std::min(n, v.size());
    }
    m_ih.resize(n);
    for (I i = 0; i < n; i++) {
      m_ih[i] = i;
      for (I j = 0; j < i; j++) {
        if (M[i][j] != 0.0) {
          break;
        }
        --m_ih[i];
      }
    }
    auto sum = std::accumulate(m_ih.begin(), m_ih.end(), (I)0);

    m_ik.resize(n);
    m_im.resize(n);
    // Convert heights to column offsets.
    m_ik[0] = 0;
    m_im[0] = 0;
    for (I k = 1; k < n; ++k) {
      m_ik[k] = m_ik[k - 1] + m_ih[k - 1];
      m_im[k] = k - m_ih[k];
    }

    // Copy into the am array
#ifdef SKYLINE_SINGLE_ARRAY
    m_am.resize(n + sum);
#else
    m_ad.resize(n);
    m_au.resize(sum);
#endif
    I count = 0;
    for (I i = 0; i < n; i++) {
#ifdef SKYLINE_SINGLE_ARRAY
      m_am[i] = M[i][i];
#else
      m_ad[i] = M[i][i];
#endif
      I j;
      for (j = 0; j < i; j++) {
        if (M[i][j] != 0.0) {
          break;
        }
      }
      for (; j < i; j++) {
#ifdef SKYLINE_SINGLE_ARRAY
        m_am[n + count] = M[i][j];
#else
        m_au[count] = M[i][j];
#endif
        ++count;
      }
    }

    m_v.resize(n);
    m_n = n;
  }

  SymmetricMatrix(V<I> &heights) : m_ih(heights)
  {
    I n = m_ih.size();
    auto sum = std::accumulate(m_ih.begin(), m_ih.end(), (I)0);

    m_ik.resize(n);
    m_im.resize(n);
    // Convert heights to column offsets.
    m_ik[0] = 0;
    m_im[0] = 0;
    for (I k = 1; k < n; ++k) {
      m_ik[k] = m_ik[k - 1] + m_ih[k - 1];
      m_im[k] = k - m_ih[k];
    }

#ifdef SKYLINE_SINGLE_ARRAY
    // Size the am array
    I total{ n + sum };
    m_am.resize(total);
    for (I i = 0; i < total; i++) {
      m_am[i] = 0.0;
    }
#else
    // Size the au/d array
    m_ad.resize(n);
    for (I i = 0; i < n; i++) {
      m_ad[i] = 0.0;
    }

    m_au.resize(sum);
    for (I i = 0; i < sum; i++) {
      m_au[i] = 0.0;
    }
#endif

    m_v.resize(n);
    m_n = n;
  }

  void fill(R v = 0.0)
  {
#ifdef SKYLINE_SINGLE_ARRAY
    std::fill(m_am.begin(), m_am.end(), v);
#else
    std::fill(m_ad.begin(), m_ad.end(), v);
    std::fill(m_au.begin(), m_au.end(), v);
#endif
  }

  V<I> offsets() const
  {
    return m_ik;
  }

  V<I> heights() const
  {
    return m_ih;
  }

  V<I> minima() const
  {
    return m_im;
  }

  V<R> diagonal() const
  {
#ifdef SKYLINE_SINGLE_ARRAY
    return V<R>(m_am.begin(), m_am.begin() + m_n);
#else
    return m_ad;
#endif
  }

  V<R> upper() const
  {
#ifdef SKYLINE_SINGLE_ARRAY
    return V<R>(m_am.begin()+m_n, m_am.end());
#else
    return m_au;
#endif
  }

  V<R> lower() const
  {
#ifdef SKYLINE_SINGLE_ARRAY
    return V<R>(m_am.begin() + m_n, m_am.end());
#else
    return m_au;
#endif
  }

  R &operator()(I i)
  {
#ifdef SKYLINE_SINGLE_ARRAY
    return m_am[i];
#else
    return m_au[i];
#endif
  }

  R &diagonal(I i)
  {
#ifdef SKYLINE_SINGLE_ARRAY
    return m_am[i]; // This ends up being the same as operator()
#else
    return m_ad[i];
#endif
  }

  std::optional<I> index(I i, I j) const
  {
    if (m_im[j] <= i) {
      return m_n + m_ik[j] + i - m_im[j];
    } else if (m_im[j] == i) {
      return i;
    }
    return {};
  }

  void utdu()
  {
    // j = 0, nothing much to do
    for (I k = 1; k < m_n; ++k) {
      if (m_im[k] == 0) {
#ifdef SKYLINE_SINGLE_ARRAY
        I ij = m_n + m_ik[k];
        m_am[ij] /= m_am[0];
#else
        I ij = m_ik[k];
        m_au[ij] /= m_ad[0];
#endif
      }
    }
    // Now for the rest
    for (I j = 1; j < m_n; ++j) {
      // Compute v
      for (I i = 0; i < m_im[j]; ++i) {
        m_v[i] = 0.0;
      }
      for (I i = m_im[j]; i < j; ++i) {
#ifdef SKYLINE_SINGLE_ARRAY
        m_v[i] = m_am[m_n + m_ik[j] + i - m_im[j]] * m_am[i]; // OK, i >= m_im[j]
#else
        m_v[i] = m_au[m_ik[j] + i - m_im[j]] * m_ad[i]; // OK, i >= m_im[j]
#endif
      }
      // Compute the diagonal term
      R value = 0.0;
      for (I i = m_im[j]; i < j; ++i) {
#ifdef SKYLINE_SINGLE_ARRAY
        value += m_am[m_n + m_ik[j] + i - m_im[j]] * m_v[i];  // OK, i >= m_im[j]
#else
        value += m_au[m_ik[j] + i - m_im[j]] * m_v[i];  // OK, i >= m_im[j]
#endif
      }
#ifdef SKYLINE_SINGLE_ARRAY
      m_am[j] -= value;
#else
      m_ad[j] -= value;
#endif
      // Compute the rest of the row
      for (I k = j + 1; k < m_n; ++k) {
        if (m_im[k] <= j) {
          value = 0.0;
#ifdef SKYLINE_SINGLE_ARRAY
          for (I i = m_im[k]; i < j; ++i) {
            I ij = m_n + m_ik[k] + i - m_im[k]; // OK, i >= m_im[k]
            value += m_am[ij] * m_v[i];
          }
          I ij = m_n + m_ik[k] + j - m_im[k]; // OK, j >= m_im[k]
          m_am[ij] = (m_am[ij] - value) / m_am[j];
#else
          for (I i = m_im[k]; i < j; ++i) {
            I ij = m_ik[k] + i - m_im[k]; // OK, i >= m_im[k]
            value += m_au[ij] * m_v[i];
          }
          I ij = m_ik[k] + j - m_im[k]; // OK, j >= m_im[k]
          m_au[ij] = (m_au[ij] - value) / m_ad[j];
#endif
        }
      }
    }
  }

  void forward_substitution(V<R> &b) const
  {
    // Solve Lz=b (Dy=z, Ux=y)
    for (I i = 1; i < m_n; ++i) {
      R value = 0.0;
      for (I k = m_im[i]; k < i; ++k) {
#ifdef SKYLINE_SINGLE_ARRAY
        I ij = m_n + m_ik[i] + k - m_im[i];
        value += m_am[ij] * b[k];
#else
        I ij = m_ik[i] + k - m_im[i];
        value += m_au[ij] * b[k];
#endif
      }
      b[i] -= value;
    }
  }

  void back_substitution(V<R> &z) const
  {
    // Account for the diagonal first (invert Dy=z)
    for (I j = 0; j < m_n; ++j) {
#ifdef SKYLINE_SINGLE_ARRAY
      z[j] /= m_am[j];
#else
      z[j] /= m_ad[j];
#endif
    }
    // Solve Ux=y
    for (I j = m_n - 1; j > 0; --j) {
      for (I k = m_im[j]; k < j; ++k) {
#ifdef SKYLINE_SINGLE_ARRAY
        I ij = m_n + m_ik[j] + k - m_im[j];
        z[k] -= z[j] * m_am[ij];
#else
        I ij = m_ik[j] + k - m_im[j];
        z[k] -= z[j] * m_au[ij];
#endif
      }
    }
  }

  void ldlt_solve(V<R> &b)
  {
    utdu();
    forward_substitution(b);
    back_substitution(b);
  }

  I rows() const
  {
    return m_n;
  }

  I cols() const
  {
    return m_n;
  }

private:

  I m_n;     // System size
  V<I> m_ik; // Index offsets to top of skylines
  V<I> m_ih; // Height of each skyline (not used, should probably be removed)
  V<I> m_im; // Minimum row, or top of skyline
#ifdef SKYLINE_SINGLE_ARRAY
  V<R> m_am; // The entire matrix in one vector, first the diagonal, then the rest
#else
  V<R> m_au; // Upper triangular part of matrix
  V<R> m_ad; // Diagonal of matrix
#endif
  V<R> m_v;  // Temporary used in solution
};

}

#endif // !SKYLINE_HPP
