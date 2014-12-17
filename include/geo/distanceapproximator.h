
#ifndef VALHALLA_GEO_DISTANCEAPPROXIMATOR_H_
#define VALHALLA_GEO_DISTANCEAPPROXIMATOR_H_

#include <math.h>

#include "pointll.h"

namespace valhalla{
namespace geo{

/**
 * Provides distance approximation in latitude, longitude space. Approximates
 * distance in kilometers between two points. This method is more efficient
 * than using spherical distance calculations. It computes an approximate
 * distance using the pythagorean theorem with the kilometers of latitude change
 * (exact) and the kilometers of longitude change at the "test point". Longitude
 * is inexact since kilometers per degree of longitude changes with latitude.
 * This approximation has very little error (less than 1%) if the positions
 * are close to one another (within several hundred kilometers). Error increases
 * at high (near polar) latitudes. This method will not work if the points
 * cross 180 degrees longitude.
 */
class DistanceApproximator
{
 public:
  /**
   * Constructor.
   */
  DistanceApproximator();

  /**
   * Destructor.
   */
  ~DistanceApproximator();

  /**
   * Sets the test point.  This method is used when a distance is to be
   * checked for a series of positions relative to a single point.  This
   * precalculates the kilometers per degree of longitude.
   * @param   ll    Latitude, longitude of the test point (degrees)
   */
  void SetTestPoint(const PointLL& ll);

  /**
   * Approximates the arc distance between the supplied position and the
   * current test point.  It uses the pythagorean theorem with kilometers
   * per latitude and longitude degree. Assumes the number of kilometers per
   * degree of longitude at the test point latitude.
   * @param   ll    Latitude, longitude of the point (degrees)
   * @return  Returns the squared distance in kilometers by using pythagorean
   *          theorem.  Squared distance is returned for more efficient
   *          searching (avoids sqrt).
   */
  float DistanceSquared(const PointLL& ll) const;

  /**
   * Approximates arc distance between 2 lat,lng positions using kilometers per
   * latitude and longitude degree.  Uses the mid latitude of the 2 positions
   * to estimate the number of kilometers per degree of longitude.
   * @param   ll1  First point (lat,lng)
   * @param   ll2  Second point (lat,lng)
   * @return  Returns the approximate distance squared (in km)
   */
  static float DistanceSquared(const PointLL& ll1, const PointLL& ll2);

  /**
   * Gets the number of kilometers per degree of longitude for a specified
   * latitude.  While the number of kilometers per degree of latitude is
   * constant, the number of kilometers per degree of longitude varies: it has
   * a maximum at the equator and lessens as latitude approaches the poles.
   * @param   lat   Latitude in degrees
   * @return  Returns the number of kilometers per degree of longitude
   */
  static float KmPerLngDegree(const float lat);

private:
   float centerlat_;
   float centerlng_;
   float km_per_lng_degree_;
};

}
}

#endif  // VALHALLA_GEO_DISTANCEAPPROXIMATOR_H_

