/*
 * utility_point_types.h
 *
 *  Created on: Sep 5, 2012
 *      Author: Adam Stambler
 *
 */

#ifndef UTILITY_POINT_TYPES_H_
#define UTILITY_POINT_TYPES_H_

#include <pcl/point_types.h>

namespace osgpcl
{
/*
 * RadiusPointT
 * special point type defined for surfel radius
 */
struct RadiusPointT
{
    float radius;
};
}

POINT_CLOUD_REGISTER_POINT_STRUCT(osgpcl::RadiusPointT,
                                  (float, radius, radius))

#endif /* UTILITY_POINT_TYPES_H_ */
