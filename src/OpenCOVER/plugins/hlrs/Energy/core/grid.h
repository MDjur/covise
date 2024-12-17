#ifndef _CORE_GRID_H
#define _CORE_GRID_H

#include <osg/Geode>
#include <osg/Group>
#include <osg/ShapeDrawable>

namespace core {
namespace grid {
class Point : public osg::Group {
 public:
  Point(const std::string &name, const float &x, const float &y, const float &z,
        const float &radius);

  const auto &getPosition() const { return m_point->getCenter(); }
  osg::ref_ptr<osg::Geode> getGeode() {
    return dynamic_cast<osg::Geode *>(osg::Group::getChild(0));
  }

 private:
  osg::ref_ptr<osg::ShapeDrawable> m_shape;
  osg::ref_ptr<osg::Sphere> m_point;
};

template <typename CoordType>
struct ConnectionData {
  std::string name;
  CoordType start;
  CoordType end;
  float radius;
  osg::ref_ptr<osg::TessellationHints> hints;
};

class Connection : public osg::Group {
  Connection(const std::string &name, const osg::Vec3 &start, const osg::Vec3 &end,
             const float &radius, osg::ref_ptr<osg::TessellationHints> hints);

 public:
  Connection(const ConnectionData<osg::Vec3> &data)
      : Connection(data.name, data.start, data.end, data.radius, data.hints){};

  Connection(const ConnectionData<Point> &data)
      : Connection(data.name, data.start.getPosition(), data.end.getPosition(),
                   data.radius, data.hints){};

  osg::Vec3 getDirection() const { return *m_end - *m_start; }
  osg::ref_ptr<osg::Geode> getGeode() const { return m_geode; }

 private:
  osg::ref_ptr<osg::Geode> m_geode;
  osg::Vec3 *m_start;
  osg::Vec3 *m_end;
};

typedef std::vector<osg::ref_ptr<Point>> Points;
typedef std::vector<osg::ref_ptr<Connection>> Connections;
// list of directed connections between points
// TODO: write a concept for PointType
// template <typename PointType>
// using ConnectivityList = std::vector<ConnectionData<PointType>>;
typedef std::vector<std::vector<int>> Indices;
}  // namespace grid
}  // namespace core

#endif
