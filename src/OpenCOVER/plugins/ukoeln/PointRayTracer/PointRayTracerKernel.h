/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef POINT_RAY_TRACER_KERNEL_H
#define POINT_RAY_TRACER_KERNEL_H

//#include <visionaray/get_color.h>

#include "PointRayTracerGlobals.h"

// kernel with ray tracing logic
template <typename BVHs>
struct Kernel
{
    using R = ray_type;
    using S = R::scalar_type;
    using C = visionaray::vector<4, S>;

    Kernel(BVHs bvhs_begin, BVHs bvhs_end)
        : m_bvhs_begin(bvhs_begin)
        , m_bvhs_end(bvhs_end)
    {
    }

    VSNRAY_FUNC
    visionaray::result_record<S> operator()(R ray)
    {
        visionaray::result_record<S> result;
        result.color = C(0.f);

        auto hit_rec = visionaray::closest_hit(
                ray,
                m_bvhs_begin,
                m_bvhs_end
                );

        result.hit = hit_rec.hit;

        auto color = hit_rec.color;

        result.color = visionaray::select(
                hit_rec.hit,
                C(visionaray::vector<3, S>(color), S(1.0)),
                result.color
                );

        result.depth = hit_rec.t;

        return result;
    }

    BVHs m_bvhs_begin;
    BVHs m_bvhs_end;
};

#endif // POINT_RAY_TRACER_KERNEL_H
