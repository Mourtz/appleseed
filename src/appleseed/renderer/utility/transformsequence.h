
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef APPLESEED_RENDERER_UTILITY_TRANSFORMSEQUENCE_H
#define APPLESEED_RENDERER_UTILITY_TRANSFORMSEQUENCE_H

// appleseed.foundation headers.
#include "foundation/math/aabb.h"
#include "foundation/math/transform.h"
#include "foundation/platform/compiler.h"

// appleseed.main headers.
#include "main/dllsymbol.h"

// Standard headers.
#include <cstddef>

namespace renderer
{

//
// A piecewise linear rigid transform path as a function of time.
//

class DLLSYMBOL TransformSequence
{
  public:
    // Constructors.
    TransformSequence();
    TransformSequence(const TransformSequence& rhs);

    // Destructor.
    ~TransformSequence();

    // Assignment operator.
    TransformSequence& operator=(const TransformSequence& rhs);

    // Return true if the sequence is empty.
    bool empty() const;

    // Return the number of transforms in the sequence.
    size_t size() const;

    // Remove all transforms from the sequence.
    void clear();

    // Set a transform at a given time.
    // Replaces any transform already set at this time.
    void set_transform(
        const double                    time,
        const foundation::Transformd&   transform);

    // Return a given (time, transform) pair.
    void get_transform(
        const size_t                    index,
        double&                         time,
        foundation::Transformd&         transform) const;

    // Return the transform with the lowest time value.
    // Return the identity transform if the sequence is empty.
    const foundation::Transformd& get_earliest_transform() const;

    // Prepare the sequence for quick evaluation.
    // This method must be called after new transforms have been set and
    // before any call to evaluate(). It may be called multiple times.
    // Returns true on success, false otherwise.
    bool prepare();

    // Compute the transform for any time value.
    foundation::Transformd evaluate(const double time) const;

    // A variant of evaluate() that avoids copying transforms in certain cases.
    const foundation::Transformd& evaluate(
        const double                    time,
        foundation::Transformd&         tmp) const;

    // Compose two transform sequences.
    TransformSequence operator*(const TransformSequence& rhs) const;

    // Transform a 3D axis-aligned bounding box.
    // If the bounding box is invalid, it is returned unmodified.
    template <typename T>
    foundation::AABB<T, 3> to_parent(const foundation::AABB<T, 3>& bbox) const;

  private:
    struct TransformKey
    {
        double                          m_time;
        foundation::Transformd          m_transform;

        bool operator<(const TransformKey& rhs) const
        {
            return m_time < rhs.m_time;
        }
    };

    size_t                              m_capacity;
    size_t                              m_size;
    TransformKey*                       m_keys;
    foundation::TransformInterpolatord* m_interpolators;

    void copy_from(const TransformSequence& rhs);

    void interpolate(
        const double                    time,
        foundation::Transformd&         result) const;

    foundation::AABB3d compute_motion_segment_bbox(
        const foundation::AABB3d&       bbox,
        const foundation::Transformd&   from,
        const foundation::Transformd&   to) const;
};


//
// TransformSequence class implementation.
//

inline bool TransformSequence::empty() const
{
    return m_size == 0;
}

inline size_t TransformSequence::size() const
{
    return m_size;
}

inline foundation::Transformd TransformSequence::evaluate(const double time) const
{
    foundation::Transformd tmp;
    return evaluate(time, tmp);
}

FORCE_INLINE const foundation::Transformd& TransformSequence::evaluate(
    const double            time,
    foundation::Transformd& tmp) const
{
    if (m_size == 0)
    {
        tmp = foundation::Transformd::identity();
        return tmp;
    }

    assert(m_size == 1 || m_interpolators != 0);

    const TransformKey* first = m_keys;

    if (m_size == 1)
        return first->m_transform;

    if (time <= first->m_time)
        return first->m_transform;

    const TransformKey* last = m_keys + m_size - 1;

    if (time >= last->m_time)
        return last->m_transform;

    interpolate(time, tmp);

    return tmp;
}

template <typename T>
foundation::AABB<T, 3> TransformSequence::to_parent(const foundation::AABB<T, 3>& bbox) const
{
    if (m_size == 0 || !bbox.is_valid())
        return bbox;

    foundation::AABB<T, 3> result;
    result.invalidate();

    for (size_t i = 0; i < m_size - 1; ++i)
    {
        // Insert the bounding box of the path from this key frame to the next.
        result.insert(
            foundation::AABB<T, 3>(
                compute_motion_segment_bbox(
                    foundation::AABB3d(bbox),
                    m_keys[i].m_transform,
                    m_keys[i + 1].m_transform)));
    }

    // Insert the bounding box at the last key frame.
    result.insert(m_keys[m_size - 1].m_transform.to_parent(bbox));

    return result;
}

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_UTILITY_TRANSFORMSEQUENCE_H
